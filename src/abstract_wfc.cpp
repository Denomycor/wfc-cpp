#include "abstract_wfc.hpp"
#include <array>
#include <boost/dynamic_bitset.hpp>
#include <cmath>
#include <string>


namespace wfc {

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


AbstractWFC::Status AbstractWFC::get_status() const {
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


//********************************************************************************************************

auto AdjacencyConstraints::add_new_id(TileWeights& weights, TileLabels& labels) {
    auto size = weights.size();
    weights.emplace_back(0);
    labels.emplace_back();
    for(auto& v : m_constraints){
        for(auto& bs : v){
            bs.resize(size+1, 1);
        }
        v.emplace_back(boost::dynamic_bitset<>(size+1, 1));
    }
    return size;
}


static const std::array<std::array<Directions,4>,8> D4 = {{
    {UP, DOWN, LEFT, RIGHT}, // identity
    {LEFT, RIGHT, DOWN, UP}, // rot90
    {DOWN, UP, RIGHT, LEFT}, // rot180
    {RIGHT, LEFT, UP, DOWN}, // rot270
    {DOWN, UP, LEFT, RIGHT}, // vertical flip
    {UP, DOWN, RIGHT, LEFT}, // horizontal flip
    {RIGHT, LEFT, DOWN, UP}, // horizontal flip + rot90
    {LEFT, RIGHT, UP, DOWN}, // horizontal flip + rot270
}};


// static const std::array<std::array<int,6>,24> D6 = {{
//     {UP, DOWN, LEFT, RIGHT, FRONT, BACK},   // 0  identity
//     {LEFT, RIGHT, DOWN, UP, FRONT, BACK},   // 1
//     {DOWN, UP, RIGHT, LEFT, FRONT, BACK},   // 2
//     {RIGHT, LEFT, UP, DOWN, FRONT, BACK},   // 3
//     {FRONT, BACK, LEFT, RIGHT, DOWN, UP},   // 4
//     {DOWN, UP, LEFT, RIGHT, BACK, FRONT},   // 5
//     {BACK, FRONT, LEFT, RIGHT, UP, DOWN},   // 6
//     {UP, DOWN, FRONT, BACK, RIGHT, LEFT},   // 7
//     {UP, DOWN, RIGHT, LEFT, BACK, FRONT},   // 8
//     {UP, DOWN, BACK, FRONT, LEFT, RIGHT},   // 9
//     {LEFT, RIGHT, FRONT, BACK, DOWN, UP},   // 10
//     {RIGHT, LEFT, BACK, FRONT, DOWN, UP},   // 11
//     {LEFT, RIGHT, BACK, FRONT, UP, DOWN},   // 12
//     {RIGHT, LEFT, FRONT, BACK, UP, DOWN},   // 13
//     {FRONT, BACK, DOWN, UP, RIGHT, LEFT},   // 14
//     {BACK, FRONT, UP, DOWN, RIGHT, LEFT},   // 15
//     {FRONT, BACK, UP, DOWN, LEFT, RIGHT},   // 16
//     {BACK, FRONT, DOWN, UP, LEFT, RIGHT},   // 17
//     {DOWN, UP, FRONT, BACK, LEFT, RIGHT},   // 18
//     {DOWN, UP, BACK, FRONT, RIGHT, LEFT},   // 19
//     {UP, DOWN, FRONT, BACK, LEFT, RIGHT},   // 20
//     {UP, DOWN, BACK, FRONT, RIGHT, LEFT},   // 21
//     {FRONT, BACK, RIGHT, LEFT, UP, DOWN},   // 22
//     {BACK, FRONT, LEFT, RIGHT, UP, DOWN}    // 23
// }};


void AdjacencyConstraints::generate_variant(std::size_t id,
                       Variants2D transform,
                       TileWeights& weights,
                       TileLabels& labels)
{
    if(labels.size() != weights.size())
        labels.resize(weights.size());

    if(labels[id].empty())
        labels[id] = std::to_string(id);

    if(transform == IDENTITY) // identity
        return;

    auto new_id = add_new_id(weights, labels);
    weights[new_id] = weights[id];
    auto& c = m_constraints;
    for(std::size_t d = 0; d < 4; d++)
        c[d][new_id] = c[D4[transform][d]][id];
    labels[new_id] = labels[id] + "_t" + std::to_string(transform);
}

}

