#pragma once

#include "array3d.hpp"
#include <boost/dynamic_bitset.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <array>


namespace wfc {

constexpr double EPS = 1e-9;

int wrapped(int lower, int upper, int x);
bool is_approx(double x, double y, double error = EPS);


template<typename T>
void print_array2d(const AbstractArray3D<T>& obj){
    for (std::size_t i=0; i < obj.get_height(); i++){
        for (std::size_t j=0; j < obj.get_width(); j++){
            std::cout << obj.get(j,i,0) << ' ';
        }
        std::cout << std::endl;
    }
}


template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    os << '[';
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) os << ", ";
        os << v[i];
    }
    return os << ']';
}


template<typename T, std::size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<T, N>& a)
{
    os << '[';
    for (std::size_t i = 0; i < N; ++i) {
        if (i) os << ", ";
        os << a[i];
    }
    return os << ']';
}


template <typename Block, typename Allocator>
std::ostream& operator<<(std::ostream& os, const boost::dynamic_bitset<Block, Allocator>& bs) {
    for (std::size_t i = 0; i < bs.size(); ++i) {
        os << bs[i];
    }
    return os;
}


template<typename T, typename Derived>
struct Vec3 {
    T x, y, z;

    Vec3() = default;

    Vec3(T px, T py, T pz)
        : x(px), y(py), z(pz) 
    {}

    Vec3(const std::tuple<T,T,T>& tuple)
        :x(std::get<0>(tuple)), y(std::get<1>(tuple)), z(std::get<2>(tuple))
    {}

    operator std::tuple<T, T, T>() const {
        return { x, y, z };
    }

    bool operator==(const Derived& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Derived& other) const noexcept {
        return !(*static_cast<const Derived*>(this) == other);
    }

    T volume() const {
        return x * y * z;
    }

    std::string to_string() const {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    }

    Derived operator+(const Derived& other) const {
        return Derived{
            x + other.x,
            y + other.y,
            z + other.z
        };
    }

    Derived operator-(const Derived& other) const {
        return Derived{
            x - other.x,
            y - other.y,
            z - other.z
        };
    }

    Derived operator*(T scalar) const {
        return Derived{
            x * scalar,
            y * scalar,
            z * scalar
        };
    }

    Derived operator/(T scalar) const {
        return Derived{
            x / scalar,
            y / scalar,
            z / scalar
        };
    }

    Derived& operator+=(const Derived& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return static_cast<Derived&>(*this);
    }

    Derived& operator-=(const Derived& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return static_cast<Derived&>(*this);
    }

    Derived& operator*=(T scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return static_cast<Derived&>(*this);
    }

    Derived& operator/=(T scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return static_cast<Derived&>(*this);
    }
};


template<typename T, typename Derived>
std::ostream& operator<<(std::ostream& os, const Vec3<T, Derived>& v)
{
    os << v.to_string();
    return os;
}


struct Vec3i;

struct Vec3u : public Vec3<unsigned int, Vec3u> {
    using Vec3<unsigned int, Vec3u>::Vec3;

    operator Vec3i() const;
};

struct Vec3i : public Vec3<int, Vec3i> {
    using Vec3<int, Vec3i>::Vec3;

    Vec3u wrapi(const Vec3u& val) const;
    Vec3u to_vec3u() const;
    explicit operator Vec3u() const;
};


struct Vec3Hash {
    template<typename T>
    static std::size_t h(T v) {
        if constexpr (std::is_signed_v<T>) {
            return std::hash<std::int64_t>{}(static_cast<std::int64_t>(v));
        } else {
            return std::hash<std::uint64_t>{}(static_cast<std::uint64_t>(v));
        }
    }

    std::size_t operator()(const Vec3i& v) const noexcept ;

};

namespace Vec3Constants {
    inline const Vec3i UP { 0, -1, 0 };
    inline const Vec3i DOWN { 0, 1, 0 };
    inline const Vec3i LEFT { -1, 0, 0 };
    inline const Vec3i RIGHT { 1, 0, 0 };
    inline const Vec3i FRONT { 0, 0, 1 };
    inline const Vec3i BACK { 0, 0, -1 };
}


}

