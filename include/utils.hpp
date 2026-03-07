#pragma once

#include "array3d.hpp"
#include <boost/dynamic_bitset.hpp>
#include <iostream>
#include <vector>
#include <array>


int wrapped(int lower, int upper, int x);

template<typename T>
void print_array2d(const AbstractArray3D<T>& obj){
    for (std::size_t i=0; i < obj.get_height(); i++){
        for (std::size_t j=0; j < obj.get_width(); j++){
            std::cout << obj.get(j,i,0) << ' ';
        }
        std::cout << std::endl;
    }
}


template<typename Block, typename Allocator>
std::ostream& operator<<(std::ostream& os, const boost::dynamic_bitset<Block, Allocator>& bs) {
    for (std::size_t i = bs.find_first();
         i != boost::dynamic_bitset<Block, Allocator>::npos;
         i = bs.find_next(i))
    {
        os << i << '.';
    }
    return os;
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

