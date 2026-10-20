//
// Created by Lukas on 19.10.2026.
//

#ifndef MPI_HEAT_DIFFUSION_HEATPLATE_HPP
#define MPI_HEAT_DIFFUSION_HEATPLATE_HPP
#pragma once

#include <cstddef>
#include <vector>

class HeatPlate {
public:
    HeatPlate(
        std::size_t nx,
        std::size_t ny,
        double alpha,
        double initialTemp,
        std::size_t initialSize
    );

    void step();
    void run(std::size_t iterations);

    [[nodiscard]] double temperature(
        std::size_t x,
        std::size_t y
    ) const;

    [[nodiscard]] std::size_t nx() const noexcept;
    [[nodiscard]] std::size_t ny() const noexcept;

private:
    std::size_t nx_;
    std::size_t ny_;
    double alpha_;

    std::vector<double> current_;
    std::vector<double> next_;

    [[nodiscard]] std::size_t index(
        std::size_t x,
        std::size_t y
    ) const noexcept;

    void initialize(double initialTemp, std::size_t initialSize);
};


#endif //MPI_HEAT_DIFFUSION_HEATPLATE_HPP
