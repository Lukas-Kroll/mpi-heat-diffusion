//
// Created by Lukas on 19.10.2026.
//
#include "../include/heatPlate.hpp"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
int failures = 0;

void check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void expectNear(double actual, double expected, const char* message)
{
    constexpr double tolerance = 1e-12;

    check(
        std::isfinite(actual) &&
            std::abs(actual - expected) <= tolerance,
        message
    );
}

template <typename Exception, typename Function>
void expectThrows(Function function, const char* message)
{
    bool caughtExpectedException = false;

    try {
        function();
    } catch (const Exception&) {
        caughtExpectedException = true;
    } catch (...) {
        // A different exception type was thrown.
    }

    check(caughtExpectedException, message);
}

void testInitialization()
{
    HeatPlate plate(7, 7, 0.20, 100.0, 1);

    expectNear(plate.temperature(3, 3), 100.0,
               "The initial center should be hot");
    expectNear(plate.temperature(2, 3), 0.0,
               "A neighbor should initially be cold");
    expectNear(plate.temperature(0, 3), 0.0,
               "The boundary should initially be zero");
}

void testOneIteration()
{
    HeatPlate plate(7, 7, 0.20, 100.0, 1);
    plate.step();

    expectNear(plate.temperature(3, 3), 20.0,
               "Center after one iteration");

    expectNear(plate.temperature(2, 3), 20.0,
               "Left neighbor after one iteration");
    expectNear(plate.temperature(4, 3), 20.0,
               "Right neighbor after one iteration");
    expectNear(plate.temperature(3, 2), 20.0,
               "Upper neighbor after one iteration");
    expectNear(plate.temperature(3, 4), 20.0,
               "Lower neighbor after one iteration");

    expectNear(plate.temperature(2, 2), 0.0,
               "Diagonal should remain cold after one iteration");
}

void testZeroIterations()
{
    HeatPlate plate(7, 7, 0.20, 100.0, 1);
    plate.run(0);

    expectNear(plate.temperature(3, 3), 100.0,
               "Zero iterations should preserve the center");
    expectNear(plate.temperature(2, 3), 0.0,
               "Zero iterations should preserve its neighbor");
}

void testFixedBoundaries()
{
    HeatPlate plate(7, 7, 0.20, 100.0, 1);
    plate.run(10);

    for (std::size_t i = 0; i < 7; ++i) {
        expectNear(plate.temperature(i, 0), 0.0,
                   "Top boundary should remain zero");
        expectNear(plate.temperature(i, 6), 0.0,
                   "Bottom boundary should remain zero");
        expectNear(plate.temperature(0, i), 0.0,
                   "Left boundary should remain zero");
        expectNear(plate.temperature(6, i), 0.0,
                   "Right boundary should remain zero");
    }
}

void testInvalidInputs()
{
    expectThrows<std::invalid_argument>(
        [] { HeatPlate plate(2, 7, 0.20, 100.0, 1); },
        "Reject a grid dimension smaller than 3"
    );

    expectThrows<std::invalid_argument>(
        [] { HeatPlate plate(7, 7, 0.30, 100.0, 1); },
        "Reject an unstable alpha value"
    );

    expectThrows<std::invalid_argument>(
        [] {
            HeatPlate plate(
                7, 7, 0.20,
                std::numeric_limits<double>::infinity(), 1
            );
        },
        "Reject a non-finite initial temperature"
    );

    expectThrows<std::invalid_argument>(
        [] { HeatPlate plate(7, 7, 0.20, 100.0, 6); },
        "Reject a hot region that does not fit"
    );

    expectThrows<std::out_of_range>(
        [] {
            HeatPlate plate(7, 7, 0.20, 100.0, 1);
            (void)plate.temperature(7, 3);
        },
        "Reject an out-of-range coordinate"
    );

    expectThrows<std::length_error>(
        [] {
            const auto max =
                std::numeric_limits<std::size_t>::max();
            HeatPlate plate(max, 3, 0.20, 100.0, 1);
        },
        "Reject grid dimensions that overflow during multiplication"
    );
}
} // namespace

int main()
{
    testInitialization();
    testOneIteration();
    testZeroIterations();
    testFixedBoundaries();
    testInvalidInputs();

    if (failures != 0) {
        std::cerr << failures << " test assertion(s) failed.\n";
        return 1;
    }

    std::cout << "All HeatPlate tests passed.\n";
    return 0;
}