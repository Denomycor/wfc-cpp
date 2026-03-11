#include "utils.hpp"

using namespace wfc;

int wrapped(int lower, int upper, int x){
    return x % (upper - lower) + lower;
}

