//
// Created by Lukas on 19.10.2026.
//

#include "../include/heatPlate.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>


#include <limits>



HeatPlate::HeatPlate(
    std::size_t nx,
    std::size_t ny,
    double alpha,
    double initialTemp,
    std::size_t initialSize
)
    : nx_(nx),
      ny_(ny),
      alpha_(alpha)
{
    if (nx < 3 || ny < 3) {
        throw std::invalid_argument(
            "Grid dimensions must be at least 3."
        );
    }

    if (!std::isfinite(alpha) || alpha < 0.0 || alpha > 0.25) {
        throw std::invalid_argument(
            "Alpha must be in the range [0, 0.25]."
        );
    }

    if (!std::isfinite(initialTemp)) {
        throw std::invalid_argument(
            "Initial temperature must be finite."
        );
    }

    if (initialSize > nx - 2 || initialSize > ny - 2) {
        throw std::invalid_argument(
            "Hot region must fit inside the grid boundaries."
        );
    }

    // Check for size_t overflow
    if (nx > std::numeric_limits<std::size_t>::max() / ny) {
        throw std::length_error("Grid dimensions are too large.");
    }

    const std::size_t cellCount = nx * ny;

    current_.assign(cellCount, 0.0);
    next_.assign(cellCount, 0.0);

    initialize(initialTemp, initialSize);
}

std::size_t HeatPlate::index(
    std::size_t x,
    std::size_t y
) const noexcept
{
    return y * nx_ + x;
}

void HeatPlate::initialize(
    double initialTemp,
    std::size_t initialSize
)
{
    if (initialSize == 0) {
        return;
    }

    const std::size_t startX = (nx_ - initialSize) / 2;
    const std::size_t startY = (ny_ - initialSize) / 2;

    for (std::size_t y = startY; y < startY + initialSize; ++y) {
        for (std::size_t x = startX; x < startX + initialSize; ++x) {
            current_[index(x, y)] = initialTemp;
        }
    }
}

void HeatPlate::step()
{

    std::fill(next_.begin(), next_.end(), 0.0);

    for (std::size_t y = 1; y < ny_ - 1; ++y) {
        for (std::size_t x = 1; x < nx_ - 1; ++x) {
            const std::size_t i = index(x, y);

            const double center = current_[i];
            const double north  = current_[index(x, y - 1)];
            const double south  = current_[index(x, y + 1)];
            const double west   = current_[index(x - 1, y)];
            const double east   = current_[index(x + 1, y)];

            next_[i] = center + alpha_ *
                (north + south + east + west - 4.0 * center);
        }
    }

    current_.swap(next_);
}

void HeatPlate::run(std::size_t iterations)
{
    for (std::size_t i = 0; i < iterations; ++i) {
        step();
    }
}

double HeatPlate::temperature(
    std::size_t x,
    std::size_t y
) const
{
    if (x >= nx_ || y >= ny_) {
        throw std::out_of_range("Grid coordinate out of range.");
    }

    return current_[index(x, y)];
}

std::size_t HeatPlate::nx() const noexcept
{
    return nx_;
}

std::size_t HeatPlate::ny() const noexcept
{
    return ny_;
}