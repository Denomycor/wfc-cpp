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


WFC::WFC(const Vec3u& size, const TileWeights& weights, const AdjacencyConstraints& constraints)
:m_wave(new Array3D<CellState>(std::get<0>(size), std::get<1>(size), std::get<2>(size))),
m_entropy(size),
m_adjacency(constraints),
m_weights(weights)
{}


WFC::~WFC() {
    delete m_wave;
}


void WFC::init(){
    for(auto& c : *m_wave){
        c.resize(m_weights.size(), 1);
    }
}


void WFC::init(std::size_t id, bool value){
    for(auto& c : *m_wave){
        c[id] = value;
    }
}


AdjacencyConstraints& WFC::get_constraints(){
    return m_adjacency;
}


TileWeights& WFC::get_weights(){
    return m_weights;
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


std::optional<Vec3u> WFC::select_cell(){
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
            return {};
        }
    }}}

    if(entr == DBL_MAX) {
    //All cells have collapsed get_cell_entropy always returned 0 and entr is unchanged since initialized
        m_status = AbstractWFC::FINISHED_STATUS;
        return {};
    }else{
        m_status = AbstractWFC::RUNNING_STATUS;
        return out[rand() % out.size()];
    }
}


void WFC::collapse_cell(const Vec3u& coords) {
    auto[x,y,z] = coords;
    auto& cell = m_wave->get(x, y, z);

    double total = 0;
    for(std::size_t i=0; i<cell.size(); i++){
        if(cell[i])
            total += m_weights[i];
    }

    auto r = rand() / (double) RAND_MAX;
    double acc = 0;
    auto selected = 0;
    for(std::size_t i=0; i<cell.size(); i++){
        if(cell[i]){
            acc += m_weights[i] / total;
            if(r <= acc){
                selected = i;
                break;
            }
        }
    }

    cell.reset();
    cell[selected] = true; 
    m_entropy.invalidate_cell(coords);
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


void WFC::propagate_direction(const Vec3u& from, const Vec3u& to, Directions dir, std::queue<Vec3u>& queue) {
    auto[f_x, f_y, f_z] = from;
    auto[t_x, t_y, t_z] = to;
    if(m_wave->valid_coords(t_x, t_y, t_z)){
        if(update_cell_state(m_wave->get(t_x, t_y, t_z), m_adjacency.get(dir), m_wave->get(f_x, f_y, f_z))){
            queue.push(to);
            m_entropy.invalidate_cell(to);
        }
    }
}


void WFC::propagate_constraints(const Vec3u& coords){
    int x=std::get<0>(coords), y=std::get<1>(coords), z=std::get<2>(coords);
    std::queue<Vec3u> queue;
    queue.push({x,y,z});

    while(!queue.empty()){
        auto current = queue.front();
        auto[x, y, z] = current;

        propagate_direction(current, {x,y-1,z}, Directions::UP, queue);
        propagate_direction(current, {x,y+1,z}, Directions::DOWN, queue);
        propagate_direction(current, {x-1,y,z}, Directions::LEFT, queue);
        propagate_direction(current, {x+1,y,z}, Directions::RIGHT, queue);
        propagate_direction(current, {x,y,z-1}, Directions::BACK, queue);
        propagate_direction(current, {x,y,z+1}, Directions::FRONT, queue);

        queue.pop();
    }
}


bool WFC::step() {
    auto selected = select_cell();
    if(!selected.has_value()){
        return true;
    }
    collapse_cell(selected.value());
    propagate_constraints(selected.value());
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

