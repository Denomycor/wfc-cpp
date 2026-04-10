#pragma once
#include "abstract_wfc.hpp"
#include "utils.hpp"
#include "wfc.hpp"
#include "array3d.hpp"
#include "observer.hpp"
#include "thread_pool.hpp"
#include <filesystem>
#include <functional>
#include <shared_mutex>
#include <sys/types.h>
#include <unordered_map>

namespace wfc {

class ChunkWFC {
private:
    Vec3u m_chunk_size;
    mutable Random m_rand; 
    unsigned int m_max_attempts;

public:
    using Writer_T = std::function<void(const Vec3i&, const Array3D<unsigned int>&)>;
    using Reader_T = std::function<std::optional<Array3D<unsigned int>>(const Vec3i&)>;
    using Exists_T = std::function<bool(const Vec3i&)>;

    AdjacencyConstraints constraints;
    TileWeights weights;

private:
    Writer_T writer;
    Reader_T reader;
    mutable ThreadPool pool;

    void init_margins(WFC& wfc, const Vec3i& coords, Directions d) const;
    std::optional<Array3D<unsigned int>> get_chunk_signal(const Vec3i& coords, bool signal) const;

public:
    Signal<Vec3i,Array3D<unsigned int>> successful_chunk;
    Signal<Vec3i> failed_chunk;

    ChunkWFC(const Vec3u& chunk_size, const TileWeights& weights, const Writer_T& p_writer, const Reader_T& p_reader, unsigned int max_attempts = 4, unsigned int seed = 0);
    ChunkWFC(const Vec3u& chunk_size, const TileWeights& weights, const AdjacencyConstraints& constraints, const Writer_T& p_writer, const Reader_T& p_reader, unsigned int max_attempts = 4, unsigned int seed = 0);

    void generate_range(const Vec3i& from, const Vec3i& to) const;
    std::optional<Array3D<unsigned int>> get_chunk(const Vec3i& coords) const;

    const Vec3u& get_chunk_size() const;

    ~ChunkWFC();

};


class DataChunkWFC {
private:
    std::shared_mutex m_mutex;
    std::filesystem::path m_index_path, m_chunks_path;
    std::unordered_map<Vec3i, uint32_t, Vec3Hash> m_index;
    Vec3u m_chunk_size;
    bool m_dirty = false;
    
    std::optional<Array3D<unsigned int>> reader_imp(const Vec3i& coords);
    void writer_imp(const Vec3i& coords, const Array3D<unsigned int>& result);

    void load_index();
    void save_index();

public:
    DataChunkWFC(const std::filesystem::path& index_path, const std::filesystem::path& chunks_path, const Vec3u& chunk_size);

    ChunkWFC::Reader_T reader = [this](const Vec3i& coords){
        return this->reader_imp(coords);
    };

    ChunkWFC::Writer_T writer = [this](const Vec3i& coords, const Array3D<unsigned int>& result){
        this->writer_imp(coords, result);
    };

    void flush_index();

    const Vec3u& get_chunk_size();

    ~DataChunkWFC();
};

}

