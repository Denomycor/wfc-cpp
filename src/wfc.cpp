#include "wfc.hpp"
#include "abstract_wfc.hpp"
#include "utils.hpp"
#include <algorithm>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <cassert>
#include <cfloat>
#include <tuple>

namespace wfc {



WFC::WFC(const Vec3u& size, const TileWeights& weights, unsigned int seed, bool periodic)
:m_wave(new Array3D<CellState>(size.x, size.y, size.z)),
m_entropy(size),
constraints(weights.size(), true),
weights(weights),
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
m_rng(seed),
m_periodic(periodic)
{}


WFC::~WFC() {
    delete m_wave;
}


bool WFC::check_contradiction(){
    if(m_status == CONTRADICTION_STATUS){
        return true;
    }else{
        for(auto& c : *m_wave){
            if(c.none()) return true;
        }
        return false;
    }
}


void WFC::clean_cache(){
    m_entropy.invalidate_all();
}


void WFC::init(){
    for(auto& c : *m_wave){
        c.resize(weights.size(), 1);
        c.set();
    }
    m_scratch.resize(weights.size());
    m_removed.resize(weights.size());
    m_entropy.reset(weights);
    // Set before rebuild_entropy_queue(), not after: a degenerate all-empty
    // (e.g. zero tiles) wave would otherwise have this unconditionally
    // overwrite the CONTRADICTION_STATUS requeue_cell just set.
    m_status = Status::READY_STATUS;
    rebuild_entropy_queue();
}


Array3D<unsigned int> WFC::get_result() {
    Array3D<unsigned int> out(m_wave->get_width(), m_wave->get_height(), m_wave->get_depth());
    for(std::size_t i = 0; i < m_wave->size(); i++) {
        out.get_linear(i) = m_wave->get_linear(i).find_first();
    }
    return out;
}


std::optional<Vec3u> WFC::select_cell(){
    // A contradiction is flagged eagerly (see requeue_cell) the moment any
    // cell loses its last possible tile, rather than discovered by scanning
    // for it here: m_status only ever moves into CONTRADICTION_STATUS, and
    // once it does every subsequent select_cell() call (this one included)
    // must report it before considering any candidate.
    if(m_status == AbstractWFC::CONTRADICTION_STATUS){
        return {};
    }

    if(m_entropy_queue.empty()){
        m_status = AbstractWFC::FINISHED_STATUS;
        return {};
    }

    // m_entropy_queue is sorted by entropy, so the tied-for-lowest set is
    // exactly the run of entries at the front within `error` of the first
    // one's entropy -- walking forward and stopping at the first non-tied
    // entry correctly finds that set, since is_approx is checked against a
    // fixed reference (the true minimum) and entropy only increases from
    // there. Their order within the multimap is not meaningful (see the
    // member comment), so re-sort the collected set by grid position to
    // match the old full-scan's (x outer, z inner) iteration order exactly
    // -- otherwise near-tied entries (is_approx-equal but not
    // bit-identical) land in whatever order their tiny floating-point
    // difference happens to produce, changing which cell a given RNG draw
    // lands on even though the draw itself, and the tied *set*, match.
    m_select_candidates.clear();
    double min_entropy = m_entropy_queue.begin()->first;
    for(auto it = m_entropy_queue.begin(); it != m_entropy_queue.end() && is_approx(it->first, min_entropy); ++it){
        m_select_candidates.push_back(it->second);
    }
    std::sort(m_select_candidates.begin(), m_select_candidates.end(), [](const Vec3u& a, const Vec3u& b){
        return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
    });

    m_status = AbstractWFC::RUNNING_STATUS;
    return m_select_candidates[m_rng.next_int() % m_select_candidates.size()];
}


void WFC::requeue_cell(const Vec3u& coords){
    std::size_t idx = m_wave->index(coords.x, coords.y, coords.z);

    auto pos_it = m_entropy_pos.find(idx);
    if(pos_it != m_entropy_pos.end()){
        m_entropy_queue.erase(pos_it->second);
        m_entropy_pos.erase(pos_it);
    }

    const auto& cell = m_wave->get(coords.x, coords.y, coords.z);
    auto remaining = cell.count();
    if(remaining <= 1){
        // Resolved (1 left, entropy is exactly 0) or contradicted (0
        // left): not a selection candidate either way.
        if(remaining == 0) m_status = AbstractWFC::CONTRADICTION_STATUS;
        return;
    }

    double e = m_entropy.get_cell_entropy(coords, cell, weights);
    auto it = m_entropy_queue.emplace(e, coords);
    m_entropy_pos.emplace(idx, it);
}


void WFC::rebuild_entropy_queue(){
    m_entropy_queue.clear();
    m_entropy_pos.clear();

    for(std::size_t x = 0; x < m_wave->get_width(); x++){
    for(std::size_t y = 0; y < m_wave->get_height(); y++){
    for(std::size_t z = 0; z < m_wave->get_depth(); z++){
        requeue_cell(Vec3u(x,y,z));
    }}}
}


void WFC::collapse_cell(const Vec3u& coords, int boost_bit, double boost_factor) {
    auto[x,y,z] = coords;
    auto& cell = m_wave->get(x, y, z);

    double total = 0;
    for(std::size_t i=0; i<cell.size(); i++){
        if(cell[i])
            total += weights[i] * (static_cast<int>(i) == boost_bit ? boost_factor : 1.0);
    }

    auto r = m_rng.next_double();
    double acc = 0;
    auto selected = 0;
    for(std::size_t i=0; i<cell.size(); i++){
        if(cell[i]){
            acc += weights[i] * (static_cast<int>(i) == boost_bit ? boost_factor : 1.0) / total;
            if(r <= acc){
                selected = i;
                break;
            }
        }
    }

    // Tell EntropyMemory exactly which tiles are being removed (everything
    // that was possible except the one selected) before actually clearing
    // them, so its running sums stay correct incrementally instead of
    // needing a from-scratch recompute on the next cache miss.
    for(std::size_t i=0; i<cell.size(); i++){
        if(cell[i] && static_cast<int>(i) != selected){
            m_entropy.remove_tile(coords, i);
        }
    }

    cell.reset();
    cell[selected] = true;

    // The cell is now resolved (1 tile left): pulls it out of the entropy
    // queue, since it's no longer a selection candidate.
    requeue_cell(coords);
}


bool WFC::update_cell_state(const Vec3u& coords, CellState& cell, const TileConstraints& constraints, const CellState& neighbor) {
    // m_scratch is reused across calls (sized once in init()) instead of
    // allocating a fresh bitset every time.
    m_scratch.reset();
    for (std::size_t i = 0; i < neighbor.size(); i++) {
        if (neighbor[i]) {
            m_scratch |= constraints[i];
        }
    }

    // m_removed (also reused, no allocation) ends up holding exactly the
    // tiles this call eliminates: was possible before, isn't after. Reusing
    // it as the change signal (instead of comparing set-bit counts) lets
    // EntropyMemory be told precisely which tiles were removed, rather than
    // just "something changed".
    m_removed = cell;
    cell &= m_scratch;
    m_removed -= cell;

    if(m_removed.none()) return false;

    for(auto t = m_removed.find_first(); t != CellState::npos; t = m_removed.find_next(t)){
        m_entropy.remove_tile(coords, t);
    }
    requeue_cell(coords);
    return true;
}


void WFC::propagate_direction(const Vec3i& from, const Vec3i& to, Directions dir) {
    auto[f_x, f_y, f_z] = from;
    auto[t_x, t_y, t_z] = to;
    Vec3u dim{
        static_cast<unsigned int>(m_wave->get_width()),
        static_cast<unsigned int>(m_wave->get_height()),
        static_cast<unsigned int>(m_wave->get_depth())
    };
    if(m_periodic){
        Vec3u wrapped_to = to.wrapi(dim);
        // A size-1 axis wraps back onto the source cell itself: there is no
        // actual neighbor in this direction, so skip it rather than
        // constraining a cell against itself with a (likely empty, e.g.
        // FRONT/BACK on a 2D grid) constraint set.
        if(static_cast<Vec3i>(wrapped_to) == from) return;

        if(update_cell_state(wrapped_to, m_wave->get_wrapped(t_x, t_y, t_z), constraints.get(dir), m_wave->get_wrapped(f_x, f_y, f_z))){
            m_propagate_queue.push(static_cast<Vec3i>(wrapped_to));
        }
    }else if(m_wave->valid_coords(t_x, t_y, t_z)){
        Vec3u target = to.to_vec3u();
        if(update_cell_state(target, m_wave->get(t_x, t_y, t_z), constraints.get(dir), m_wave->get(f_x, f_y, f_z))){
            m_propagate_queue.push(to);
        }
    }
}


void WFC::propagate_constraints(const Vec3u& coords){
    // m_propagate_queue is reused across calls instead of a fresh
    // std::queue every time; it's always left empty on entry, since the
    // loop below only ever exits once it's fully drained.
    m_propagate_queue.push(static_cast<Vec3i>(coords));

    while(!m_propagate_queue.empty()){
        auto current = m_propagate_queue.front();

        propagate_direction(current, current + Vec3Constants::UP, Directions::UP);
        propagate_direction(current, current + Vec3Constants::DOWN, Directions::DOWN);
        propagate_direction(current, current + Vec3Constants::LEFT, Directions::LEFT);
        propagate_direction(current, current + Vec3Constants::RIGHT, Directions::RIGHT);
        propagate_direction(current, current + Vec3Constants::BACK, Directions::BACK);
        propagate_direction(current, current + Vec3Constants::FRONT, Directions::FRONT);

        m_propagate_queue.pop();
    }
}


void WFC::propagate_exterior(const Vec3u& coords, Directions dir, const CellState& state){
    auto[x,y,z] = coords;
    update_cell_state(coords, m_wave->get(x,y,z), constraints.get(dir), state);
    propagate_constraints(coords);
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


bool WFC::step_boosted(const Array3D<unsigned int>& boost, double factor){
    auto selected = select_cell();
    if(!selected.has_value()){
        return true;
    }
    auto[x,y,z] = selected.value();
    collapse_cell(selected.value(), boost.get(x,y,z), factor);
    propagate_constraints(selected.value());
    stepped.emit(this, m_step_counter++, selected.value());
    return false;
};


bool WFC::run() {
    while(true){
        if(step()){
            finished.emit(this);
            return m_status != AbstractWFC::CONTRADICTION_STATUS;
        };
    }
}


bool WFC::run_boosted(const Array3D<unsigned int>& boost, double factor){
    while(true){
        if(step_boosted(boost, factor)){
            finished.emit(this);
            return m_status != AbstractWFC::CONTRADICTION_STATUS;
        };
    }
}


Vec3u WFC::get_size(){
    return Vec3u(m_wave->get_width(), m_wave->get_height(), m_wave->get_depth());
}


const WaveState& WFC::get_wave() const {
    return *m_wave;
}


void WFC::set_wave(const WaveState& wave){
    *m_wave = wave;
    // Bypasses remove_tile's incremental accounting entirely (the wave is
    // replaced wholesale, not mutated tile-by-tile), so the running sums
    // -- and the entropy queue, which is driven off the same per-removal
    // events -- both need a full re-derivation from the new contents.
    m_entropy.resync(*m_wave);
    rebuild_entropy_queue();
}


}

