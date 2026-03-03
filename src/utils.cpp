#include "utils.hpp"

int wrapped(int lower, int upper, int x){
    return x % (upper - lower) + lower;
}

