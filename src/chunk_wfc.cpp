#include "chunk_wfc.hpp"
#include "abstract_wfc.hpp"
#include "wfc.hpp"


namespace wfc {

ChunkWFC::ChunkWFC(const Vec3u& chunk_size, const TileWeights& weights, unsigned int seed)
:m_chunk_size(chunk_size), 
m_seed(seed),
constraints(weights.size(), true),
weights(weights)
{}


ChunkWFC::ChunkWFC(const Vec3u& chunk_size, const TileWeights& weights, const AdjacencyConstraints& constraints, unsigned int seed)
:m_chunk_size(chunk_size), 
m_seed(seed),
constraints(constraints),
weights(weights)
{}


void ChunkWFC::init_margins(WFC& wfc, const Vec3i& coords, Directions d) {
    auto size = wfc.get_size();
    switch (d) {
    case UP: {
        auto result = reader({std::get<0>(coords), std::get<1>(coords) + 1, std::get<2>(coords)});
        if (result.has_value()) {
            for (size_t x = 0; x < std::get<0>(size); x++) {
            for (size_t z = 0; z < std::get<2>(size); z++) {
                Vec3u coord = {x, std::get<1>(size) - 1, z};
                wfc.init_cell(coord, result.value().get(x, std::get<1>(size) - 1, z), true);
            }}
        }
        break;
    } case DOWN: {
        auto result = reader({std::get<0>(coords), std::get<1>(coords) - 1, std::get<2>(coords)});
        if (result.has_value()) {
            for (size_t x = 0; x < std::get<0>(size); x++) {
            for (size_t z = 0; z < std::get<2>(size); z++) {
                Vec3u coord = {x, 0, z};
                wfc.init_cell(coord, result.value().get(x, 0, z), true);
            }}
        }
        break;
    } case LEFT: {
        auto result = reader({std::get<0>(coords) - 1, std::get<1>(coords), std::get<2>(coords)});
        if (result.has_value()) {
            for (size_t y = 0; y < std::get<1>(size); y++) {
            for (size_t z = 0; z < std::get<2>(size); z++) {
                Vec3u coord = {0, y, z};
                wfc.init_cell(coord, result.value().get(0, y, z), true);
            }}
        }
        break;
    } case RIGHT: {
        auto result = reader({std::get<0>(coords) + 1, std::get<1>(coords), std::get<2>(coords)});
        if (result.has_value()) {
            for (size_t y = 0; y < std::get<1>(size); y++) {
            for (size_t z = 0; z < std::get<2>(size); z++) {
                Vec3u coord = {std::get<0>(size) - 1, y, z};
                wfc.init_cell(coord, result.value().get(std::get<0>(size) - 1, y, z), true);
            }}
        }
        break;
    } case FRONT: {
        auto result = reader({std::get<0>(coords), std::get<1>(coords), std::get<2>(coords) + 1});
        if (result.has_value()) {
            for (size_t x = 0; x < std::get<0>(size); x++) {
            for (size_t y = 0; y < std::get<1>(size); y++) {
                Vec3u coord = {x, y, std::get<2>(size) - 1};
                wfc.init_cell(coord, result.value().get(x, y, std::get<2>(size) - 1), true);
            }}
        }
        break;
    } case BACK: {
        auto result = reader({std::get<0>(coords), std::get<1>(coords), std::get<2>(coords) - 1});
        if (result.has_value()) {
            for (size_t x = 0; x < std::get<0>(size); x++) {
            for (size_t y = 0; y < std::get<1>(size); y++) {
                Vec3u coord = {x, y, 0};
                wfc.init_cell(coord, result.value().get(x, y, 0), true);
            }}
        }
        break;
    } case COUNT:
        break;
    }
}


bool ChunkWFC::generate_chunk(const Vec3i& coords){
    bool success = false;
    WFC wfc(m_chunk_size, weights, constraints, m_seed, false);
    wfc.init();
    for(std::size_t d = 0; d < Directions::COUNT; d++)   {
        init_margins(wfc, coords, static_cast<Directions>(d));
    }
    auto backup_wave = wfc.get_wave();

    for(int attempt = 0; attempt < max_attempts || success; attempt++){
        success = wfc.run();
        if(!success){
            wfc.clean_cache();
            wfc.set_wave(backup_wave);
        }
    }
    return success;
}


}

