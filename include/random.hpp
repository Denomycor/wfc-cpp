#pragma once

#include <cstdint>
#include <random>

namespace wfc {

class Random {
private:
    std::mt19937 rng;

public:
    Random();
    explicit Random(unsigned int seed);

    void set_seed(unsigned int seed);

    int next_int(int min = 0, int max = INT32_MAX);
    double next_double(double min = 0.0, double max = 1.0);

};

}

