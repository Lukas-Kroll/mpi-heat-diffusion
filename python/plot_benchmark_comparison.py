import argparse
import csv
import statistics
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt


def load_measurements(csv_path):
    """Read raw serial and MPI measurements from a benchmark CSV."""
    required_columns = {
        "variant",
        "processes",
        "grid_size",
        "iterations",
        "repeat",
        "seconds",
    }

    groups = defaultdict(list)

    with csv_path.open(newline="", encoding="utf-8") as file:
        reader = csv.DictReader(file)

        if not required_columns.issubset(reader.fieldnames or []):
            raise ValueError(
                "CSV is missing required columns: "
                + ", ".join(sorted(required_columns))
            )

        for row in reader:
            key = (
                row["variant"].lower(),
                int(row["processes"]),
                int(row["grid_size"]),
            )
            groups[key].append(float(row["seconds"]))

    if not groups:
        raise ValueError("The CSV file contains no measurements.")

    return groups


def median_runtime(groups, variant, processes, size):
    """Return the median, minimum and maximum for one configuration."""
    key = (variant, processes, size)

    if key not in groups:
        raise ValueError(
            f"Missing measurements for {variant}, "
            f"{processes} processes, grid {size}."
        )

    values = groups[key]

    return (
        statistics.median(values),
        min(values),
        max(values),
    )


def plot_runtime(groups, sizes, process_counts, output_dir):
    """Plot runtime medians and the observed minimum/maximum range."""
    fig, ax = plt.subplots(figsize=(9, 5.5))

    configurations = [("Serial", "serial", 1)]
    configurations.extend(
        (f"MPI ({p} processes)", "mpi", p)
        for p in process_counts
    )

    for label, variant, processes in configurations:
        results = [
            median_runtime(groups, variant, processes, size)
            for size in sizes
        ]

        medians = [result[0] for result in results]
        minima = [result[1] for result in results]
        maxima = [result[2] for result in results]

        lower_errors = [
            median - minimum
            for median, minimum in zip(medians, minima)
        ]
        upper_errors = [
            maximum - median
            for median, maximum in zip(medians, maxima)
        ]

        ax.errorbar(
            sizes,
            medians,
            yerr=[lower_errors, upper_errors],
            marker="o",
            capsize=4,
            label=label,
        )

    ax.set_title("Heat Diffusion: Runtime Comparison")
    ax.set_xlabel("Grid size (N × N)")
    ax.set_ylabel("Runtime (seconds)")
    ax.set_xticks(sizes)
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()

    output_path = output_dir / "runtime_comparison.png"
    fig.savefig(output_path, dpi=160)
    plt.close(fig)

    print(f"Runtime graph saved to: {output_path}")


def plot_speedup(groups, sizes, process_counts, output_dir):
    """Plot measured MPI speedup and ideal speedup."""
    fig, ax = plt.subplots(figsize=(9, 5.5))

    for processes in process_counts:
        speedups = []

        for size in sizes:
            serial_time, _, _ = median_runtime(
                groups, "serial", 1, size
            )
            mpi_time, _, _ = median_runtime(
                groups, "mpi", processes, size
            )

            speedups.append(serial_time / mpi_time)

        ax.plot(
            sizes,
            speedups,
            marker="o",
            label=f"MPI ({processes} processes)",
        )

    # Ideal speedup is equal to the number of processes.
    for processes in process_counts:
        ax.axhline(
            y=processes,
            linestyle="--",
            alpha=0.6,
            label=f"Ideal ({processes} processes)",
        )

    ax.set_title("MPI Speedup by Grid Size")
    ax.set_xlabel("Grid size (N × N)")
    ax.set_ylabel("Speedup (serial runtime / MPI runtime)")
    ax.set_xticks(sizes)
    ax.set_ylim(bottom=0)
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()

    output_path = output_dir / "speedup_comparison.png"
    fig.savefig(output_path, dpi=160)
    plt.close(fig)

    print(f"Speedup graph saved to: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Plot serial and MPI heat diffusion benchmarks."
    )
    parser.add_argument(
        "csv",
        nargs="?",
        default="results/benchmark_comparison_robust.csv",
        help="CSV file containing raw benchmark measurements",
    )
    parser.add_argument(
        "--out-dir",
        default="results",
        help="Directory in which to save the graphs",
    )
    args = parser.parse_args()

    csv_path = Path(args.csv)
    if not csv_path.is_file():
        parser.error(f"Benchmark CSV not found: {csv_path}")

    try:
        groups = load_measurements(csv_path)
        sizes = sorted({key[2] for key in groups})
        process_counts = sorted({
            key[1]
            for key in groups
            if key[0] == "mpi"
        })

        if not process_counts:
            parser.error("No MPI measurements found in the CSV.")

        output_dir = Path(args.out_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        plot_runtime(groups, sizes, process_counts, output_dir)
        plot_speedup(groups, sizes, process_counts, output_dir)

    except (ValueError, KeyError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()