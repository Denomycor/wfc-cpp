#include "wfc.hpp"
#include "abstract_wfc.hpp"
#include <cassert>
#include <cfloat>

namespace wfc {


WFC::WFC(const Vec3u& size, const TileWeights& weights, unsigned int seed, bool periodic)
:m_wave(new Array3D<CellState>(size.x, size.y, size.z)),
m_entropy(size),
constraints(weights.size(), true),
weights(weights),
labels(weights.size()),
m_rng(seed),
m_periodic(periodic)
{}


// WFC::WFC(const WFC& view, const Vec3u offset, const Vec3u length)
// :m_wave(new Array3DView<CellState>(*(view.m_wave), offset, length)),
// m_entropy(length), 
// constraints(view.constraints),
// weights(view.weights)
// {}


WFC::WFC(const Vec3u& size, const TileWeights& weights, const AdjacencyConstraints& constraints, unsigned int seed, bool periodic)
:m_wave(new Array3D<CellState>(size.x, size.y, size.z)),
m_entropy(size),
constraints(constraints),
weights(weights),
labels(weights.size()),
m_rng(seed),
m_periodic(periodic)
{}


WFC::~WFC() {
    delete m_wave;
}


void WFC::clean_cache(){
    m_entropy.invalidate_all();
}


void WFC::init(){
    for(auto& c : *m_wave){
        c.resize(weights.size(), 1);
    }
    m_status = Status::READY_STATUS;
}


void WFC::init_cell(const Vec3u& coords, unsigned int bit, bool value){
    auto[x,y,z] = coords;
    m_wave->get(x, y, z).reset();
    m_wave->get(x,y,z)[bit] = value;
}


Array3D<unsigned int> WFC::get_result() {
    Array3D<unsigned int> out(m_wave->get_width(), m_wave->get_height(), m_wave->get_depth());
    for(std::size_t i = 0; i < m_wave->size(); i++) {
        out.get_linear(i) = m_wave->get_linear(i).find_first();
    }
    return out;
}


std::optional<Vec3u> WFC::select_cell(){
    std::vector<Vec3u> out{{0,0,0}};
    double entr = DBL_MAX;

    for(std::size_t x = 0; x < m_wave->get_width(); x++){
    for(std::size_t y = 0; y < m_wave->get_height(); y++){
    for(std::size_t z = 0; z < m_wave->get_depth(); z++){
        double e = m_entropy.get_cell_entropy(Vec3u(x,y,z), m_wave->get(x,y,z), weights);
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
        return out[m_rng.next_int() % out.size()];
    }
}


void WFC::collapse_cell(const Vec3u& coords) {
    auto[x,y,z] = coords;
    auto& cell = m_wave->get(x, y, z);

    double total = 0;
    for(std::size_t i=0; i<cell.size(); i++){
        if(cell[i])
            total += weights[i];
    }

    auto r = m_rng.next_double();
    double acc = 0;
    auto selected = 0;
    for(std::size_t i=0; i<cell.size(); i++){
        if(cell[i]){
            acc += weights[i] / total;
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


bool WFC::update_cell_state(CellState& cell, const TileConstraints& constraints, const CellState& neighbor) {
    auto tmp = cell;
    CellState new_cell(cell.size());
    new_cell.reset();
    for (std::size_t i = 0; i < neighbor.size(); i++) {
        if (neighbor[i]) {
            new_cell |= constraints[i];
        }
    }
    cell &= new_cell;
    return tmp != cell;
}


void WFC::propagate_direction(const Vec3i& from, const Vec3i& to, Directions dir, std::queue<Vec3i>& queue) {
    auto[f_x, f_y, f_z] = from;
    auto[t_x, t_y, t_z] = to;
    Vec3u dim{
        static_cast<unsigned int>(m_wave->get_width()), 
        static_cast<unsigned int>(m_wave->get_height()), 
        static_cast<unsigned int>(m_wave->get_depth())
    };
    if(m_periodic){
        if(update_cell_state(m_wave->get_wrapped(t_x, t_y, t_z), constraints.get(dir), m_wave->get_wrapped(f_x, f_y, f_z))){
            queue.push(to);
            m_entropy.invalidate_cell(to.wrapi(dim));
        }
    }else if(m_wave->valid_coords(t_x, t_y, t_z)){
        if(update_cell_state(m_wave->get(t_x, t_y, t_z), constraints.get(dir), m_wave->get(f_x, f_y, f_z))){
            queue.push(to);
            m_entropy.invalidate_cell(to.to_vec3u());
        }
    }
}


void WFC::propagate_constraints(const Vec3u& coords){
    std::queue<Vec3i> queue;
    queue.push(static_cast<Vec3i>(coords));

    while(!queue.empty()){
        auto current = queue.front();

        propagate_direction(current, current + Vec3Constants::UP, Directions::UP, queue);
        propagate_direction(current, current + Vec3Constants::DOWN, Directions::DOWN, queue);
        propagate_direction(current, current + Vec3Constants::LEFT, Directions::LEFT, queue);
        propagate_direction(current, current + Vec3Constants::RIGHT, Directions::RIGHT, queue);
        propagate_direction(current, current + Vec3Constants::BACK, Directions::BACK, queue);
        propagate_direction(current, current + Vec3Constants::FRONT, Directions::FRONT, queue);

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
    stepped.emit(this, m_step_counter++, selected.value());
    return false;
}


bool WFC::run() {
    while(true){
        if(step()){
            finished.emit(this);
            return m_status != AbstractWFC::CONTRADICTION_STATUS;
        };
    }
}


Vec3u WFC::get_size(){
    return Vec3u(m_wave->get_width(), m_wave->get_height(), m_wave->get_depth());
}


const WaveState& WFC::get_wave(){
    return *m_wave;
}


void WFC::set_wave(const WaveState& wave){
    *m_wave = wave;
}

}

