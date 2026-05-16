// Conway's Game of Life - headless CLI benchmark
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#include "Common.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

struct Options {
    int iterations = 10000;
    int warmup = 10;
    bool addNoise = false;
    int initialNoise = (GRID_SIZE * GRID_SIZE) / 4;
};

void printUsage(const char* prog)
{
    std::cout << "Usage: " << prog << " [options]\n"
              << "Options:\n"
              << "  -i, --iterations N    Number of generations to simulate (default: 10000)\n"
              << "  -w, --warmup N        Warmup generations excluded from timing (default: 10)\n"
              << "  -n, --noise N         Number of initial random cells to toggle (default: GRID_SIZE^2 / 4)\n"
              << "      --add-noise       Add one random toggle per generation (matches GUI behavior)\n"
              << "  -h, --help            Show this help and exit\n";
}

bool parseArgs(int argc, char** argv, Options& opts)
{
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto needsValue = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << name << "\n";
                std::exit(1);
            }
            return argv[++i];
        };
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            std::exit(0);
        } else if (arg == "-i" || arg == "--iterations") {
            opts.iterations = std::atoi(needsValue("--iterations"));
        } else if (arg == "-w" || arg == "--warmup") {
            opts.warmup = std::atoi(needsValue("--warmup"));
        } else if (arg == "-n" || arg == "--noise") {
            opts.initialNoise = std::atoi(needsValue("--noise"));
        } else if (arg == "--add-noise") {
            opts.addNoise = true;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            printUsage(argv[0]);
            return false;
        }
    }
    if (opts.iterations <= 0) {
        std::cerr << "iterations must be > 0\n";
        return false;
    }
    if (opts.warmup < 0) {
        opts.warmup = 0;
    }
    return true;
}

int countAlive(const Grid<GRID_SIZE>& g)
{
    int alive = 0;
    for (int x = 0; x < GRID_SIZE; ++x) {
        for (int y = 0; y < GRID_SIZE; ++y) {
            alive += g.get({ x, y }) ? 1 : 0;
        }
    }
    return alive;
}

} // namespace

int main(int argc, char** argv)
{
    Options opts;
    if (!parseArgs(argc, argv, opts)) {
        return 1;
    }

    printAppInfo();
    std::cout << "Mode: headless benchmark\n"
              << "Iterations: " << opts.iterations << " (warmup: " << opts.warmup << ")\n"
              << "Initial noise toggles: " << opts.initialNoise << "\n"
              << "Per-step noise: " << (opts.addNoise ? "on" : "off") << "\n";
    std::cout.flush();

    // Ping-pong between two raw grids — no DoubleBuffer locking overhead for the benchmark.
    Grid<GRID_SIZE> a;
    Grid<GRID_SIZE> b;
    a.clear();
    b.clear();
    a.addNoise(opts.initialNoise);

    Grid<GRID_SIZE>* curr = &a;
    Grid<GRID_SIZE>* next = &b;

    using clock = std::chrono::steady_clock;

    for (int i = 0; i < opts.warmup; ++i) {
        next->updateGrid(*curr);
        if (opts.addNoise) {
            next->addNoise();
        }
        std::swap(curr, next);
    }

    const auto t0 = clock::now();
    for (int i = 0; i < opts.iterations; ++i) {
        next->updateGrid(*curr);
        if (opts.addNoise) {
            next->addNoise();
        }
        std::swap(curr, next);
    }
    const auto t1 = clock::now();

    const double seconds = std::chrono::duration<double>(t1 - t0).count();
    const double eps = opts.iterations / seconds;
    const double cellsPerIter = static_cast<double>(GRID_SIZE) * GRID_SIZE;
    const double cups = eps * cellsPerIter;
    const int finalAlive = countAlive(*curr);

    std::cout << "\nResults\n"
              << "  Elapsed:        " << seconds << " s\n"
              << "  Generations/s:  " << eps << "\n"
              << "  Cells updated/s:" << cups << " (" << cups / 1e9 << " GCUpS)\n"
              << "  ns / cell:      " << (seconds * 1e9) / (opts.iterations * cellsPerIter) << "\n"
              << "  Final alive:    " << finalAlive << " / " << static_cast<long long>(cellsPerIter) << "\n";

    return 0;
}
