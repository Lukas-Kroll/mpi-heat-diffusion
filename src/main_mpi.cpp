//
// Created by Lukas on 20.10.2026.
//
#include "mpiHeatPlate.hpp"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct Config {
    std::size_t iterations = 100;
    std::size_t nx = 1000;
    std::size_t ny = 1000;
    double alpha = 0.20;
    double initialTemp = 100.0;
    std::size_t initialSize = 10;
    std::string outputPath;
    bool help = false;
};

template <typename T>
T parseInteger(const char* value, const char* name)
{
    T result{};
    const std::string_view input(value);

    const auto [ptr, ec] = std::from_chars(
        input.data(),
        input.data() + input.size(),
        result
    );

    if (ec != std::errc{} || ptr != input.data() + input.size()) {
        throw std::invalid_argument(
            std::string("Invalid integer for ") + name + ": " + value
        );
    }

    return result;
}

double parseDouble(const char* value, const char* name)
{
    std::size_t parsed = 0;
    double result = 0.0;

    try {
        result = std::stod(value, &parsed);
    } catch (const std::exception&) {
        throw std::invalid_argument(
            std::string("Invalid number for ") + name + ": " + value
        );
    }

    if (parsed != std::string(value).size() || !std::isfinite(result)) {
        throw std::invalid_argument(
            std::string("Invalid number for ") + name + ": " + value
        );
    }

    return result;
}

void printHelp(const char* program)
{
    std::cout
        << "Usage: " << program << " [options]\n"
        << "  -n <iterations>   Number of time steps (default: 100)\n"
        << "  -x <width>        Grid width (default: 1000)\n"
        << "  -y <height>       Grid height (default: 1000)\n"
        << "  -a <alpha>        Diffusion coefficient (default: 0.20)\n"
        << "  -t <temperature>  Initial hot-region temperature (default: 100)\n"
        << "  -s <size>         Hot-region side length (default: 10)\n"
        << "  -o, --output <file> Write final grid to CSV\n"
        << "  --help            Show this help\n";
}

Config parseArguments(int argc, char* argv[])
{
    Config config;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help") {
            config.help = true;
            return config;
        }

        const bool known =
            arg == "-n" || arg == "-x" || arg == "-y" ||
            arg == "-a" || arg == "-t" || arg == "-s" ||
            arg == "-o" || arg == "--output";

        if (!known) {
            throw std::invalid_argument("Unknown argument: " + arg);
        }

        if (i + 1 >= argc) {
            throw std::invalid_argument("Missing value for " + arg);
        }

        const char* value = argv[++i];

        if (arg == "-n") {
            config.iterations = parseInteger<std::size_t>(value, "-n");
        } else if (arg == "-x") {
            config.nx = parseInteger<std::size_t>(value, "-x");
        } else if (arg == "-y") {
            config.ny = parseInteger<std::size_t>(value, "-y");
        } else if (arg == "-a") {
            config.alpha = parseDouble(value, "-a");
        } else if (arg == "-t") {
            config.initialTemp = parseDouble(value, "-t");
        } else if (arg == "-s") {
            config.initialSize = parseInteger<std::size_t>(value, "-s");
        } else {
            config.outputPath = value;
        }
    }

    if (config.nx < 3 || config.ny < 3) {
        throw std::invalid_argument("Grid dimensions must be >= 3.");
    }

    if (config.initialSize == 0 ||
        config.initialSize > config.nx - 2 ||
        config.initialSize > config.ny - 2) {
        throw std::invalid_argument(
            "Hot-region size must fit inside the grid."
        );
    }

    if (config.alpha < 0.0 || config.alpha > 0.25) {
        throw std::invalid_argument(
            "Alpha must be between 0.0 and 0.25."
        );
    }

    return config;
}

void writeCsv(
    const std::vector<double>& grid,
    std::size_t nx,
    std::size_t ny,
    const std::string& path
)
{
    std::ofstream output(path);

    if (!output) {
        throw std::runtime_error("Could not open output file: " + path);
    }

    output << std::setprecision(
        std::numeric_limits<double>::max_digits10
    );

    for (std::size_t y = 0; y < ny; ++y) {
        for (std::size_t x = 0; x < nx; ++x) {
            if (x != 0) {
                output << ',';
            }

            output << grid[y * nx + x];
        }

        output << '\n';
    }

    if (!output) {
        throw std::runtime_error("Failed while writing output file: " + path);
    }
}

int main(int argc, char* argv[])
{
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        return 1;
    }

    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    try {
        const Config config = parseArguments(argc, argv);

        if (config.help) {
            if (rank == 0) {
                printHelp(argv[0]);
            }
        } else {
            MpiHeatPlate plate(
                config.nx,
                config.ny,
                config.alpha,
                config.initialTemp,
                config.initialSize
            );

            MPI_Barrier(MPI_COMM_WORLD);

            const double start = MPI_Wtime();
            plate.run(config.iterations);
            const double localElapsed = MPI_Wtime() - start;

            double elapsed = 0.0;

            MPI_Reduce(
                &localElapsed,
                &elapsed,
                1,
                MPI_DOUBLE,
                MPI_MAX,
                0,
                MPI_COMM_WORLD
            );

            if (rank == 0) {
                std::cout
                    << "Grid: " << config.nx << " x " << config.ny << '\n'
                    << "Iterations: " << config.iterations << '\n'
                    << "Alpha: " << config.alpha << '\n'
                    << "Initial temperature: " << config.initialTemp << '\n'
                    << "Hot-region size: " << config.initialSize << '\n'
                    << std::setprecision(12)
                    << "COMPUTE_TIME_SECONDS=" << elapsed << '\n';
            }

            if (!config.outputPath.empty()) {
                const std::vector<double> grid = plate.gather(0);

                if (rank == 0) {
                    writeCsv(grid, config.nx, config.ny, config.outputPath);
                    std::cout << "Wrote temperature grid to: "
                              << config.outputPath << '\n';
                }
            }
        }
    } catch (const std::exception& e) {
        if (rank == 0) {
            std::cerr << "Error: " << e.what() << '\n';
        }

        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    MPI_Finalize();
    return 0;
}