#pragma once
#include "abstract_wfc.hpp"
#include "array3d.hpp"
#include "observer.hpp"
#include <queue>

namespace wfc {

class WFC : public AbstractWFC {
private:
    WaveState* m_wave;
    EntropyMemory m_entropy;
    AdjacencyConstraints m_adjacency;
    TileWeights m_weights;
    bool m_periodic;
    int step_counter = 0;

    void propagate_direction(const Vec3i& from, const Vec3i& to, Directions dir, std::queue<Vec3i>& queue);

public:
    Signal<WFC*, int> stepped;
    Signal<WFC*> finished;

    // Standard constructor
    WFC(const Vec3u& size, const TileWeights& weights, bool periodic = false);
    // Sub WFC constructor
    // WFC(const WFC& view, const Vec3u offset, const Vec3u length); // NOT SMART
    // Constructor from pre-existing AdjacencyConstraints
    WFC(const Vec3u& size, const TileWeights& weights, const AdjacencyConstraints& constraints, bool periodic = false);

    AdjacencyConstraints& get_constraints();
    TileWeights& get_weights();

    std::optional<Vec3u> select_cell();
    void collapse_cell(const Vec3u& coords);
    void propagate_constraints(const Vec3u& coords);

    void init();
    // void init(std::size_t id, bool value); // NOT SMART

    bool step();
    bool run();

    Array3D<std::size_t> get_result();
    WaveState& get_wave();

    ~WFC() override;

};

}

