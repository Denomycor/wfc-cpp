#include "chunk_wfc.hpp"
#include "abstract_wfc.hpp"
#include "utils.hpp"
#include "wfc.hpp"
#include <cstdint>
#include <fstream>
#include <sys/types.h>

namespace wfc {

ChunkWFC::ChunkWFC(const Vec3u& chunk_size, const TileWeights& weights, const Writer_T& p_writer, const Reader_T& p_reader, unsigned int max_attempts, unsigned int seed)
:m_chunk_size(chunk_size), 
m_seed(seed),
m_max_attempts(max_attempts),
constraints(weights.size(), true),
weights(weights),
writer(p_writer),
reader(p_reader),
pool()
{}


ChunkWFC::ChunkWFC(const Vec3u& chunk_size, const TileWeights& weights, const AdjacencyConstraints& constraints, const Writer_T& p_writer, const Reader_T& p_reader, unsigned int max_attempts, unsigned int seed)
:m_chunk_size(chunk_size), 
m_seed(seed),
m_max_attempts(max_attempts),
constraints(constraints),
weights(weights),
writer(p_writer),
reader(p_reader),
pool()
{}


void ChunkWFC::init_margins(WFC& wfc, const Vec3i& coords, Directions d) {
    auto size = wfc.get_size();
    switch (d) {
    case UP: {
        auto result = reader(coords + Vec3Constants::UP);
        if (result.has_value()) {
            for (unsigned int x = 0; x < size.x; x++) {
            for (unsigned int z = 0; z < size.z; z++) {
                Vec3u coord = {x, 0, z};
                wfc.init_cell(coord, result->get(x, size.y - 1, z), true);
            }}
        }
        break;
    } case DOWN: {
        auto result = reader(coords + Vec3Constants::DOWN);
        if (result.has_value()) {
            for (unsigned int x = 0; x < size.x; x++) {
            for (unsigned int z = 0; z < size.z; z++) {
                Vec3u coord = {x, size.y - 1, z};
                wfc.init_cell(coord, result->get(x, 0, z), true);
            }}
        }
        break;
    } case LEFT: {
        auto result = reader(coords + Vec3Constants::LEFT);
        if (result.has_value()) {
            for (unsigned int y = 0; y < size.y; y++) {
            for (unsigned int z = 0; z < size.z; z++) {
                Vec3u coord = {0, y, z};
                wfc.init_cell(coord, result->get(size.x - 1, y, z), true);
            }}
        }
        break;
    } case RIGHT: {
        auto result = reader(coords + Vec3Constants::RIGHT);
        if (result.has_value()) {
            for (unsigned int y = 0; y < size.y; y++) {
            for (unsigned int z = 0; z < size.z; z++) {
                Vec3u coord = {size.x - 1, y, z};
                wfc.init_cell(coord, result->get(0, y, z), true);
            }}
        }
        break;
    } case FRONT: {
        auto result = reader(coords + Vec3Constants::FRONT);
        if (result.has_value()) {
            for (unsigned int x = 0; x < size.x; x++) {
            for (unsigned int y = 0; y < size.y; y++) {
                Vec3u coord = {x, y, size.z - 1};
                wfc.init_cell(coord, result->get(x, y, 0), true);
            }}
        }
        break;
    } case BACK: {
        auto result = reader(coords +  Vec3Constants::BACK);
        if (result.has_value()) {
            for (unsigned int x = 0; x < size.x; x++) {
            for (unsigned int y = 0; y < size.y; y++) {
                Vec3u coord = {x, y, 0};
                wfc.init_cell(coord, result->get(x, y, size.z - 1), true);
            }}
        }
        break;
    } case COUNT:
        break;
    }
}


void ChunkWFC::generate_range(const Vec3i& from, const Vec3i& to){
    auto[fx, fy, fz] = from;
    auto[tx, ty, tz] = to;
    std::vector<std::pair<Vec3i,std::future<std::optional<Array3D<unsigned int>>>>> thread_results;
    thread_results.reserve((tx-fx)*(ty-fy)*(tz-fz)/2);

    for(int x = fx; x < tx; x++){
    for(int y = fy; y < ty; y++){
    for(int z = fz; z < tz; z++){
        if((x+y+z) & 1){
            thread_results.emplace_back(Vec3i{x,y,z}, pool.enqueue([this,x,y,z](){return this->get_chunk_signal({x,y,z}, false);}));
        }
    }}}

    for(auto& pair : thread_results){
        auto&[coords, fut] = pair;
        auto result = fut.get();
        if(result.has_value()){
            completed_chunk.emit(coords, result.value());
        }
    }
    thread_results.clear();

    for(int x = fx; x < tx; x++){
    for(int y = fy; y < ty; y++){
    for(int z = fz; z < tz; z++){
        if(!((x+y+z) & 1)){
            thread_results.emplace_back(Vec3i{x,y,z}, pool.enqueue([this,x,y,z](){return this->get_chunk_signal({x,y,z}, false);}));
        }
    }}}

    for(auto& pair : thread_results){
        auto&[coords, fut] = pair;
        auto result = fut.get();
        if(result.has_value()){
            completed_chunk.emit(coords, result.value());
        }
    }
}


std::optional<Array3D<unsigned int>> ChunkWFC::get_chunk_signal(const Vec3i& coords, bool signal){
    auto result = reader(coords);
    if(result.has_value()){
        return result.value();
    }else{
        WFC wfc(m_chunk_size, weights, constraints, m_seed, false);
        wfc.init();
        // Avoid 3D neighboords if this is 2D
        unsigned int count = Directions::COUNT + (m_chunk_size.z == 1 ? - 2 : 0); 
        for(unsigned int d = 0; d < count; d++)   {
            init_margins(wfc, coords, static_cast<Directions>(d));
        }
        auto backup_wave = wfc.get_wave();

        for(unsigned int attempt = 0; attempt < m_max_attempts; attempt++){
            bool success = wfc.run();
            if(success){
                auto result = wfc.get_result();
                writer(coords, result);
                if(signal) completed_chunk.emit(coords, result);
                return wfc.get_result();
            }else{
                wfc.clean_cache();
                wfc.set_wave(backup_wave);
            }
        }
        return {};
    }
}


std::optional<Array3D<unsigned int>> ChunkWFC::get_chunk(const Vec3i& coords){
    return get_chunk_signal(coords, true);
}


ChunkWFC::~ChunkWFC(){

}


DataChunkWFC::DataChunkWFC(const std::filesystem::path& index_path, const std::filesystem::path& chunks_path, const Vec3u& chunk_size)
:m_mutex(),
m_index_path(index_path),
m_chunks_path(chunks_path),
m_index(),
m_chunk_size(chunk_size)
{
    load_index();
}


void DataChunkWFC::load_index(){
    std::ifstream file(m_index_path, std::ios::binary);
    if (!file.is_open()) {
        return;
    }

    u_int32_t index_size;

    file.read(reinterpret_cast<char*>(&index_size), sizeof(index_size));

    file.read(reinterpret_cast<char*>(&m_chunk_size.x), sizeof(m_chunk_size.x));
    file.read(reinterpret_cast<char*>(&m_chunk_size.y), sizeof(m_chunk_size.y));
    file.read(reinterpret_cast<char*>(&m_chunk_size.z), sizeof(m_chunk_size.z));

    if (!file) {
        throw std::runtime_error("Failed to read index header (corrupt file)");
    }

    m_index.clear();
    m_index.reserve(static_cast<size_t>(index_size));

    for (uint64_t i = 0; i < index_size; i++) {
        int32_t x, y, z;
        uint32_t offset;

        file.read(reinterpret_cast<char*>(&x), sizeof(x));
        file.read(reinterpret_cast<char*>(&y), sizeof(y));
        file.read(reinterpret_cast<char*>(&z), sizeof(z));
        file.read(reinterpret_cast<char*>(&offset), sizeof(offset));

        if (!file) {
            throw std::runtime_error("Corrupted index file (unexpected EOF)");
        }

        m_index.emplace(Vec3i{x, y, z}, offset);
    }

    file.close();
}


void DataChunkWFC::save_index(){
    std::ofstream file(m_index_path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open index file for writing");
    }

    std::size_t index_size = m_index.size();
    Vec3u& chunk_size = m_chunk_size;

    file.write(reinterpret_cast<const char*>(&index_size), sizeof(index_size));

    file.write(reinterpret_cast<const char*>(&chunk_size.x), sizeof(chunk_size.x));
    file.write(reinterpret_cast<const char*>(&chunk_size.y), sizeof(chunk_size.y));
    file.write(reinterpret_cast<const char*>(&chunk_size.z), sizeof(chunk_size.z));

    for (const auto& [coords, offset] : m_index) {
        int32_t x = coords.x;
        int32_t y = coords.y;
        int32_t z = coords.z;

        file.write(reinterpret_cast<const char*>(&x), sizeof(x));
        file.write(reinterpret_cast<const char*>(&y), sizeof(y));
        file.write(reinterpret_cast<const char*>(&z), sizeof(z));

        file.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
    }

    file.close();
}


std::optional<Array3D<unsigned int>> DataChunkWFC::reader_imp(const Vec3i& coords){
    std::shared_lock lock(m_mutex);

    auto it = m_index.find(coords);
    if (it == m_index.end()) {
        return {};
    }

    uint32_t offset = it->second;

    std::ifstream file(m_chunks_path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    file.seekg(offset);
    if (!file) {
        return {};
    }

    Array3D<unsigned int> result(
        m_chunk_size.x,
        m_chunk_size.y,
        m_chunk_size.z
    );

    file.read(
        reinterpret_cast<char*>(result.data()),
        result.byte_size()
    );

    if (!file) {
        return {};
    }

    return result;
}


void DataChunkWFC::writer_imp(const Vec3i& coords, const Array3D<unsigned int>& result){
    std::unique_lock lock(m_mutex);

    std::fstream file(m_chunks_path, std::ios::binary | std::ios::in | std::ios::out);

    if (!file.is_open()) {
        file.open(m_chunks_path, std::ios::binary | std::ios::out);
        file.close();
        file.open(m_chunks_path, std::ios::binary | std::ios::in | std::ios::out);
    }

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open chunk file");
    }

    uint32_t offset;
    auto it = m_index.find(coords);

    if (it != m_index.end()) {
        offset = it->second;
    } else {
        file.seekp(0, std::ios::end);
        offset = static_cast<uint32_t>(file.tellp());
        m_index[coords] = offset;
        m_dirty = true;
    }

    file.seekp(offset);

    file.write(
        reinterpret_cast<const char*>(result.data()),
        result.byte_size()
    );

    if (!file) {
        throw std::runtime_error("Failed to write chunk data");
    }

    file.close();
}


void DataChunkWFC::flush_index(){
    if (m_dirty) save_index();
}


DataChunkWFC::~DataChunkWFC(){
    flush_index();
}


}

