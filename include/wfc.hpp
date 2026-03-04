#pragma once

#include "abstract_wfc.hpp"
#include "observer.hpp"
#include <tuple>


class WFC3D : public AbstractWFC {
public:
    WaveState* m_wave;
    EntropyWFC m_entropy;
    TileWeights m_weights;
    WaveConstraints m_constraints;

public:
    Signal<WFC3D*> stepped;
    Signal<WFC3D*> finished;

    WFC3D(int width, int height, int depth, const TileWeights& weights);
    WFC3D(
        const WFC3D& p_source, 
        const std::tuple<int,int,int>& offset,
        const std::tuple<int,int,int>& length,
        const TileWeights& weights
    );

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

    void add_constraint_allow_all(TileId tile);

    std::tuple<int, int, int> select_cell();
    void collapse_cell(const std::tuple<int, int, int>& coords);
    void propagate_constraints(const std::tuple<int, int, int>& coords);

    void init() override;
    bool step() override;
    bool run() override;

    void print2D() const;

    virtual ~WFC3D();

};

