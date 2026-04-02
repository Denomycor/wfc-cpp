#include "utils.hpp"
#include <cstdlib>


namespace wfc {

int wrapped(int lower, int upper, int x){
    return x % (upper - lower) + lower;
}


bool is_approx(double x, double y, double error) {
    return std::abs(x - y) < error;
}


Vec3u::operator Vec3i() const {
    return {
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<int>(z)
    };
}


Vec3u Vec3i::wrapi(const Vec3u& val) const {
    return {
        static_cast<unsigned int>(wrapped(0, val.x, x)),
        static_cast<unsigned int>(wrapped(0, val.y, y)),
        static_cast<unsigned int>(wrapped(0, val.z, z))
    };
}


Vec3i::operator Vec3u() const {
    return {
        static_cast<unsigned int>(x),
        static_cast<unsigned int>(y),
        static_cast<unsigned int>(z)
    };
}


Vec3u Vec3i::to_vec3u() const {
    assert(x>=0 && y>=0 && z>=0 && "All values must be non-negative");
    return static_cast<Vec3u>(*this);
}


std::size_t Vec3Hash::operator()(const Vec3i& v) const noexcept {
    std::size_t seed = 0;

    auto combine = [&](std::size_t x) {
        seed ^= x + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
    };

    combine(h(v.x));
    combine(h(v.y));
    combine(h(v.z));

    return seed;
}


}
