#pragma once
#include <assert.h>
#include <cstddef>
#include <vector>
#include "utils.hpp"

template<typename T>
class Array3D {
private:
    std::size_t width, height, depth;
    std::vector<T> data;

    inline std::size_t index(std::size_t x, std::size_t y, std::size_t z) const {
        return z * width * height + y * width + x;
    }

    inline std::size_t wrapped_index(std::size_t x, std::size_t y, std::size_t z) const {
        return wrapped(0, depth, z) * width * height + wrapped(0, height, y) * width + wrapped(0, width, x);
    }

public:
    Array3D(std::size_t w, std::size_t h, std::size_t d)
        : width(w), height(h), depth(d), data(w * h * d)
    {}

    Array3D(std::size_t w, std::size_t h, std::size_t d, const T& init_value)
        : width(w), height(h), depth(d), data(w * h * d, init_value)
    {}

    T& get(std::size_t x, std::size_t y, std::size_t z) {
        assert(x < width && y < height && z < depth);
        return data[index(x,y,z)];
    }

    T& get_wrapped(std::size_t x, std::size_t y, std::size_t z) {
        return data[wrapped_index(x,y,z)];
    }

    const T& get(std::size_t x, std::size_t y, std::size_t z) const {
        assert(x < width && y < height && z < depth);
        return data[index(x,y,z)];
    }

    const T& wrapped_get(std::size_t x, std::size_t y, std::size_t z) const {
        return data[wrapped_index(x,y,z)];
    }

    template<typename U>
    void set(std::size_t x, std::size_t y, std::size_t z, U&& value)
    {
        assert(x < width && y < height && z < depth);
        data[index(x,y,z)] = std::forward<U>(value);
    }

    bool valid_coords(int x, int y, int z, bool wrap = false) const {
        return wrap || (x >= 0 && y >= 0 && z >= 0 &&
               static_cast<std::size_t>(x) < width &&
               static_cast<std::size_t>(y) < height &&
               static_cast<std::size_t>(z) < depth);
    }

    std::size_t get_width() const { return width; }
    std::size_t get_height() const { return height; }
    std::size_t get_depth() const { return depth; }
    std::size_t size() const { return data.size(); }

    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
};

