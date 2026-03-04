#pragma once

#include "array3d.hpp"
#include <unordered_map>
#include <vector>

enum Directions {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    FRONT,
    BACK
};

using TileId = int;
using CellState = std::unordered_map<TileId, bool>;
using WaveState = Array3D<CellState>;
using TileWeights = std::unordered_map<TileId, double>;
using TileConstraints = std::unordered_map<TileId, std::vector<TileId>>;
using WaveConstraints = std::unordered_map<Directions, TileConstraints>;

class EntropyWFC {
private:
    Array3D<std::pair<bool, double>> entropy_memory;

public:
    EntropyWFC(int width, int height, int depth);

    double get_cell_entropy(int x, int y, int z, const CellState& cell, const TileWeights& weights);
    void invalidate_cell(int x, int y, int z);
    void invalidate_all();
};

class AbstractWFC {
protected:
    int m_status;

public:
    enum Satus {
        NOT_INIT_STATUS,
        READY_STATUS,
        RUNNING_STATUS,
        FINISHED_STATUS,
        CONTRADICTION_STATUS
    };

    AbstractWFC();

    virtual void init() = 0;
    virtual bool step() = 0;
    virtual bool run() = 0;

    int get_status();

};

