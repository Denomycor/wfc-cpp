#include "wfc.hpp"
#include "abstract_wfc.hpp"
#include "array3d.hpp"
#include "utils.hpp"
#include <cfloat>
#include <iostream>
#include <queue>
#include <tuple>
#include <vector>

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec)
{
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        os << vec[i];
        if (i + 1 < vec.size())
            os << ", ";
    }
    os << "]";
    return os;
}


template <typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<K, V>& map)
{
    os << "{";
    auto it = map.begin();
    while (it != map.end())
    {
        os << it->first << ": " << it->second;
        ++it;
        if (it != map.end())
            os << ", ";
    }
    os << "}";
    return os;
}


void WFC3D::init() {
    for(auto& tile : *m_wave){
        for(auto [t,w] : m_weights){
            if(w>0)
                tile[t] = true;
        }
    }
    m_status = AbstractWFC::READY_STATUS;
}

WFC3D::WFC3D(int width, int height, int depth, const TileWeights& weights)
:AbstractWFC(), m_wave(new Array3D<CellState>(width, height, depth)), m_entropy(width, height, depth), m_weights(weights), m_constraints()
{
    m_constraints[Directions::UP] = TileConstraints();
    m_constraints[Directions::DOWN] = TileConstraints(); 
    m_constraints[Directions::LEFT] = TileConstraints(); 
    m_constraints[Directions::RIGHT] = TileConstraints(); 
    m_constraints[Directions::FRONT] = TileConstraints(); 
    m_constraints[Directions::BACK] = TileConstraints(); 
}

WFC3D::WFC3D(
    const WFC3D& p_source, 
    const std::tuple<int,int,int>& offset,
    const std::tuple<int,int,int>& length,
    const TileWeights& weights
)
:AbstractWFC(), 
    m_wave(new Array3DView<CellState>(*(p_source.m_wave), offset, length)), 
    m_entropy(p_source.m_wave->get_width(), p_source.m_wave->get_height(), p_source.m_wave->get_depth()), 
    m_weights(weights), 
    m_constraints()
{
    m_constraints[Directions::UP] = TileConstraints();
    m_constraints[Directions::DOWN] = TileConstraints(); 
    m_constraints[Directions::LEFT] = TileConstraints(); 
    m_constraints[Directions::RIGHT] = TileConstraints(); 
    m_constraints[Directions::FRONT] = TileConstraints(); 
    m_constraints[Directions::BACK] = TileConstraints(); 
}


std::tuple<int, int, int> WFC3D::select_cell(){
    std::vector<std::tuple<int, int, int>> out{{0,0,0}};
    double entr = DBL_MAX;

    for(std::size_t x = 0; x < m_wave->get_width(); x++){
        for(std::size_t y = 0; y < m_wave->get_height(); y++){
            for(std::size_t z = 0; z < m_wave->get_depth(); z++){
                double e = m_entropy.get_cell_entropy(x, y, z, m_wave->get(x,y,z), m_weights);
                if(e > 0) {
                    if(e < entr){
                        out.clear();
                        out.emplace_back(x,y,z);   
                        entr = e;
                    } else if (e == entr) {
                    // this comparison of doubles is ok because cells with same tile-weight distribution should return the same exact entropy value
                        out.emplace_back(x,y,z);
                    }
                }else if(e < 0){
                // get_cell_entropy returns < 0 if a cell has 0 tiles possible
                    m_status = AbstractWFC::CONTRADICTION_STATUS;
                    return {DBL_MIN,DBL_MIN,DBL_MIN};
                }
            }
        }
    }

    if(entr == DBL_MAX) {
    //All cells have collapsed get_cell_entropy always returned 0 and entr is unchanged since initialized
        m_status = AbstractWFC::FINISHED_STATUS;
        return {DBL_MIN,DBL_MIN,DBL_MIN};
    }else{
        m_status = AbstractWFC::RUNNING_STATUS;
        return out[rand() % out.size()];
    }
}

// Build a map of only the available tiles and normalize their weights
static auto normalized_weight_map(const std::unordered_map<int, double>& weights, const std::unordered_map<int, bool>& tiles) {
    std::unordered_map<int, double> out;
    double w_sum = 0;
    for(auto[t,b] : tiles){
        if(b){
            out[t] = weights.at(t);
            w_sum += weights.at(t);
        }
    }
    for(auto[t,w] : out){
        out[t] = w/w_sum;
    }
    return out;
}

void WFC3D::collapse_cell(const std::tuple<int, int, int>& coords) {
    int x=std::get<0>(coords), y=std::get<1>(coords), z=std::get<2>(coords);
    auto normalized = normalized_weight_map(m_weights, m_wave->get(x, y, z));

    auto r = rand() / (double) RAND_MAX;
    double acc = 0;
    int selected_id = normalized.begin()->first;
    for(auto[t,w] : normalized){
        acc += w;
        if(r <= acc){
            selected_id = t;
            break;
        }
    }
    for(auto[t,b] : m_wave->get(x,y,z)){
        if(t != selected_id){
            m_wave->get(x,y,z)[t] = false;
        }else{
            assert(b);
        }
    }
    m_entropy.invalidate_cell(x, y, z);
}


static bool update_cell_state(std::unordered_map<int, bool>& cell, const std::unordered_map<int, std::vector<int>>& constraints, const std::unordered_map<int, bool>& neighboor) {
    bool updated = false;
    for(auto[tile, enabled] : cell){
        // skip tiles already excluded
        if(!enabled) continue;

        bool found = false;

        for(auto[n_tile, n_enabled] : neighboor){
            // skip tiles already excluded
            if(!n_enabled) continue;

            const auto& vec = constraints.at(n_tile);
            if(vec_has(vec, tile)){
                found = true;
                break;
            }
        }
        updated = updated || !found;
        cell[tile] = found;
    }
    return updated;
}


void WFC3D::propagate_constraints(const std::tuple<int, int, int>& coords){
    int x=std::get<0>(coords), y=std::get<1>(coords), z=std::get<2>(coords);
    std::queue<std::tuple<int, int, int>> queue;
    queue.push({x,y,z});

    while(!queue.empty()){
        auto current = queue.front();
        int cur_x=std::get<0>(current), cur_y=std::get<1>(current), cur_z=std::get<2>(current);

        //Up
        if(cur_y > 0){
            if(update_cell_state(m_wave->get(cur_x, cur_y-1, cur_z), m_constraints[Directions::UP], m_wave->get(cur_x, cur_y, cur_z))){
                queue.push({cur_x, cur_y-1, cur_z});
                m_entropy.invalidate_cell(cur_x, cur_y-1, cur_z);
            }
        }
        //Down
        if(cur_y < static_cast<int>(m_wave->get_height())-1){
            if(update_cell_state(m_wave->get(cur_x, cur_y+1, cur_z), m_constraints[Directions::DOWN], m_wave->get(cur_x, cur_y, cur_z))){
                queue.push({cur_x, cur_y+1, cur_z});
                m_entropy.invalidate_cell(cur_x, cur_y+1, cur_z);
            }
        }
        //Left
        if(cur_x > 0){
            if(update_cell_state(m_wave->get(cur_x-1, cur_y, cur_z), m_constraints[Directions::LEFT], m_wave->get(cur_x, cur_y, cur_z))){
                queue.push({cur_x-1, cur_y, cur_z});
                m_entropy.invalidate_cell(cur_x-1, cur_y, cur_z);
            }
        }
        //Right
        if(cur_x < static_cast<int>(m_wave->get_width())-1){
            if(update_cell_state(m_wave->get(cur_x+1, cur_y, cur_z), m_constraints[Directions::RIGHT], m_wave->get(cur_x, cur_y, cur_z))){
                queue.push({cur_x+1, cur_y, cur_z});
                m_entropy.invalidate_cell(cur_x+1, cur_y, cur_z);
            }
        }
        //Back
        if(cur_z > 0){
            if(update_cell_state(m_wave->get(cur_x, cur_y, cur_z-1), m_constraints[Directions::BACK], m_wave->get(cur_x, cur_y, cur_z))){
                queue.push({cur_x, cur_y, cur_z-1});
                m_entropy.invalidate_cell(cur_x, cur_y, cur_z-1);
            }
        }
        //Front
        if(cur_z < static_cast<int>(m_wave->get_depth())-1){
            if(update_cell_state(m_wave->get(cur_x, cur_y, cur_z+1), m_constraints[Directions::FRONT], m_wave->get(cur_x, cur_y, cur_z))){
                queue.push({cur_x, cur_y, cur_z+1});
                m_entropy.invalidate_cell(cur_x, cur_y, cur_z+1);
            }
        }
        queue.pop();
    }
}


bool WFC3D::step() {
    auto selected = select_cell();
    if(m_status != AbstractWFC::RUNNING_STATUS){
        return true;
    }
    collapse_cell(selected);
    propagate_constraints(selected);
    stepped.emit(this);
    return false;
}


bool WFC3D::run() {
    while(true){
        if(step()){
            finished.emit(this);
            return m_status == AbstractWFC::CONTRADICTION_STATUS;
        };
    }
}


void WFC3D::add_constraint(
    TileId tile,
    const std::vector<TileId>& up,
    const std::vector<TileId>& down,
    const std::vector<TileId>& left,
    const std::vector<TileId>& right
){
    m_constraints[Directions::UP][tile] = up;
    m_constraints[Directions::DOWN][tile] = down;
    m_constraints[Directions::LEFT][tile] = left;
    m_constraints[Directions::RIGHT][tile] = right;
}


void WFC3D::add_constraint(
    TileId tile,
    const std::vector<TileId>& up,
    const std::vector<TileId>& down,
    const std::vector<TileId>& left,
    const std::vector<TileId>& right,
    const std::vector<TileId>& front,
    const std::vector<TileId>& back
){
    add_constraint(tile, up, down, left, right);
    m_constraints[Directions::FRONT][tile] = front;
    m_constraints[Directions::BACK][tile] = back;
}


void WFC3D::add_constraint_allow_all(TileId tile){
    std::vector<TileId> vec;
    for(const auto&[t,w] : m_weights){
        vec.emplace_back(t);
    }
    add_constraint(tile, vec, vec, vec, vec, vec, vec);
}


void WFC3D::print2D() const {
    for(std::size_t y=0; y<m_wave->get_height(); y++){
        for(std::size_t x=0; x<m_wave->get_width(); x++){
            for(auto[k,b] : m_wave->get(x,y,0)){
                if(b) std::cout << k << '.';
            }
            std::cout << ' ';
        }
        std::cout << std::endl;
    }
}

WFC3D::~WFC3D(){
    delete m_wave;
}

