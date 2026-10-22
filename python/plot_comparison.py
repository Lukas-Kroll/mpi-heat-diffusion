import argparse
import csv
import statistics
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "csv",
        nargs="?",
        default="benchmark_comparison.csv",
    )
    args = parser.parse_args()

    measurements = defaultdict(list)

    with Path(args.csv).open(newline="", encoding="utf-8") as file:
        for row in csv.DictReader(file):
            key = (
                row["variant"],
                int(row["processes"]),
                int(row["grid_size"]),
            )
            measurements[key].append(float(row["seconds"]))

    medians = {
        key: statistics.median(values)
        for key, values in measurements.items()
    }

    sizes = sorted({key[2] for key in medians})
    process_counts = sorted({
        key[1] for key in medians if key[0] == "mpi"
    })

    fig, ax = plt.subplots(figsize=(8, 5))

    for variant, processes, label in [
        ("serial", 1, "Serial"),
        *[
            ("mpi", p, f"MPI ({p} processes)")
            for p in process_counts
        ],
    ]:
        available = [
            size for size in sizes
            if (variant, processes, size) in medians
        ]
        times = [
            medians[(variant, processes, size)]
            for size in available
        ]
        ax.plot(available, times, marker="o", label=label)

    ax.set_title("Heat Diffusion Runtime")
    ax.set_xlabel("Grid side length (N × N)")
    ax.set_ylabel("Median compute time (seconds)")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig("runtime_comparison.png", dpi=150)
    plt.close(fig)

    for metric in ("speedup", "efficiency"):
        fig, ax = plt.subplots(figsize=(8, 5))

        for processes in process_counts:
            available = [
                size for size in sizes
                if ("serial", 1, size) in medians
                   and ("mpi", processes, size) in medians
            ]

            values = []

            for size in available:
                serial_time = medians[("serial", 1, size)]
                mpi_time = medians[("mpi", processes, size)]
                speedup = serial_time / mpi_time

                if metric == "efficiency":
                    values.append(speedup / processes)
                else:
                    values.append(speedup)

            ax.plot(
                available,
                values,
                marker="o",
                label=f"{processes} processes",
            )

        ax.set_title(
            "MPI Speedup" if metric == "speedup"
            else "MPI Parallel Efficiency"
        )
        ax.set_xlabel("Grid side length (N × N)")
        ax.set_ylabel(
            "Speedup" if metric == "speedup"
            else "Efficiency"
        )
        ax.grid(True, alpha=0.3)
        ax.legend()
        fig.tight_layout()
        fig.savefig(f"mpi_{metric}.png", dpi=150)
        plt.close(fig)

    print("Saved runtime_comparison.png")
    print("Saved mpi_speedup.png")
    print("Saved mpi_efficiency.png")


if __name__ == "__main__":
    main()