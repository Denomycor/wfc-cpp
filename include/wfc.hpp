#pragma once

#include "abstract_wfc.hpp"
#include "observer.hpp"
#include <tuple>


class WFC3D : public AbstractWFC {
protected:
    WaveState m_wave;
    EntropyWFC m_entropy;
    TileWeights m_weights;
    WaveConstraints m_constraints;

public:
    Signal<WFC3D*> stepped;
    Signal<WFC3D*> finished;

    WFC3D(int width, int height, int depth, const TileWeights& weights);

    void add_constraint(
        TileId tile,
        const std::vector<TileId>& up,
        const std::vector<TileId>& down,
        const std::vector<TileId>& left,
        const std::vector<TileId>& right,
        const std::vector<TileId>& front,
        const std::vector<TileId>& back
    );

    void add_constraint(
        TileId tile,
        const std::vector<TileId>& up,
        const std::vector<TileId>& down,
        const std::vector<TileId>& left,
        const std::vector<TileId>& right
    );

    std::tuple<int, int, int> select_cell();
    void collapse_cell(const std::tuple<int, int, int>& coords);
    void propagate_constraints(const std::tuple<int, int, int>& coords);

    void init() override;
    bool step() override;
    bool run() override;

};

class WFC2D : public WFC3D {
public: 
    WFC2D(int width, int height, const TileWeights& weights);
    void print2D() const;

};

