import argparse
import csv
import re
import statistics
import subprocess
from pathlib import Path


RUNTIME_PATTERN = re.compile(
    r"^COMPUTE_TIME_SECONDS=([0-9.eE+-]+)$",
    re.MULTILINE,
)


def run_simulation(
        executable: Path,
        size: int,
        iterations: int,
) -> float:
    command = [
        str(executable),
        "-n", str(iterations),
        "-x", str(size),
        "-y", str(size),
        "-a", "0.20",
        "-t", "100",
        "-s", "10",
    ]

    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        check=True,
    )

    match = RUNTIME_PATTERN.search(result.stdout)

    if match is None:
        raise RuntimeError(
            "Could not find COMPUTE_TIME_SECONDS in program output:\n"
            + result.stdout
        )

    return float(match.group(1))


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Benchmark the serial heat diffusion solver."
    )
    parser.add_argument(
        "--executable",
        type=Path,
        default=Path("build-release/heat_sim"),
        help="Path to the compiled heat_sim executable",
    )
    parser.add_argument(
        "--sizes",
        nargs="+",
        type=int,
        default=[250, 500, 750, 1000],
        help="Square grid side lengths",
    )
    parser.add_argument(
        "--iterations",
        type=int,
        default=100,
        help="Number of simulation iterations",
    )
    parser.add_argument(
        "--repeats",
        type=int,
        default=3,
        help="Measured runs per grid size",
    )
    parser.add_argument(
        "--warmups",
        type=int,
        default=1,
        help="Unrecorded warm-up runs per grid size",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("benchmark_results.csv"),
        help="CSV file for raw benchmark results",
    )

    args = parser.parse_args()

    executable = args.executable.resolve()

    if not executable.is_file():
        parser.error(f"Executable not found: {executable}")

    if not args.sizes or any(size < 12 for size in args.sizes):
        parser.error("All grid sizes must be at least 12.")

    if args.iterations < 1:
        parser.error("Iterations must be at least 1.")

    if args.repeats < 1 or args.warmups < 0:
        parser.error("Repeats must be positive; warmups cannot be negative.")

    rows = []

    for size in args.sizes:
        print(f"\nBenchmarking grid: {size} x {size}")

        for _ in range(args.warmups):
            run_simulation(executable, size, args.iterations)

        times = []

        for repeat in range(1, args.repeats + 1):
            elapsed = run_simulation(
                executable,
                size,
                args.iterations,
            )
            times.append(elapsed)

            rows.append({
                "grid_size": size,
                "iterations": args.iterations,
                "repeat": repeat,
                "seconds": f"{elapsed:.9f}",
            })

            print(f"  Run {repeat}: {elapsed:.6f} seconds")

        print(
            f"  Median: {statistics.median(times):.6f} seconds"
        )

    args.output.parent.mkdir(parents=True, exist_ok=True)

    with args.output.open("w", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(
            file,
            fieldnames=[
                "grid_size",
                "iterations",
                "repeat",
                "seconds",
            ],
        )
        writer.writeheader()
        writer.writerows(rows)

    print(f"\nRaw results saved to: {args.output}")


if __name__ == "__main__":
    main()