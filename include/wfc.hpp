#pragma once
#include "abstract_wfc.hpp"
#include "array3d.hpp"
#include "observer.hpp"
#include "random.hpp"
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <map>
#include <queue>
#include <unordered_map>

namespace wfc {

class ChunkWFC;

class WFC : public AbstractWFC {
private:
    WaveState* m_wave;
    EntropyMemory m_entropy;
public:
    AdjacencyConstraints constraints;
    TileWeights weights;
private:
    Random m_rng;
    bool m_periodic;
    int m_step_counter = 0;

    // Reused scratch state for the propagation/selection hot paths (see
    // update_cell_state, propagate_constraints, select_cell in wfc.cpp) so
    // they don't heap-allocate a fresh bitset/queue/vector on every call.
    // Safe as a per-instance member: nothing calls into a single WFC
    // instance from more than one thread at a time (ChunkWFC gives each
    // worker task its own local WFC object).
    CellState m_scratch;
    // Set of tiles a single update_cell_state call actually eliminates;
    // lets EntropyMemory be told exactly which tiles were removed instead
    // of rescanning a cell's whole remaining possibility set.
    CellState m_removed;
    std::queue<Vec3i> m_propagate_queue;
    std::vector<Vec3u> m_select_candidates;

    // Indexed min-priority structure over currently-unresolved cells
    // (remaining possible tiles > 1), keyed by entropy: replaces
    // select_cell's O(N) full-grid rescan with O(log N) maintenance per
    // tile removal. m_entropy_queue is kept sorted by entropy (so the
    // lowest-entropy candidates, and any ties with it, sit at the front);
    // m_entropy_pos maps a cell's linear index to its iterator into
    // m_entropy_queue so requeue_cell can find and erase a cell's existing
    // entry in O(log N) before inserting its updated one. A cell is absent
    // from both once it resolves (1 tile left) or contradicts (0 left).
    //
    // Sort order among near-tied entries (is_approx-equal but not
    // bit-identical -- e.g. two cells that reached the same remaining tile
    // set via differently-ordered incremental subtractions) is NOT
    // meaningful: std::multimap orders purely by the raw double, so such
    // entries land wherever their tiny floating-point difference happens to
    // put them, not in any position-derived order. select_cell's walk below
    // only relies on this sort to find the tied-for-lowest *set* correctly
    // (is_approx against a fixed reference, entropy non-decreasing from
    // there) -- it does not rely on the order within that set, and
    // explicitly re-sorts by grid position afterward to match the old
    // scan's (x outer, z inner) iteration order exactly.
    std::multimap<double, Vec3u> m_entropy_queue;
    std::unordered_map<std::size_t, std::multimap<double, Vec3u>::iterator> m_entropy_pos;

    // Recomputes a cell's queue membership from its current wave state:
    // removes any existing entry, then reinserts with the current entropy
    // if it's still unresolved (more than one tile possible), or flags a
    // contradiction if it has none left. Called whenever a cell's
    // possibility set changes.
    void requeue_cell(const Vec3u& coords);
    // Repopulates the entropy queue for every cell from scratch: O(N log N),
    // used only at init() and after set_wave() (which bypasses requeue_cell's
    // per-removal incremental path entirely).
    void rebuild_entropy_queue();

    void propagate_direction(const Vec3i& from, const Vec3i& to, Directions dir);
    bool update_cell_state(const Vec3u& coords, CellState& cell, const TileConstraints& constraints, const CellState& neighbor);
    void propagate_exterior(const Vec3u& coords, Directions dir, const CellState& state);
    void set_wave(const WaveState& wave);

public:
    Signal<WFC*, int, Vec3u> stepped;
    Signal<WFC*> finished;

    // Standard constructor
    WFC(const Vec3u& size, const TileWeights& weights, unsigned int seed = 0, bool periodic = false);
    // Sub WFC constructor
    // WFC(const WFC& view, const Vec3u offset, const Vec3u length);
    // Constructor from pre-existing AdjacencyConstraints
    WFC(const Vec3u& size, const TileWeights& weights, const AdjacencyConstraints& constraints, unsigned int seed = 0, bool periodic = false);

    std::optional<Vec3u> select_cell();
    void collapse_cell(const Vec3u& coords, int boost_bit = -1, double boost_factor = 100000.0);
    void propagate_constraints(const Vec3u& coords);

    bool check_contradiction();

    void clean_cache();
    void init();

    bool step();
    bool run();

    bool step_boosted(const Array3D<unsigned int>& boost, double factor);
    bool run_boosted(const Array3D<unsigned int>& boost, double factor);

    Array3D<unsigned int> get_result();
    Vec3u get_size();
    const WaveState& get_wave() const;

    ~WFC() override;

    friend ChunkWFC;

};



}

