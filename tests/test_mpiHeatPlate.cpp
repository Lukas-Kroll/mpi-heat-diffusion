//
// Created by Lukas on 20.10.2026.
//
#include "heatPlate.hpp"
#include "mpiHeatPlate.hpp"

#include <mpi.h>

#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

bool compareCase(
    int rank,
    std::size_t nx,
    std::size_t ny,
    double alpha,
    double initialTemp,
    std::size_t initialSize,
    std::size_t iterations
)
{
    MpiHeatPlate parallel(
        nx, ny, alpha, initialTemp, initialSize
    );

    parallel.run(iterations);
    const std::vector<double> distributed = parallel.gather(0);

    int passed = 1;

    if (rank == 0) {
        HeatPlate serial(
            nx, ny, alpha, initialTemp, initialSize
        );

        serial.run(iterations);

        constexpr double tolerance = 1e-12;

        for (std::size_t y = 0; y < ny && passed; ++y) {
            for (std::size_t x = 0; x < nx; ++x) {
                const double expected = serial.temperature(x, y);
                const double actual = distributed[y * nx + x];

                if (!std::isfinite(actual) ||
                    std::abs(actual - expected) > tolerance) {
                    std::cerr
                        << "Mismatch at (" << x << ", " << y << ")"
                        << ": serial=" << expected
                        << ", MPI=" << actual << '\n';

                    passed = 0;
                    break;
                }
            }
        }

        if (passed) {
            std::cout
                << "PASS: " << nx << "x" << ny
                << ", iterations=" << iterations << '\n';
        }
    }

    MPI_Bcast(&passed, 1, MPI_INT, 0, MPI_COMM_WORLD);

    return passed == 1;
}

int main(int argc, char* argv[])
{
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        return 1;
    }

    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    bool passed = true;

    if (!compareCase(rank, 101, 97, 0.20, 87.5, 9, 23)) {
        passed = false;
    }

    if (!compareCase(rank, 17, 13, 0.20, 100.0, 3, 1)) {
        passed = false;
    }

    if (!compareCase(rank, 18, 11, 0.20, 100.0, 1, 0)) {
        passed = false;
    }

    if (rank == 0 && passed) {
        std::cout << "All MPI correctness tests passed.\n";
    }

    MPI_Finalize();

    return passed ? 0 : 1;
}