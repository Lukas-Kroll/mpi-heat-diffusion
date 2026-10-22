import argparse
import csv
import statistics
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Plot serial heat diffusion benchmark results."
    )
    parser.add_argument(
        "csv",
        nargs="?",
        default="benchmark_results.csv",
        help="Raw benchmark CSV file",
    )
    parser.add_argument(
        "--save",
        default="serial_benchmark.png",
        help="Output image path",
    )
    args = parser.parse_args()

    csv_path = Path(args.csv)

    if not csv_path.is_file():
        parser.error(f"Benchmark file not found: {csv_path}")

    groups = defaultdict(list)

    with csv_path.open(newline="", encoding="utf-8") as file:
        reader = csv.DictReader(file)

        required_columns = {
            "grid_size", "iterations", "repeat", "seconds"
        }
        if not required_columns.issubset(reader.fieldnames or []):
            parser.error("CSV does not contain the expected columns.")

        for row in reader:
            groups[int(row["grid_size"])].append(
                float(row["seconds"])
            )

    if not groups:
        parser.error("The benchmark CSV contains no measurements.")

    sizes = sorted(groups)
    medians = [statistics.median(groups[size]) for size in sizes]
    minima = [min(groups[size]) for size in sizes]
    maxima = [max(groups[size]) for size in sizes]


    lower_error = [m - lo for m, lo in zip(medians, minima)]
    upper_error = [hi - m for m, hi in zip(medians, maxima)]

    fig, ax = plt.subplots(figsize=(8, 5))

    ax.errorbar(
        sizes,
        medians,
        yerr=[lower_error, upper_error],
        fmt="o-",
        capsize=4,
    )

    ax.set_title("Serial Heat Diffusion Performance")
    ax.set_xlabel("Grid side length (N × N)")
    ax.set_ylabel("Compute time (seconds)")
    ax.grid(True, alpha=0.3)

    fig.tight_layout()

    output_path = Path(args.save)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_path, dpi=150)
    plt.close(fig)

    print(f"Benchmark plot saved to: {output_path}")


if __name__ == "__main__":
    main()