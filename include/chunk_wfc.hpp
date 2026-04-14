#pragma once
#include "abstract_wfc.hpp"
#include "utils.hpp"
#include "wfc.hpp"
#include "array3d.hpp"
#include "observer.hpp"
#include "thread_pool.hpp"
#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <sys/types.h>
#include <unordered_map>

namespace wfc {

struct ChunkWFCIO {
    virtual std::optional<Array3D<unsigned int>> reader(const Vec3i& coords) = 0;
    virtual void writer(const Vec3i& coords, const Array3D<unsigned int>& result) = 0;
    virtual ~ChunkWFCIO() = default;
};


struct NullChunkWFCIO final : public ChunkWFCIO  {
    virtual std::optional<Array3D<unsigned int>> reader(const Vec3i& coords){return {};};
    virtual void writer(const Vec3i& coords, const Array3D<unsigned int>& result){};
};


class DiskChunkWFCIO : public ChunkWFCIO {
private:
    std::shared_mutex m_mutex;
    std::filesystem::path m_index_path, m_chunks_path;
    std::unordered_map<Vec3i, uint32_t, Vec3Hash> m_index;
    Vec3u m_chunk_size;
    bool m_dirty = false;
    
    void load_index();
    void save_index();

public:
    DiskChunkWFCIO(const std::filesystem::path& index_path, const std::filesystem::path& chunks_path, const Vec3u& chunk_size);

    std::optional<Array3D<unsigned int>> reader(const Vec3i& coords) override;
    void writer(const Vec3i& coords, const Array3D<unsigned int>& result) override;
    const Vec3u& get_chunk_size() const;

    void flush_index();

    ~DiskChunkWFCIO();
};


class ChunkWFC {
private:
    Vec3u m_chunk_size;
    mutable Random m_rand; 
    unsigned int m_max_attempts;

public:
    AdjacencyConstraints constraints;
    TileWeights weights;

private:
    std::shared_ptr<ChunkWFCIO> m_handler;
    mutable ThreadPool pool;

    void init_margins(WFC& wfc, const Vec3i& coords, Directions d) const;
    std::optional<Array3D<unsigned int>> get_chunk_signal(const Vec3i& coords, bool signal) const;

public:
    Signal<Vec3i,Array3D<unsigned int>> successful_chunk;
    Signal<Vec3i> failed_chunk;

    ChunkWFC(const Vec3u& chunk_size, const TileWeights& weights, const std::shared_ptr<ChunkWFCIO>& p_handler, unsigned int max_attempts = 4, unsigned int seed = 0);
    ChunkWFC(const Vec3u& chunk_size, const TileWeights& weights, const AdjacencyConstraints& constraints, const std::shared_ptr<ChunkWFCIO>& p_handler, unsigned int max_attempts = 4, unsigned int seed = 0);

    void generate_range(const Vec3i& from, const Vec3i& to) const;
    std::optional<Array3D<unsigned int>> get_chunk(const Vec3i& coords) const;

    const Vec3u& get_chunk_size() const;

    ~ChunkWFC();

};

}

