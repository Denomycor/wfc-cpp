#pragma once
#include "abstract_wfc.hpp"
#include "array3d.hpp"
#include "observer.hpp"
#include "random.hpp"
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <queue>

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

    void propagate_direction(const Vec3i& from, const Vec3i& to, Directions dir, std::queue<Vec3i>& queue);
    bool update_cell_state(CellState& cell, const TileConstraints& constraints, const CellState& neighbor);
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

