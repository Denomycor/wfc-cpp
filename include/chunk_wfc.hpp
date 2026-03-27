#pragma once
#include "abstract_wfc.hpp"
#include "wfc.hpp"
#include "array3d.hpp"
#include "observer.hpp"
#include <functional>

namespace wfc {

class ChunkWFC {
private:
    using Writer_T = std::function<void(const Vec3u& coords, const Array3D<unsigned int>&)>;
    using Reader_T = std::function<std::optional<Array3D<unsigned int>>(const Vec3u& coords)>;

    Vec3u m_chunk_size;
    unsigned int m_seed, max_attempts = 4;

public:
    AdjacencyConstraints constraints;
    TileWeights weights;

private:
    Writer_T writer;
    Reader_T reader;

    void init_margins(WFC& wfc, const Vec3i& coords, Directions d);

public:
    Signal<WFC*> completed_chunk;

    ChunkWFC(const Vec3u& chunk_size, const TileWeights& weights, unsigned int seed = 0);
    ChunkWFC(const Vec3u& chunk_size, const TileWeights& weights, const AdjacencyConstraints& constraints, unsigned int seed = 0);

    bool generate_range(const Vec3i& from, const Vec3i& to);
    bool generate_chunk(const Vec3i& coords);

    ~ChunkWFC();

};

}

