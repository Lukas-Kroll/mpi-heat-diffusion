# MPI Heat Diffusion

A C++20 implementation of a two-dimensional heat-diffusion simulation with both a serial solver and an MPI-parallel solver. The project includes automated tests, CSV output, numerical result comparison, performance benchmarks and Python visualisation tools.

## Features

- Serial heat-diffusion solver.
- MPI solver that distributes grid rows among processes and exchanges boundary (ghost) rows.
- Configurable grid dimensions, iteration count, diffusion coefficient and initial temperature.
- CSV output for result visualisation and numerical comparison.
- Automated tests with CTest, including MPI runs with 1, 2 and 4 processes.
- Repeated performance benchmarks with median runtime, speedup and parallel efficiency.
- GitHub Actions continuous integration for building and testing.

## Requirements

On Linux or WSL (Ubuntu), install:

- A C++ compiler with C++20 support, such as `g++`
- CMake 3.20 or newer
- OpenMPI
- Python 3 and `pip` for the visualisation and benchmarking tools

Install the C++ build dependencies:

```bash
sudo apt update
sudo apt install --no-install-recommends -y cmake g++ openmpi-bin libopenmpi-dev
```

## Build

From the repository root:

```bash
cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON

cmake --build build-release --parallel 2
```

This builds the serial executable `heat_sim` and the MPI executable `heat_sim_mpi`.

## Run the simulation

### Serial version

Run the default configuration:

```bash
./build-release/heat_sim
```

Run a 500 × 500 simulation and save the final grid to CSV:

```bash
./build-release/heat_sim \
  -n 100 -x 500 -y 500 \
  -a 0.20 -t 100 -s 10 \
  --output serial_results.csv
```

### MPI version

Run the simulation with four MPI processes:

```bash
mpirun -np 4 ./build-release/heat_sim_mpi \
  -n 100 -x 500 -y 500 \
  -a 0.20 -t 100 -s 10 \
  --output mpi_results_4.csv
```

Only rank 0 writes the gathered final grid to the specified CSV file.

### Command-line options

| Option | Meaning | Default |
|---|---|---:|
| `-n <iterations>` | Number of simulation time steps | `100` |
| `-x <width>` | Grid width | `1000` |
| `-y <height>` | Grid height | `1000` |
| `-a <alpha>` | Diffusion coefficient, from `0.0` to `0.25` | `0.20` |
| `-t <temperature>` | Initial hot-region temperature | `100` |
| `-s <size>` | Side length of the initial hot region | `10` |
| `-o <file>`, `--output <file>` | Write the final grid to CSV | Disabled |
| `--help` | Print available options | — |

Grid dimensions must be at least 3, and the initial hot region must fit inside the grid.

## Tests

Run the tests after building:

```bash
OMPI_MCA_rmaps_base_oversubscribe=1 \
  ctest --test-dir build-release --output-on-failure
```

The test suite contains one serial test and MPI tests using 1, 2 and 4 processes.

## Verify serial and MPI results

Run both versions with identical simulation parameters:

```bash
./build-release/heat_sim \
  -n 100 -x 500 -y 500 \
  --output serial_results.csv

mpirun -np 4 ./build-release/heat_sim_mpi \
  -n 100 -x 500 -y 500 \
  --output mpi_results_4.csv
```

Install the Python dependencies and activate the virtual environment as described below. Then run:

```bash
python python/compareResults.py
```

The comparison script reports the maximum absolute difference and checks whether the arrays match within a numerical tolerance. Small floating-point differences can occur, so bit-for-bit equality is not required.

## Python environment

The Python tools use NumPy and Matplotlib. From the repository root:

```bash
python3 -m venv .venv
source .venv/bin/activate

python -m pip install --upgrade pip
python -m pip install -r python/requirements.txt
```

If your virtual environment is already active and the dependencies are installed, you do not need to repeat these steps.

## Performance benchmarks

Build the Release executables before benchmarking.

### Serial benchmark

Run the serial solver for several grid sizes:

```bash
python scripts/benchmark.py \
  --executable build-release/heat_sim \
  --sizes 250 500 750 1000 \
  --iterations 100 \
  --repeats 3 \
  --warmups 1 \
  --output results/benchmark_serial.csv
```

### Serial vs. MPI comparison

Run repeated measurements for 1, 2 and 4 MPI processes:

```bash
python scripts/benchmark_compare.py \
  --serial-executable build-release/heat_sim \
  --mpi-executable build-release/heat_sim_mpi \
  --sizes 250 500 750 1000 \
  --iterations 500 \
  --processes 1 2 4 \
  --repeats 10 \
  --warmups 2 \
  --output results/benchmark_comparison_robust.csv
```

The script saves raw measurements to CSV and reports the median, minimum and maximum runtime for each configuration.

For reliable comparisons, use a Release build, keep the run conditions consistent and avoid competing CPU-intensive workloads during measurement. Runtime depends on hardware, available CPU cores, process placement, system load, compiler and build settings.

## Generating benchmark graphs

The comparison CSV can be plotted with the following script:

```bash
python python/plot_benchmark_comparison.py \
  results/benchmark_comparison_robust.csv \
  --out-dir results
```

The script generates two images:

- `results/runtime_comparison.png`: median runtime for the serial solver and MPI runs, with the observed minimum-to-maximum range.
- `results/speedup_comparison.png`: measured MPI speedup compared with the ideal speedup.

The speedup is calculated as:

\[
S_p = \frac{T_{\mathrm{serial}}}{T_{\mathrm{MPI},p}}
\]

where \(T_{\mathrm{serial}}\) is the serial median runtime and \(T_{\mathrm{MPI},p}\) is the MPI median runtime using \(p\) processes.

The parallel efficiency is:

\[
E_p = \frac{S_p}{p}
\]

### Recorded benchmark results

The following measurements were collected in a local WSL environment using a Release build, 500 iterations per simulation, 2 warm-up runs and 10 measured repetitions. Values are medians; individual runs varied, so these results describe this environment rather than guaranteeing the same speedup on other machines.

| Grid | Serial median | MPI 1 median | MPI 2 median | MPI 4 median | Speedup with 4 processes |
|---|---:|---:|---:|---:|---:|
| 250 × 250 | 47.943 ms | 48.345 ms | 23.883 ms | 12.486 ms | 3.84× |
| 500 × 500 | 158.752 ms | 171.574 ms | 79.850 ms | 43.815 ms | 3.62× |
| 750 × 750 | 361.845 ms | 349.715 ms | 184.594 ms | 92.608 ms | 3.91× |
| 1000 × 1000 | 767.644 ms | 704.011 ms | 367.988 ms | 194.175 ms | 3.95× |

![Runtime comparison](results/runtime_comparison.png)

![MPI speedup by grid size](results/speedup_comparison.png)

For these measurements, the four-process MPI run was approximately 3.6–4.0 times as fast as the serial run. The two-process run was approximately twice as fast. MPI with one process was not consistently faster than the serial executable, which is possible because MPI introduces additional overhead.

Some minimum/maximum measurements showed substantial variation, and calculated parallel efficiency was slightly above 100% for a few configurations. These values should not be interpreted as a guarantee of superlinear scaling; cache effects, system scheduling and measurement noise can affect comparisons between separate serial and MPI runs. Medians are reported instead of best-case times.

## Project layout

```text
.
├── .github/workflows/   # GitHub Actions build-and-test workflow
├── include/             # Public C++ headers
├── python/              # Visualisation and result-analysis tools
├── scripts/             # Benchmark scripts
├── src/                 # Serial and MPI implementations and entry points
├── tests/               # Serial and MPI tests
├── results/              # Benchmark CSV files and generated plots
└── CMakeLists.txt        # Build targets and CTest registration
```

## Implementation notes

The grid is updated iteratively using a five-point finite-difference stencil. The MPI implementation distributes rows across processes and exchanges neighbouring boundary rows so each process can update its local subdomain. The final grid can be gathered to rank 0 and written to CSV.

## Continuous integration

GitHub Actions configures the project with CMake, builds the C++ targets and runs CTest on Ubuntu with OpenMPI. Automated tests help catch build and correctness regressions when the code changes.
