#pragma once
#include "abstract_wfc.hpp"
#include "wfc.hpp"
#include "array3d.hpp"
#include "observer.hpp"
#include <functional>

namespace wfc {

class ChunkWFC {
private:
    Vec3u chunk_size;

public:
    AdjacencyConstraints constraints;
    TileWeights weights;

private:
    std::function<void(const Array3D<unsigned int>&)> writer;
    std::function<std::optional<Array3D<unsigned int>>()> reader;

public:
    Signal<WFC*> completed_chunk;

    ChunkWFC(const Vec3u& chunk_size, const TileWeights& weights, unsigned int seed = 0);
    ChunkWFC(const Vec3u& chunk_size, const TileWeights& weights, const AdjacencyConstraints& constraints, unsigned int seed = 0);

    bool generate_range(const Vec3u& from, const Vec3u& to);
    bool generate_chunk(const Vec3u& coords);

    ~ChunkWFC();

};

}

