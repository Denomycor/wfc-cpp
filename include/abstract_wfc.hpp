#pragma once
#include <array>
#include <cstddef>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <vector>
#include "utils.hpp"
#include "array3d.hpp"


namespace wfc{

enum Directions {
    UP,
    RIGHT,
    DOWN,
    LEFT,
    FRONT,
    BACK,
    COUNT,
};

enum Variants2D {
    IDENTITY,
    ROT90,
    ROT180,
    ROT270,
    VFLIP,
    HFLIP,
    HFLIPROT90,
    HFLIPROT270,
};


// Avoid lengthy type declarations
using CellState = boost::dynamic_bitset<>;
using WaveState = Array3D<CellState>;
using TileWeights = std::vector<double>;
using TileLabels = std::vector<std::string>;
using TileConstraints = std::vector<boost::dynamic_bitset<>>;
using WaveConstraints = std::array<TileConstraints, Directions::COUNT>;


Directions get_opposite(Directions dir);


/*
 * Memoizes entropy values for WFC cells to avoid recalculating values.
 *
 * The cache miss path itself is incremental rather than a from-scratch
 * recompute: reset() precomputes each tile's weight*log(weight) term once
 * and seeds every cell's running (sum of weights, sum of weight*log(weight))
 * assuming all tiles are possible; remove_tile() then adjusts those sums by
 * a simple subtraction whenever a specific tile stops being possible
 * somewhere, so a cache miss only ever needs one log() call instead of a
 * loop over every still-possible tile.
 */
class EntropyMemory {
private:
    Array3D<std::pair<bool, double>> m_memory;
    TileWeights m_weights;
    std::vector<double> m_weight_log_weight;
    Array3D<double> m_sum_weights;
    Array3D<double> m_sum_weight_log_weights;

public:
    EntropyMemory(const Vec3u& size);

    // (Re)seeds every cell's running sums as if all tiles were possible,
    // and precomputes weights[t]*log(weights[t]) once per tile. Must be
    // called before get_cell_entropy/remove_tile are used (WFC::init()
    // does this), and again any time `weights` changes.
    void reset(const TileWeights& weights);

    // Fully re-derives every cell's running sums directly from `wave`'s
    // actual contents, for when the wave is replaced wholesale (e.g.
    // WFC::set_wave), which bypasses remove_tile's incremental accounting.
    // O(cells x tiles), same cost class as the old from-scratch recompute,
    // but paid once here rather than repeatedly per cache miss.
    void resync(const WaveState& wave);

    double get_cell_entropy(const Vec3u& cell, const CellState& state, const TileWeights& weights);

    // Records that `tile` is no longer possible at `cell`: adjusts that
    // cell's running sums (skipping non-positive-weight tiles, matching
    // get_cell_entropy's existing exclusion of them) and invalidates the
    // cached value.
    void remove_tile(const Vec3u& cell, std::size_t tile);

    void invalidate_cell(const Vec3u& cell);
    void invalidate_all();

};


/*
 * Wraps Adjacency constraint creation and manipulation
 */
class AdjacencyConstraints {
private:
    WaveConstraints m_constraints;
    std::size_t m_tiles;

    auto add_new_id(TileWeights& weights);

public:
    explicit AdjacencyConstraints(std::size_t n_tiles, bool default_allow_all = true);

    const WaveConstraints& get() const;
    const TileConstraints& get(Directions dir) const;

    void change_rule(std::size_t id, Directions dir, std::size_t n_id, bool value);
    void change_all_rules(bool value);
    void change_all_rules_tile(std::size_t id, bool value);
    void change_all_rules_tile_neighbor(std::size_t id, std::size_t n_id, bool value);
    void merge(const AdjacencyConstraints& other);

    int generate_variant(std::size_t id, Variants2D type, TileWeights& weights);
};


/*
 * Base class for all WFC variations, basically a constraint statisfaction problem wrapper
 */
class AbstractWFC{
public:
    enum Status {
        NOT_INIT_STATUS,
        READY_STATUS,
        RUNNING_STATUS,
        FINISHED_STATUS,
        CONTRADICTION_STATUS
    };

protected:
    Status m_status = NOT_INIT_STATUS;

public:
    Status get_status() const;

    virtual ~AbstractWFC() = default;
};

std::pair<TileWeights, AdjacencyConstraints> get_wfc_parameters(const Array3D<unsigned int>& map);

}

