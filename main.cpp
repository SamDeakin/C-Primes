#include <climits>
#include <cstdlib>
#include <iostream>

#include "ThreadPool.hpp"

int main(int argc, const char** argv) {
    // TODO Increase these when the program is more stable
    uint64_t defaultStart = 2;
    uint64_t defaultEnd = 100;
    std::size_t defaultNumThreads = 1;

    // Override these for command line arguments
    for (int i = 1; i + 1 < argc; i = i + 2) {
        if (!strcmp(argv[i], "-s")) {
            uint64_t value = strtoull(argv[i + 1], nullptr, 0);
            if (value != 0 && value != ULLONG_MAX) {
                defaultStart = value;
            }
        } else if (!strcmp(argv[i], "-e")) {
            uint64_t value = strtoull(argv[i + 1], nullptr, 0);
            if (value != 0 && value != ULLONG_MAX) {
                defaultEnd = value;
            }
        } else if (!strcmp(argv[i], "-t")) {
            uint64_t value = strtoull(argv[i + 1], nullptr, 0);
            if (value != 0 && !(value > SIZE_MAX)) {
                defaultNumThreads = value;
            }
        } else {
            std::cout << "Unrecognized argument " << argv[i] << std::endl;
        }
    }

    ThreadPool pool(defaultStart, defaultEnd, defaultNumThreads);
    pool.start();
}
