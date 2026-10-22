import argparse
import csv
import re
import statistics
import subprocess
from pathlib import Path
from collections import defaultdict


TIME_PATTERN = re.compile(
    r"^COMPUTE_TIME_SECONDS=([0-9.eE+-]+)$",
    re.MULTILINE,
)


def run_simulation(executable, size, iterations, processes=None, mpirun="mpirun"):
    arguments = [
        "-n", str(iterations),
        "-x", str(size),
        "-y", str(size),
        "-a", "0.20",
        "-t", "100",
        "-s", "10",
    ]

    if processes is None:
        command = [str(executable), *arguments]
    else:
        command = [
            mpirun, "-np", str(processes), str(executable), *arguments
        ]

    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        check=True,
    )

    match = TIME_PATTERN.search(result.stdout)

    if match is None:
        raise RuntimeError(
            f"No compute time found in output:\n{result.stdout}"
        )

    return float(match.group(1))


def measure(executable, size, iterations, repeats, warmups,
            processes=None, mpirun="mpirun"):
    command_args = {
        "executable": executable,
        "size": size,
        "iterations": iterations,
        "processes": processes,
        "mpirun": mpirun,
    }

    for _ in range(warmups):
        run_simulation(**command_args)

    times = []

    for _ in range(repeats):
        times.append(run_simulation(**command_args))

    return times


def main():
    parser = argparse.ArgumentParser(
        description="Compare serial and MPI heat diffusion performance."
    )
    parser.add_argument(
        "--serial-executable",
        type=Path,
        default=Path("build-release/heat_sim"),
    )
    parser.add_argument(
        "--mpi-executable",
        type=Path,
        default=Path("build-release/heat_sim_mpi"),
    )
    parser.add_argument("--mpirun", default="mpirun")
    parser.add_argument(
        "--sizes",
        nargs="+",
        type=int,
        default=[250, 500, 750, 1000],
    )
    parser.add_argument("--iterations", type=int, default=100)
    parser.add_argument("--processes", nargs="+", type=int, default=[1, 2, 4])
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("benchmark_comparison.csv"),
    )
    args = parser.parse_args()

    serial_executable = args.serial_executable.resolve()
    mpi_executable = args.mpi_executable.resolve()

    if not serial_executable.is_file():
        parser.error(f"Serial executable not found: {serial_executable}")

    if not mpi_executable.is_file():
        parser.error(f"MPI executable not found: {mpi_executable}")

    if any(size < 12 for size in args.sizes):
        parser.error("Grid sizes must be at least 12.")

    if args.iterations < 1 or args.repeats < 1 or args.warmups < 0:
        parser.error("Iterations and repeats must be positive; warmups non-negative.")

    if not args.processes or any(p < 1 for p in args.processes):
        parser.error("Process counts must be positive.")

    raw_rows = []
    medians = {}

    for size in args.sizes:
        configurations = [("serial", 1, serial_executable, None)]
        configurations.extend(
            ("mpi", p, mpi_executable, p) for p in args.processes
        )

        for variant, processes, executable, mpi_processes in configurations:
            print(f"{variant.upper()}, grid={size}x{size}, processes={processes}")

            times = measure(
                executable,
                size,
                args.iterations,
                args.repeats,
                args.warmups,
                processes=mpi_processes,
                mpirun=args.mpirun,
            )

            medians[(variant, processes, size)] = statistics.median(times)

            for repeat, seconds in enumerate(times, start=1):
                raw_rows.append({
                    "variant": variant,
                    "processes": processes,
                    "grid_size": size,
                    "iterations": args.iterations,
                    "repeat": repeat,
                    "seconds": f"{seconds:.9f}",
                })

            print(
                f"  median={statistics.median(times):.6f}s, "
                f"min={min(times):.6f}s, max={max(times):.6f}s"
            )

        serial_time = medians[("serial", 1, size)]

        for processes in args.processes:
            mpi_time = medians[("mpi", processes, size)]
            speedup = serial_time / mpi_time
            efficiency = speedup / processes

            print(
                f"  MPI {processes}: speedup={speedup:.2f}x, "
                f"efficiency={efficiency:.1%}"
            )

    args.output.parent.mkdir(parents=True, exist_ok=True)

    with args.output.open("w", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(
            file,
            fieldnames=[
                "variant", "processes", "grid_size",
                "iterations", "repeat", "seconds",
            ],
        )
        writer.writeheader()
        writer.writerows(raw_rows)

    print(f"\nRaw measurements saved to: {args.output}")


if __name__ == "__main__":
    main()