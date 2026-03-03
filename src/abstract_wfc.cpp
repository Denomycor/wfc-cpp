#include "abstract_wfc.hpp"
#include <cmath>

EntropyWFC::EntropyWFC(int width, int height, int depth)
:entropy_memory(width, height, depth)
{}

double EntropyWFC::get_cell_entropy(int x, int y, int z, const CellState& cell, const TileWeights& weights){
    if(entropy_memory.get(x,y,z).first){
        return entropy_memory.get(x,y,z).second;
    }else{
        double w_sum = 0;
        double sum_sum = 0;
        for(auto[t,b] : cell){
            if(b){
                w_sum += weights.at(t);
                sum_sum += weights.at(t) * log(weights.at(t));
            }
        }
        if (w_sum == 0){
            return -1; // 0 tiles in cell, contradiction
        }
        double value = log(w_sum) - (sum_sum/w_sum);
        entropy_memory.set(x, y, z, std::pair<bool, double>{true, value});
        return value;
    }
}

void EntropyWFC::invalidate_cell(int x, int y, int z){
    entropy_memory.get(x, y, z).first = false;
}

void EntropyWFC::invalidate_all(){
    for(auto& [b,d] : entropy_memory){
        b = false;
    }
}

AbstractWFC::AbstractWFC()
:m_status(AbstractWFC::NOT_INIT_STATUS)
{}

