#include "wfc.hpp"
#include "abstract_wfc.hpp"
#include <cassert>
#include <cfloat>
#include <queue>


WFC::WFC(const Vec3u& size, const TileWeights& weights)
:m_wave(new Array3D<CellState>(std::get<0>(size), std::get<1>(size), std::get<2>(size))),
m_entropy(size),
m_adjacency(weights.size(), true),
m_weights(weights)
{}


WFC::WFC(const WFC& view, const Vec3u offset, const Vec3u length)
:m_wave(new Array3DView<CellState>(*(view.m_wave), offset, length)),
m_entropy(length), 
m_adjacency(view.m_adjacency),
m_weights(view.m_weights)
{}


WFC::~WFC() {
    delete m_wave;
}


void WFC::init(){
    for(auto& c : *m_wave){
        c.resize(m_weights.size(), 1);
    }
}


AdjacencyConstraints& WFC::get_constraints(){
    return m_adjacency;
}


Array3D<std::size_t> WFC::get_result() {
    Array3D<std::size_t> out(m_wave->get_width(), m_wave->get_height(), m_wave->get_depth());
    for(std::size_t i = 0; i < m_wave->size(); i++) {
        out.get_linear(i) = m_wave->get_linear(i).find_first();
    }
    return out;
}


WaveState& WFC::get_wave() {
    return *m_wave;
}


Vec3u WFC::select_cell(){
    std::vector<std::tuple<int, int, int>> out{{0,0,0}};
    double entr = DBL_MAX;

    for(std::size_t x = 0; x < m_wave->get_width(); x++){
        for(std::size_t y = 0; y < m_wave->get_height(); y++){
            for(std::size_t z = 0; z < m_wave->get_depth(); z++){
                double e = m_entropy.get_cell_entropy({x,y,z}, m_wave->get(x,y,z), m_weights);
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
static auto normalized_weight_map(const TileWeights& weights, const CellState& tiles) {
    std::unordered_map<std::size_t, double> out;
    double w_sum = 0;
    for(std::size_t i = 0; i < tiles.size(); i++){
        if(tiles[i]){
            out[i] = weights[i];
            w_sum += weights[i];
        }
    }
    for(auto[t,w] : out){
        out[t] = w/w_sum;
    }
    return out;
}


void WFC::collapse_cell(const Vec3u& coords) {
    auto[x,y,z] = coords;
    auto normalized = normalized_weight_map(m_weights, m_wave->get(x, y, z));

    auto r = rand() / (double) RAND_MAX;
    double acc = 0;
    auto selected_id = normalized.begin()->first;
    for(auto[t,w] : normalized){
        acc += w;
        if(r <= acc){
            selected_id = t;
            break;
        }
    }
    for(std::size_t i = 0; i < m_wave->get(x,y,z).size(); i++){
        if(i != selected_id){
            m_wave->get(x,y,z)[i] = false;
        }
    }
    m_entropy.invalidate_cell({x, y, z});
}


static bool update_cell_state(CellState& cell, const TileConstraints& constraints, const CellState& neighboor) {
    auto tmp = cell;
    for(std::size_t i = 0; i < neighboor.size(); i++){
        if(neighboor[i]){
            assert(cell.size() == constraints[i].size());
            cell &= constraints[i];
        }
    }
    return tmp != cell;
}


void WFC::propagate_constraints(const Vec3u& coords){
    int x=std::get<0>(coords), y=std::get<1>(coords), z=std::get<2>(coords);
    std::queue<std::tuple<int, int, int>> queue;
    queue.push({x,y,z});

    while(!queue.empty()){
        auto current = queue.front();
        auto[cur_x, cur_y, cur_z] = current;

        //Up
        if(cur_y > 0){
            if(update_cell_state(m_wave->get(cur_x, cur_y-1, cur_z), m_adjacency.get(Directions::UP), m_wave->get(cur_x, cur_y, cur_z))){
                queue.push({cur_x, cur_y-1, cur_z});
                m_entropy.invalidate_cell({cur_x, cur_y-1, cur_z});
            }
        }
        //Down
        if(cur_y < static_cast<int>(m_wave->get_height())-1){
            if(update_cell_state(m_wave->get(cur_x, cur_y+1, cur_z), m_adjacency.get(Directions::DOWN), m_wave->get(cur_x, cur_y, cur_z))){
                queue.push({cur_x, cur_y+1, cur_z});
                m_entropy.invalidate_cell({cur_x, cur_y+1, cur_z});
            }
        }
        //Left
        if(cur_x > 0){
            if(update_cell_state(m_wave->get(cur_x-1, cur_y, cur_z), m_adjacency.get(Directions::LEFT), m_wave->get(cur_x, cur_y, cur_z))){
                queue.push({cur_x-1, cur_y, cur_z});
                m_entropy.invalidate_cell({cur_x-1, cur_y, cur_z});
            }
        }
        //Right
        if(cur_x < static_cast<int>(m_wave->get_width())-1){
            if(update_cell_state(m_wave->get(cur_x+1, cur_y, cur_z), m_adjacency.get(Directions::RIGHT), m_wave->get(cur_x, cur_y, cur_z))){
                queue.push({cur_x+1, cur_y, cur_z});
                m_entropy.invalidate_cell({cur_x+1, cur_y, cur_z});
            }
        }
        //Back
        if(cur_z > 0){
            if(update_cell_state(m_wave->get(cur_x, cur_y, cur_z-1), m_adjacency.get(Directions::BACK), m_wave->get(cur_x, cur_y, cur_z))){
                queue.push({cur_x, cur_y, cur_z-1});
                m_entropy.invalidate_cell({cur_x, cur_y, cur_z-1});
            }
        }
        //Front
        if(cur_z < static_cast<int>(m_wave->get_depth())-1){
            if(update_cell_state(m_wave->get(cur_x, cur_y, cur_z+1), m_adjacency.get(Directions::FRONT), m_wave->get(cur_x, cur_y, cur_z))){
                queue.push({cur_x, cur_y, cur_z+1});
                m_entropy.invalidate_cell({cur_x, cur_y, cur_z+1});
            }
        }
        queue.pop();
    }
}


bool WFC::step() {
    auto selected = select_cell();
    if(m_status != AbstractWFC::RUNNING_STATUS){
        return true;
    }
    collapse_cell(selected);
    propagate_constraints(selected);
    stepped.emit(this);
    return false;
}


bool WFC::run() {
    while(true){
        if(step()){
            finished.emit(this);
            return m_status == AbstractWFC::CONTRADICTION_STATUS;
        };
    }
}

