#pragma once
#include <array>
#include <cstddef>
#include <tuple>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <vector>
#include "array3d.hpp"


enum Directions {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    FRONT,
    BACK,
    COUNT,
};

// Avoid lengthy type declarations
using Vec3i = std::tuple<int,int,int>;
using Vec3u = std::tuple<std::size_t,std::size_t,std::size_t>;
using CellState = boost::dynamic_bitset<>;
using WaveState = AbstractArray3D<CellState>;
using TileWeights = std::vector<double>;
using TileConstraints = std::vector<boost::dynamic_bitset<>>;
using WaveConstraints = std::array<TileConstraints, Directions::COUNT>;


/*
 * Memoizes entropy values for WFC cells to avoid recalculating values
 */
class EntropyMemory {
private:
    Array3D<std::pair<bool, double>> m_memory;

public:
    EntropyMemory(const Vec3u& size);

    double get_cell_entropy(const Vec3u& cell, const CellState& state, const TileWeights& weights);
    void invalidate_cell(const Vec3u& cell);
    void invalidate_all();

};


/*
 * Wraps Adjacency constraint creation to later feed WFC generation
 */
class AdjacencyConstraints {
private:
    WaveConstraints m_constraints;
    std::size_t m_tiles;

public:
    explicit AdjacencyConstraints(std::size_t n_tiles, bool default_allow_all = true);

    const WaveConstraints& get() const;
    TileConstraints& get(Directions dir);
    WaveConstraints& get_data();

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
    Status get_stats() const;

    virtual ~AbstractWFC() = default;
};

