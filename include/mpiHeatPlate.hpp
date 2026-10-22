//
// Created by Lukas on 20.10.2026.
//

#ifndef HEAT_DIFFUSION_MPIHEATPLATE_HPP
#define HEAT_DIFFUSION_MPIHEATPLATE_HPP
#pragma once

#include <mpi.h>

#include <cstddef>
#include <vector>

class MpiHeatPlate {
public:
    MpiHeatPlate(
        std::size_t nx,
        std::size_t ny,
        double alpha,
        double initialTemp,
        std::size_t initialSize,
        MPI_Comm comm = MPI_COMM_WORLD
    );

    void step();
    void run(std::size_t iterations);

    [[nodiscard]] std::vector<double> gather(int root = 0) const;

    [[nodiscard]] std::size_t nx() const noexcept;
    [[nodiscard]] std::size_t ny() const noexcept;

private:
    std::size_t nx_;
    std::size_t ny_;
    double alpha_;

    MPI_Comm comm_;
    int rank_;
    int ranks_;

    std::size_t firstGlobalRow_;
    std::size_t localRows_;

    std::vector<double> current_;
    std::vector<double> next_;

    [[nodiscard]] std::size_t index(
        std::size_t localY,
        std::size_t x
    ) const noexcept;

    void initialize(double initialTemp, std::size_t initialSize);
};
#endif //HEAT_DIFFUSION_MPIHEATPLATE_HPP
