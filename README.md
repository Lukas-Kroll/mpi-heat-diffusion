
# MPI Heat Diffusion

A C++20 implementation of a two-dimensional heat-diffusion simulation with both a serial solver and an MPI-parallel solver. The project includes automated tests, numerical result comparison, performance benchmarks and Python visualisation tools.

## Features

- Serial heat-diffusion solver.
- MPI solver that distributes grid rows among processes and exchanges boundary (ghost) rows.
- Configurable grid dimensions, iteration count, diffusion coefficient and initial temperature.
- CSV output for result visualisation and numerical comparison.
- Automated tests with CTest, including MPI runs with 1, 2 and 4 processes.
- Repeated performance benchmarks with median runtime, speedup and parallel efficiency.
- Python tools for heatmap visualisation and benchmark plots.
- GitHub Actions continuous integration for building and testing.

## Requirements

This project is intended to run on Linux or WSL (Ubuntu).

Required tools:

- A C++ compiler with C++20 support, such as `g++`
- CMake 3.20 or newer
- OpenMPI
- Python 3 and `pip`

Install the C++ dependencies:

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

This builds two executables:

- `heat_sim` — serial solver
- `heat_sim_mpi` — MPI-parallel solver

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

The MPI solver distributes the grid across processes and exchanges neighbouring boundary rows. The final grid is gathered on rank 0, which writes the CSV output.

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

Grid dimensions must be at least 3. The initial hot region must fit inside the grid.

## Tests

Run the test suite after building:

```bash
OMPI_MCA_rmaps_base_oversubscribe=1 \
  ctest --test-dir build-release --output-on-failure
```

The test suite includes a serial test and MPI tests using 1, 2 and 4 processes.

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

Install the Python dependencies as described in the next section, then run the comparison script:

```bash
python python/compareResults.py
```

The comparison reports the maximum absolute difference between the results and checks numerical agreement within a tolerance. Small floating-point differences can occur, so bit-for-bit equality is not required.

## Python environment

The Python tools use NumPy and Matplotlib. From the repository root:

```bash
python3 -m venv .venv
source .venv/bin/activate

python -m pip install --upgrade pip
python -m pip install -r python/requirements.txt
```

If your virtual environment is already active and the dependencies are installed, you can skip these steps.

## Heatmap visualisation

The heatmap illustrates the final temperature distribution after 2000 simulation iterations on a 250 × 250 grid.

Generate the CSV output:

```bash
mkdir -p results

./build-release/heat_sim \
  -n 2000 -x 250 -y 250 \
  -a 0.20 -t 100 -s 10 \
  --output results/heatmap_input.csv
```

Generate the heatmap:

```bash
python python/plot_heatmap.py \
  results/heatmap_input.csv \
  --save results/heatmap.png
```

![Heat diffusion heatmap](results/heatmap.png)

The temperature distribution illustrates how heat spreads from the initial hot region over time. The boundaries remain at zero temperature.

## Performance benchmarks

The benchmark scripts compare the serial solver with MPI runs using different process counts.

Build the Release executables before benchmarking.

### Serial benchmark

Run the serial solver for several grid sizes:

```bash
mkdir -p results

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

The comparison script saves raw measurements to CSV and reports the median, minimum and maximum runtime for each configuration.

For more reliable comparisons, use a Release build, keep the run conditions consistent and avoid competing CPU-intensive workloads. Runtime depends on hardware, available CPU cores, process placement, system load and compiler settings.

## Generating benchmark graphs

The comparison CSV can be visualised using the plotting script:

```bash
python python/plot_benchmark_comparison.py \
  results/benchmark_comparison_robust.csv \
  --out-dir results
```

The script generates two images:

- `results/runtime_comparison.png` — compares median runtime and the observed minimum-to-maximum range.
- `results/speedup_comparison.png` — compares measured MPI speedup with ideal speedup.

### Runtime comparison

![Runtime comparison](results/runtime_comparison.png)

### MPI speedup

![MPI speedup by grid size](results/speedup_comparison.png)

## Recorded benchmark results

The following measurements were collected in a local WSL environment using a Release build, 500 iterations per simulation, 2 warm-up runs and 10 measured repetitions. The table reports median runtimes.

| Grid | Serial median | MPI 1 median | MPI 2 median | MPI 4 median | Speedup with 4 processes |
|---|---:|---:|---:|---:|---:|
| 250 × 250 | 47.943 ms | 48.345 ms | 23.883 ms | 12.486 ms | 3.84× |
| 500 × 500 | 158.752 ms | 171.574 ms | 79.850 ms | 43.815 ms | 3.62× |
| 750 × 750 | 361.845 ms | 349.715 ms | 184.594 ms | 92.608 ms | 3.91× |
| 1000 × 1000 | 767.644 ms | 704.011 ms | 367.988 ms | 194.175 ms | 3.95× |

The speedup is calculated as:

$$
S_p = \frac{T_{\mathrm{serial}}}{T_{\mathrm{MPI},p}}
$$

where $T_{\mathrm{serial}}$ is the serial median runtime and $T_{\mathrm{MPI},p}$ is the MPI median runtime using $p$ processes.

Parallel efficiency is calculated as:

$$
E_p = \frac{S_p}{p}
$$

Speedup relative to the single-process MPI run (to separate parallel scaling from differences between the serial and MPI implementations) is:

$$
S_p^{(1)} = \frac{T_{\mathrm{MPI},1}}{T_{\mathrm{MPI},p}}
$$

### Results interpretation

In these measurements, the four-process MPI solver achieved a speedup of approximately 3.6× to 4.0×. The two-process version was approximately twice as fast as the serial solver.

MPI with one process was not consistently faster than the serial executable. This is expected to be possible because MPI introduces additional overhead.

Some individual measurements varied considerably, and calculated parallel efficiency was slightly above 100% for a few configurations. Such values are not a guarantee of superlinear scaling; cache effects, scheduling and measurement noise can influence the comparison. Median runtimes are therefore reported instead of best-case times.

The results describe the local test environment and should not be treated as guaranteed performance on other systems.

## Project structure

```text
.
├── .github/workflows/   # GitHub Actions workflow
├── include/             # Public C++ headers
├── python/              # Visualisation and analysis tools
├── scripts/             # Benchmark scripts
├── src/                 # Serial and MPI implementations
├── tests/               # Serial and MPI tests
├── results/             # Benchmark data and generated figures
└── CMakeLists.txt        # Build targets and CTest registration
```

## Implementation notes

The grid is updated iteratively using a five-point finite-difference stencil. The MPI implementation distributes rows across processes and exchanges neighbouring boundary rows so that each process can update its local subdomain. The final grid can be gathered on rank 0 and written to CSV.

## Continuous integration

GitHub Actions configures the project with CMake, builds the C++ targets and runs CTest on Ubuntu with OpenMPI. This helps catch build and correctness regressions when the code changes.
