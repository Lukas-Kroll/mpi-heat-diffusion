//
// Created by Lukas on 19.10.2026.
//
#include "mpiHeatPlate.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

MpiHeatPlate::MpiHeatPlate(
    std::size_t nx,
    std::size_t ny,
    double alpha,
    double initialTemp,
    std::size_t initialSize,
    MPI_Comm comm
)
    : nx_(nx),
      ny_(ny),
      alpha_(alpha),
      comm_(comm),
      rank_(0),
      ranks_(1),
      firstGlobalRow_(0),
      localRows_(0)
{
    int initialized = 0;
    MPI_Initialized(&initialized);

    if (initialized == 0) {
        throw std::logic_error("MPI must be initialized before constructing MpiHeatPlate.");
    }

    if (nx_ < 3 || ny_ < 3) {
        throw std::invalid_argument("Grid dimensions must be at least 3.");
    }

    if (!std::isfinite(alpha_) || alpha_ < 0.0 || alpha_ > 0.25) {
        throw std::invalid_argument("Alpha must be in the range [0, 0.25].");
    }

    if (!std::isfinite(initialTemp)) {
        throw std::invalid_argument("Initial temperature must be finite.");
    }

    if (initialSize > nx_ - 2 || initialSize > ny_ - 2) {
        throw std::invalid_argument("Hot region must fit inside the grid boundaries.");
    }

    if (nx_ > std::numeric_limits<std::size_t>::max() / ny_) {
        throw std::length_error("Grid dimensions overflow size_t.");
    }

    const std::size_t totalCells = nx_ * ny_;

    if (totalCells > static_cast<std::size_t>(
            std::numeric_limits<int>::max())) {
        throw std::length_error(
            "This implementation supports at most INT_MAX grid cells."
        );
    }

    MPI_Comm_rank(comm_, &rank_);
    MPI_Comm_size(comm_, &ranks_);

    if (static_cast<std::size_t>(ranks_) > ny_) {
        throw std::invalid_argument(
            "The number of MPI processes cannot exceed the grid height."
        );
    }

    const std::size_t processCount = static_cast<std::size_t>(ranks_);
    const std::size_t processRank = static_cast<std::size_t>(rank_);
    const std::size_t baseRows = ny_ / processCount;
    const std::size_t extraRows = ny_ % processCount;

    localRows_ = baseRows + (processRank < extraRows ? 1 : 0);
    firstGlobalRow_ = processRank * baseRows
                    + std::min(processRank, extraRows);

    current_.assign((localRows_ + 2) * nx_, 0.0);
    next_.assign((localRows_ + 2) * nx_, 0.0);

    initialize(initialTemp, initialSize);
}

std::size_t MpiHeatPlate::index(
    std::size_t localY,
    std::size_t x
) const noexcept
{
    return localY * nx_ + x;
}

void MpiHeatPlate::initialize(
    double initialTemp,
    std::size_t initialSize
)
{
    if (initialSize == 0) {
        return;
    }

    const std::size_t startX = (nx_ - initialSize) / 2;
    const std::size_t startY = (ny_ - initialSize) / 2;

    for (std::size_t localY = 1; localY <= localRows_; ++localY) {
        const std::size_t globalY = firstGlobalRow_ + localY - 1;

        if (globalY < startY || globalY >= startY + initialSize) {
            continue;
        }

        for (std::size_t x = startX; x < startX + initialSize; ++x) {
            current_[index(localY, x)] = initialTemp;
        }
    }
}

void MpiHeatPlate::step()
{
    const int up = rank_ > 0 ? rank_ - 1 : MPI_PROC_NULL;
    const int down = rank_ + 1 < ranks_ ? rank_ + 1 : MPI_PROC_NULL;
    const int rowWidth = static_cast<int>(nx_);

    MPI_Sendrecv(
        current_.data() + index(1, 0),
        rowWidth,
        MPI_DOUBLE,
        up,
        0,
        current_.data() + index(localRows_ + 1, 0),
        rowWidth,
        MPI_DOUBLE,
        down,
        0,
        comm_,
        MPI_STATUS_IGNORE
    );

    MPI_Sendrecv(
        current_.data() + index(localRows_, 0),
        rowWidth,
        MPI_DOUBLE,
        down,
        1,
        current_.data(),
        rowWidth,
        MPI_DOUBLE,
        up,
        1,
        comm_,
        MPI_STATUS_IGNORE
    );

    std::fill(next_.begin(), next_.end(), 0.0);

    for (std::size_t localY = 1; localY <= localRows_; ++localY) {
        const std::size_t globalY = firstGlobalRow_ + localY - 1;

        if (globalY == 0 || globalY == ny_ - 1) {
            continue;
        }

        for (std::size_t x = 1; x < nx_ - 1; ++x) {
            const double center = current_[index(localY, x)];
            const double north = current_[index(localY - 1, x)];
            const double south = current_[index(localY + 1, x)];
            const double west = current_[index(localY, x - 1)];
            const double east = current_[index(localY, x + 1)];

            next_[index(localY, x)] = center + alpha_ *
                (north + south + west + east - 4.0 * center);
        }
    }

    current_.swap(next_);
}

void MpiHeatPlate::run(std::size_t iterations)
{
    for (std::size_t i = 0; i < iterations; ++i) {
        step();
    }
}

std::vector<double> MpiHeatPlate::gather(int root) const
{
    if (root < 0 || root >= ranks_) {
        throw std::invalid_argument("Invalid gather root rank.");
    }

    std::vector<int> counts(static_cast<std::size_t>(ranks_));
    std::vector<int> displacements(static_cast<std::size_t>(ranks_));

    const std::size_t processCount = static_cast<std::size_t>(ranks_);
    const std::size_t baseRows = ny_ / processCount;
    const std::size_t extraRows = ny_ % processCount;

    for (int r = 0; r < ranks_; ++r) {
        const std::size_t processRank = static_cast<std::size_t>(r);
        const std::size_t rows =
            baseRows + (processRank < extraRows ? 1 : 0);
        const std::size_t firstRow =
            processRank * baseRows + std::min(processRank, extraRows);

        counts[processRank] = static_cast<int>(rows * nx_);
        displacements[processRank] = static_cast<int>(firstRow * nx_);
    }

    std::vector<double> global;

    if (rank_ == root) {
        global.resize(nx_ * ny_);
    }

    MPI_Gatherv(
        current_.data() + nx_,
        static_cast<int>(localRows_ * nx_),
        MPI_DOUBLE,
        rank_ == root ? global.data() : nullptr,
        counts.data(),
        displacements.data(),
        MPI_DOUBLE,
        root,
        comm_
    );

    return global;
}

std::size_t MpiHeatPlate::nx() const noexcept
{
    return nx_;
}

std::size_t MpiHeatPlate::ny() const noexcept
{
    return ny_;
}