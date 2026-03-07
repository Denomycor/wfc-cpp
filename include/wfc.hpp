#pragma once
#include "abstract_wfc.hpp"
#include "array3d.hpp"
#include "observer.hpp"


class WFC : public AbstractWFC {
private:
    WaveState* m_wave;
    EntropyMemory m_entropy;
    AdjacencyConstraints m_adjacency;
    TileWeights m_weights;

public:
    Signal<WFC*> stepped;
    Signal<WFC*> finished;

    // Standard constructor
    WFC(const Vec3u& size, const TileWeights& weights);
    // Sub WFC constructor
    WFC(const WFC& view, const Vec3u offset, const Vec3u length);

    AdjacencyConstraints& get_constraints();

    Vec3u select_cell();
    void collapse_cell(const Vec3u& coords);
    void propagate_constraints(const Vec3u& coords);

    void init();
    bool step();
    bool run();

    Array3D<std::size_t> get_result();
    WaveState& get_wave();

    ~WFC() override;

};

