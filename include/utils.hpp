#pragma once

#include <vector>

int wrapped(int lower, int upper, int x);

template<typename T>
bool vec_has(const std::vector<T>& vec, const T& val){
    for(const auto& v : vec){
        if(v == val){
            return true;
        }
    }
    return false;
}

