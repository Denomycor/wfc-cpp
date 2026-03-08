#pragma once
#include "abstract_wfc.hpp"
#include "array3d.hpp"
#include "observer.hpp"
#include <queue>


class WFC : public AbstractWFC {
private:
    WaveState* m_wave;
    EntropyMemory m_entropy;
    AdjacencyConstraints m_adjacency;
    TileWeights m_weights;

    void propagate_direction(const Vec3u& from, const Vec3u& to, Directions dir, std::queue<Vec3u>& queue);

public:
    Signal<WFC*> stepped;
    Signal<WFC*> finished;

    // Standard constructor
    WFC(const Vec3u& size, const TileWeights& weights);
    // Sub WFC constructor
    WFC(const WFC& view, const Vec3u offset, const Vec3u length);
    // Constructor from pre-existing AdjacencyConstraints
    WFC(const Vec3u& size, const TileWeights& weights, const AdjacencyConstraints& constraints);

    AdjacencyConstraints& get_constraints();
    TileWeights& get_weights();

    std::optional<Vec3u> select_cell();
    void collapse_cell(const Vec3u& coords);
    void propagate_constraints(const Vec3u& coords);

    void init();
    bool step();
    bool run();

    Array3D<std::size_t> get_result();
    WaveState& get_wave();

    ~WFC() override;

};

