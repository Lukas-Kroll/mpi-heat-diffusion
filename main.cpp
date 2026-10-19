
#include <charconv>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

struct Config {
    std::size_t iterations = 100;
    std::size_t nx = 1000;
    std::size_t ny = 1000;
    double alpha = 0.20;
    double initialTemp = 100.0;
    std::size_t initialSize = 10;
};

template <typename T>
T parseInteger(const char* value, const char* name) {
    T result{};

    const std::string_view input(value);
    const auto [ptr, ec] = std::from_chars(
        input.data(), input.data() + input.size(), result
    );

    if (ec != std::errc{} || ptr != input.data() + input.size()) {
        throw std::invalid_argument(
            std::string("Invalid integer for ") + name + ": " + value
        );
    }

    return result;
}

double parseDouble(const char* value, const char* name) {
    std::size_t parsed = 0;
    const double result = std::stod(value, &parsed);

    if (parsed != std::string(value).size() ||
        !std::isfinite(result)) {
        throw std::invalid_argument(
            std::string("Invalid number for ") + name + ": " + value
        );
    }

    return result;
}

void printHelp(const char* program) {
    std::cout
        << "Usage: " << program << " [options]\n"
        << "  -n <iterations>   Number of time steps (default: 100)\n"
        << "  -x <width>        Grid width (default: 1000)\n"
        << "  -y <height>       Grid height (default: 1000)\n"
        << "  -a <alpha>        Diffusion coefficient (default: 0.20)\n"
        << "  -t <temperature>  Initial hot-region temperature (default: 100)\n"
        << "  -s <size>         Hot-region side length (default: 10)\n"
        << "  --help            Show this help\n";
}

Config parseArguments(int argc, char* argv[]) {
    Config config;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help") {
            printHelp(argv[0]);
            std::exit(0);
        }

        if (i + 1 >= argc) {
            throw std::invalid_argument("Missing value for " + arg);
        }

        const char* value = argv[++i];

        if (arg == "-n") {
            config.iterations =
                parseInteger<std::size_t>(value, "-n");
        } else if (arg == "-x") {
            config.nx = parseInteger<std::size_t>(value, "-x");
        } else if (arg == "-y") {
            config.ny = parseInteger<std::size_t>(value, "-y");
        } else if (arg == "-a") {
            config.alpha = parseDouble(value, "-a");
        } else if (arg == "-t") {
            config.initialTemp = parseDouble(value, "-t");
        } else if (arg == "-s") {
            config.initialSize =
                parseInteger<std::size_t>(value, "-s");
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }

    if (config.nx < 3 || config.ny < 3) {
        throw std::invalid_argument("Grid dimensions must be >= 3");
    }

    if (config.initialSize == 0 ||
        config.initialSize > config.nx - 2 ||
        config.initialSize > config.ny - 2) {
        throw std::invalid_argument(
            "Hot-region size must fit inside the grid"
        );
    }

    if (config.alpha < 0.0 || config.alpha > 0.25) {
        throw std::invalid_argument(
            "Alpha must be between 0.0 and 0.25 for this scheme"
        );
    }

    return config;
}

int main(int argc, char* argv[]) {
    try {
        const Config config = parseArguments(argc, argv);

        std::cout << "Grid: " << config.nx << " x " << config.ny << '\n'
                  << "Iterations: " << config.iterations << '\n'
                  << "Alpha: " << config.alpha << '\n'
                  << "Initial temperature: " << config.initialTemp << '\n'
                  << "Hot-region size: " << config.initialSize << '\n';


    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
