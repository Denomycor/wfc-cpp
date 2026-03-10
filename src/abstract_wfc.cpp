#include "abstract_wfc.hpp"
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <cmath>
#include <string>


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

WaveConstraints& AdjacencyConstraints::get() {
    return m_constraints;
}


const TileConstraints& AdjacencyConstraints::get(Directions dir) const {
    return m_constraints[dir];
}


void AdjacencyConstraints::change_rule(std::size_t id, Directions dir, std::size_t n_id, bool value){
    m_constraints[dir][id][n_id] = value;
    m_constraints[get_opposite(dir)][n_id][id] = value;
}

//********************************************************************************************************

// static auto add_new_id(TileWeights& weights, AdjacencyConstraints& constraints, TileLabels& labels) {
//     auto size = weights.size();
//     weights.emplace_back(0);
//     labels.emplace_back();
//     for(auto& v : constraints.get()){
//         for(auto& bs : v){
//             bs.resize(size+1, 1);
//         }
//         v.emplace_back(boost::dynamic_bitset<>(size+1, 1));
//     }
//     return size;
// }
//
// void generate_variants(std::size_t id, Variants2D type, TileWeights& weights, AdjacencyConstraints& constraints, TileLabels& labels) {
//     if(labels.size() != weights.size()){
//         labels.resize(weights.size());
//     }
//     if(labels[id].empty()){
//         labels[id] = std::to_string(id);
//     }
//     auto& c = constraints.get();
//
//     switch (type) {
//
//     case NONE:
//     case FULL:
//         return;
//
//     // L symmetry → 4 rotations
//     case CORNER:
//     case ROTATED: {
//         std::size_t prev = id;
//
//         for(int r = 1; r <= 3; r++){
//             auto new_id = add_new_id(weights, constraints, labels);
//
//             weights[new_id] = weights[id];
//
//             c[Directions::UP][new_id]    = c[Directions::LEFT][prev];
//             c[Directions::RIGHT][new_id] = c[Directions::UP][prev];
//             c[Directions::DOWN][new_id]  = c[Directions::RIGHT][prev];
//             c[Directions::LEFT][new_id]  = c[Directions::DOWN][prev];
//
//             labels[new_id] = labels[id] +
//                 ((type == CORNER) ? "_L" : "_T") +
//                 std::to_string(r);
//
//             prev = new_id;
//         }
//
//         return;
//     }
//
//     // I symmetry → 180° rotation
//     case STRAIGHT: {
//         auto new_id = add_new_id(weights, constraints, labels);
//
//         weights[new_id] = weights[id];
//
//         c[Directions::UP][new_id] = c[Directions::DOWN][id];
//         c[Directions::RIGHT][new_id] = c[Directions::LEFT][id];
//         c[Directions::DOWN][new_id] = c[Directions::UP][id];
//         c[Directions::LEFT][new_id] = c[Directions::RIGHT][id];
//
//         labels[new_id] = labels[id] + "_I1";
//
//         return;
//     }
//
//     // diagonal mirror symmetry
//     case DIAGONAL: {
//         auto new_id = add_new_id(weights, constraints, labels);
//
//         weights[new_id] = weights[id];
//
//         c[Directions::UP][new_id] = c[Directions::LEFT][id];
//         c[Directions::RIGHT][new_id] = c[Directions::DOWN][id];
//         c[Directions::DOWN][new_id] = c[Directions::RIGHT][id];
//         c[Directions::LEFT][new_id] = c[Directions::UP][id];
//
//         labels[new_id] = labels[id] + "_D1";
//
//         return;
//     }
//     }
// }
//
