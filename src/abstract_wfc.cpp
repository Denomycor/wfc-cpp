#include "abstract_wfc.hpp"
#include <cmath>


Directions get_opposite(Directions dir) {
    switch (dir) {
    case UP: return DOWN;
    case DOWN: return UP;
    case LEFT: return RIGHT;
    case RIGHT: return LEFT;
    case FRONT: return BACK;
    case BACK: return FRONT;
    case COUNT:
      break;
    }
    return COUNT;
}

EntropyMemory::EntropyMemory(const Vec3u& size)
:m_memory(std::get<0>(size), std::get<1>(size), std::get<2>(size))
{}

double EntropyMemory::get_cell_entropy(const Vec3u& cell, const CellState& state, const TileWeights& weights){
    auto[x,y,z] = cell;
    if(m_memory.get(x,y,z).first){
        return m_memory.get(x,y,z).second;
    }else{
        double w_sum = 0;
        double sum_sum = 0;
        
        for(std::size_t i=0; i<state.size(); i++){
            if(state[i]){
                w_sum += weights[i];
                sum_sum += weights[i] * log(weights[i]);
            }
        }

        if (w_sum == 0){
        // All tiles were false -> contradiction
            return -1;
        }
        double value = log(w_sum) - (sum_sum/w_sum);
        m_memory.set(x, y, z, std::pair<bool, double>{true, value});
        return value;
    }
}

void EntropyMemory::invalidate_cell(const Vec3u& cell){
    auto[x,y,z] = cell;
    m_memory.get(x, y, z).first = false;
}

void EntropyMemory::invalidate_all(){
    for(auto& [b,d] : m_memory){
        b = false;
    }
}

AbstractWFC::Status AbstractWFC::get_stats() const {
    return m_status;
}

AdjacencyConstraints::AdjacencyConstraints(std::size_t n_tiles, bool default_allow_all)
:m_constraints(), m_tiles()
{
    for(auto& vec : m_constraints){
        vec.resize(n_tiles);
        for(auto& bs : vec){
            bs.resize(n_tiles, default_allow_all);
        }
    }
}


const WaveConstraints& AdjacencyConstraints::get() const {
    return m_constraints;
}


const TileConstraints& AdjacencyConstraints::get(Directions dir) const {
    return m_constraints[dir];
}


void AdjacencyConstraints::change_rule(std::size_t id, Directions dir, std::size_t n_id, bool value){
    m_constraints[dir][id][n_id] = value;
    m_constraints[get_opposite(dir)][n_id][id] = value;
}

