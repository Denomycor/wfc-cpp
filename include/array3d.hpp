#pragma once
#include <assert.h>
#include <cstddef>
#include <tuple>
#include <vector>
#include "utils.hpp"


template<typename T>
class AbstractArray3D {
protected:
    virtual inline std::size_t index(std::size_t x, std::size_t y, std::size_t z) const = 0;
    virtual inline std::size_t wrapped_index(std::size_t x, std::size_t y, std::size_t z) const = 0;

public:
    class iterator {
    private:
        AbstractArray3D<T>* array;
        std::size_t index;

    public:
        iterator(AbstractArray3D<T>* arr, std::size_t i)
            : array(arr), index(i) 
        {}

        T& operator*() {
            return array->get_linear(index);
        }

        T* operator->() {
            return &array->get_linear(index);
        }

        iterator& operator++() {
            ++index;
            return *this;
        }

        bool operator==(const iterator& other) const {
            return index == other.index;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };

    class const_iterator {
        const AbstractArray3D<T>* array;
        std::size_t index_;

    public:
        const_iterator(const AbstractArray3D<T>* arr, std::size_t i)
            : array(arr), index_(i) {}

        const T& operator*() const {
            return array->get_linear(index_);
        }

        const T* operator->() const {
            return &array->get_linear(index_);
        }

        const_iterator& operator++() {
            ++index_;
            return *this;
        }

        bool operator==(const const_iterator& other) const {
            return index_ == other.index_;
        }

        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }
    };

    virtual T& get(std::size_t x, std::size_t y, std::size_t z) = 0;
    virtual T& get_wrapped(int x, int y, int z) = 0;
    virtual T& get_linear(std::size_t i) = 0;
    virtual const T& get(std::size_t x, std::size_t y, std::size_t z) const = 0;
    virtual const T& wrapped_get(int x, int y, int z) const = 0;
    virtual const T& get_linear(std::size_t i) const = 0;

    virtual void set(std::size_t x, std::size_t y, std::size_t z, const T& value) = 0;

    virtual bool valid_coords(int x, int y, int z, bool wrap = false) const = 0;

    virtual std::size_t get_width() const = 0;
    virtual std::size_t get_height() const = 0;
    virtual std::size_t get_depth() const = 0;
    virtual std::size_t size() const = 0;

    iterator begin() {
        return iterator(this,0);
    }
    iterator end() {
        return iterator(this, size());
    };
    const_iterator begin() const {
        return const_iterator(this, 0);
    };
    const_iterator end() const {
        return const_iterator(this, size());
    }

    virtual ~AbstractArray3D() = default;

};

template<typename T>
class Array3DView;

template<typename T>
class Array3D : public AbstractArray3D<T> {
private:
    std::size_t width, height, depth;
    std::vector<T> data;

protected:
    inline std::size_t index(std::size_t x, std::size_t y, std::size_t z) const override {
        return z * width * height + y * width + x;
    }

    inline std::size_t wrapped_index(std::size_t x, std::size_t y, std::size_t z) const override {
        return wrapped(0, depth, z) * width * height + wrapped(0, height, y) * width + wrapped(0, width, x);
    }

public:
    Array3D(std::size_t w, std::size_t h, std::size_t d)
        : width(w), height(h), depth(d), data(w * h * d)
    {}

    Array3D(std::size_t w, std::size_t h, std::size_t d, const T& init_value)
        : width(w), height(h), depth(d), data(w * h * d, init_value)
    {}

    T& get(std::size_t x, std::size_t y, std::size_t z) override {
        assert(x < width && y < height && z < depth);
        return data[index(x,y,z)];
    }

    T& get_wrapped(int x, int y, int z) override {
        return data[wrapped_index(x,y,z)];
    }

    T& get_linear(std::size_t i) override {
        return data[i];
    }

    const T& get(std::size_t x, std::size_t y, std::size_t z) const override {
        assert(x < width && y < height && z < depth);
        return data[index(x,y,z)];
    }

    const T& wrapped_get(int x, int y, int z) const override {
        return data[wrapped_index(x,y,z)];
    }

    const T& get_linear(std::size_t i) const override {
        return data[i];
    }

    void set(std::size_t x, std::size_t y, std::size_t z, const T& value) override
    {
        assert(x < width && y < height && z < depth);
        data[index(x,y,z)] = value;
    }

    bool valid_coords(int x, int y, int z, bool wrap = false) const override {
        return wrap || (x >= 0 && y >= 0 && z >= 0 &&
               static_cast<std::size_t>(x) < width &&
               static_cast<std::size_t>(y) < height &&
               static_cast<std::size_t>(z) < depth);
    }

    std::size_t get_width() const override { return width; }
    std::size_t get_height() const override { return height; }
    std::size_t get_depth() const override { return depth; }
    std::size_t size() const override { return data.size(); }

    friend class Array3DView<T>;

};

template<typename T>
class Array3DView : public AbstractArray3D<T> {
private:
    Array3D<T>& source;
    std::tuple<int, int, int> offset, length;

protected:
    inline std::size_t index(std::size_t x, std::size_t y, std::size_t z) const override {
        return source.index(std::get<0>(offset) + x, std::get<1>(offset) + y, std::get<2>(offset) + z);
    }

    inline std::size_t wrapped_index(std::size_t x, std::size_t y, std::size_t z) const override {
        return source.index(
            std::get<0>(offset) + wrapped(0, std::get<0>(length), x), 
            std::get<1>(offset) + wrapped(0, std::get<1>(length), y), 
            std::get<2>(offset) + wrapped(0, std::get<2>(length), z) 
        );
    }

public:
    Array3DView(Array3D<T>& p_source, std::tuple<int,int,int> p_offset, std::tuple<int,int,int> p_length)
    :source(p_source), offset(p_offset), length(p_length)
    {}

    T& get(std::size_t x, std::size_t y, std::size_t z) override {
        return source.data[index(x, y, z)];
    }

    T& get_wrapped(int x, int y, int z) override {
        return source.data[wrapped_index(x,y,z)];
    }

    T& get_linear(std::size_t i) override {
        return const_cast<T&>(std::as_const(*this).get_linear(i));
    }

    const T& get(std::size_t x, std::size_t y, std::size_t z) const override {
        return source.data[index(x, y, z)];
    }

    const T& get_linear(std::size_t i) const override {
        std::size_t w = get_width();
        std::size_t h = get_height();

        std::size_t x = i % w;
        std::size_t y = (i / w) % h;
        std::size_t z = i / (w * h);

        return get(x,y,z);
    }


    const T& wrapped_get(int x, int y, int z) const override {
        return source.data[wrapped_index(x,y,z)];
    }

    void set(std::size_t x, std::size_t y, std::size_t z, const T& value) override {
        source.data[index(x,y,z)] = value;
    }

    bool valid_coords(int x, int y, int z, bool wrap = false) const override {
        return wrap || (x >= 0 && y >= 0 && z >= 0 &&
               static_cast<std::size_t>(x) < std::get<0>(length) &&
               static_cast<std::size_t>(y) < std::get<1>(length) &&
               static_cast<std::size_t>(z) < std::get<2>(length));
    }

    std::size_t get_width() const override { return std::get<0>(length); }
    std::size_t get_height() const override { return std::get<1>(length); }
    std::size_t get_depth() const override { return std::get<2>(length); }

    std::size_t size() const override { 
        return get_width() * get_height() * get_depth();
    }

};

