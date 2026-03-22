#include "random.hpp"
#include <chrono>

namespace wfc {

Random::Random()
: rng()
{
    set_seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
}

Random::Random(unsigned int seed)
: rng()
{
    set_seed(seed);
}

void Random::set_seed(unsigned int seed) {
    rng.seed(seed);
}

int Random::next_int(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

double Random::next_double(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

}

