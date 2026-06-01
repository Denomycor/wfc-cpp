#include "ga_wfc.hpp"
#include "abstract_wfc.hpp"
#include "wfc.hpp"
#include <algorithm>
#include <limits>
#include <vector>


namespace wfc {

GAWFC::GAWFC(const Vec3u& wfc_size, int max_generations, int population_size, int seed, double boost_factor)
:m_current(),
m_candidates(population_size), 
m_rng(seed),
m_pool(),
m_weights(), // init after having examples
m_constraints(1), // init after having examples
m_wfc_size(wfc_size),
m_seed(seed),
m_max_generations(max_generations),
m_population_size(population_size),
m_boost_factor(boost_factor)
{
    assert(boost_factor > 1.0 && "Boost factor must be greater than 1");
    assert(population_size >= 2 && "Population size must be 2 or greater");
    assert(max_generations > 0 && "Max generations must be greater than 0");
}


void GAWFC::init_examples(const std::vector<GenomeT>& examples){
    assert(static_cast<int>(examples.size()) == m_population_size && "Examples should match the population size");
    m_current.clear();
    m_current.reserve(examples.size());
    for(const auto& v: examples){
        assert(v.get_width() == m_wfc_size.x && v.get_height() == m_wfc_size.y && v.get_depth() == m_wfc_size.z &&
               "Provided examples do not match specified size");
        m_current.emplace_back<Individual>({v, 0.0});
    }
    // Derive tile weights and adjacency constraints from the first example.
    // Must be called after m_current is populated.
    setup();
}


void GAWFC::setup(){
    // Pre-compute the global tile count as the max tile ID across all examples
    // so the accumulator is sized correctly before any merging begins.
    std::size_t n_tiles = 0;
    for(const auto& ind : m_current)
        for(auto t : ind.genome)
            n_tiles = std::max(n_tiles, static_cast<std::size_t>(t) + 1);

    TileWeights weights(n_tiles, 0);
    AdjacencyConstraints constraints(n_tiles, false);

    for(const auto& ind : m_current){
        auto[w, c] = get_wfc_parameters(ind.genome);
        for(std::size_t t = 0; t < w.size(); t++)
            weights[t] += w[t];
        constraints.merge(c);
    }
    m_weights = std::move(weights);
    m_constraints = std::move(constraints);
}


GAWFC::Individual GAWFC::run(){
    Individual best_ever;
    best_ever.fitness = std::numeric_limits<double>::lowest();

    while (m_generation_count < m_max_generations) {
        const int gen_offset = m_generation_count * m_population_size;

        for(std::size_t i = 0; i < m_current.size(); i++){
            m_pool.enqueue([this, gen_offset](std::size_t i){
                WFC wfc(m_wfc_size, m_weights, m_constraints, m_seed + gen_offset + i, false);
                wfc.init();
                wfc.run_boosted(m_current[i].genome, m_boost_factor);
                // Store the genome only; fitness is evaluated serially below
                // so that overridden fitness() implementations (e.g. GDScript
                // callables) are never called from a worker thread.
                m_candidates[i].genome = wfc.get_result();
            }, i);
        }
        m_pool.wait();

        // Evaluate fitness serially on the calling thread (thread-safe for
        // user-supplied fitness functions such as GDScript callables).
        for(std::size_t i = 0; i < m_candidates.size(); i++){
            m_candidates[i].fitness = fitness(m_candidates[i].genome);
            if (m_candidates[i].fitness > best_ever.fitness)
                best_ever = m_candidates[i];
        }

        m_current = make_new_generation(m_candidates);

        generation_ended.emit(m_generation_count);
        m_generation_count++;
    }

    return best_ever;
}


const GAWFC::Individual& GAWFC::select(const PopulationT& pop, int k) {
    int best = m_rng.next_int(0, pop.size() - 1);

    for (int i = 1; i < k; i++) {
        int idx = m_rng.next_int(0, pop.size() - 1);
        if (pop[idx].fitness > pop[best].fitness) {
            best = idx;
        }
    }

    return pop[best];
}


GAWFC::GenomeT GAWFC::crossover(const GAWFC::GenomeT& a, const GAWFC::GenomeT& b) {
    GAWFC::GenomeT child(a.get_width(), a.get_height(), a.get_depth());

    const int axes = (m_wfc_size.z > 1) ? 3 : 2;
    int axis = m_rng.next_int(0, axes - 1);

    if (axis == 0) { // X split
        std::size_t split = m_rng.next_int(0, m_wfc_size.x - 1);
        for (std::size_t z = 0; z < m_wfc_size.z; z++)
        for (std::size_t y = 0; y < m_wfc_size.y; y++)
        for (std::size_t x = 0; x < m_wfc_size.x; x++) {
            child.get(x,y,z) = (x <= split) ? a.get(x,y,z) : b.get(x,y,z);
        }
    }
    else if (axis == 1) { // Y split
        std::size_t split = m_rng.next_int(0, m_wfc_size.y - 1);
        for (std::size_t z = 0; z < m_wfc_size.z; z++)
        for (std::size_t y = 0; y < m_wfc_size.y; y++)
        for (std::size_t x = 0; x < m_wfc_size.x; x++) {
            child.get(x,y,z) = (y <= split) ? a.get(x,y,z) : b.get(x,y,z);
        }
    }
    else { // Z split
        std::size_t split = m_rng.next_int(0, m_wfc_size.z - 1);
        for (std::size_t z = 0; z < m_wfc_size.z; z++)
        for (std::size_t y = 0; y < m_wfc_size.y; y++)
        for (std::size_t x = 0; x < m_wfc_size.x; x++) {
            child.get(x,y,z) = (z <= split) ? a.get(x,y,z) : b.get(x,y,z);
        }
    }

    return child;
}


void GAWFC::mutate(GAWFC::GenomeT& g, double pmut, unsigned int tile_count) {
    for (std::size_t z = 0; z < m_wfc_size.z; z++)
    for (std::size_t y = 0; y < m_wfc_size.y; y++)
    for (std::size_t x = 0; x < m_wfc_size.x; x++) {
        if (m_rng.next_double() < pmut) {
            g.get(x,y,z) = m_rng.next_int(0, tile_count - 1);
        }
    }
}


GAWFC::PopulationT GAWFC::make_new_generation(const PopulationT& pop) {
    PopulationT next;
    next.reserve(m_population_size);
    add_elites(pop, next);

    while (static_cast<int>(next.size()) < m_population_size) {
        const auto& p1 = select(pop);
        const auto& p2 = select(pop);

        GenomeT child_genome;

        if (m_rng.next_double() < 0.75) {
            child_genome = crossover(p1.genome, p2.genome);
        } else {
            child_genome = p1.genome;
        }

        mutate(child_genome, 0.04, m_weights.size());

        next.push_back({std::move(child_genome), 0.0});
    }

    return next;
}


void GAWFC::add_elites(const PopulationT& pop, PopulationT& next, int elite_count) {
    auto sorted = pop;
    std::sort(sorted.begin(), sorted.end(),
        [](auto& a, auto& b){ return a.fitness > b.fitness; });

    for (int i = 0; i < elite_count; i++) {
        next.push_back(sorted[i]);
    }
}


const Vec3u& GAWFC::get_wfc_size() const {
    return m_wfc_size;
}


int GAWFC::get_max_generations() const {
    return m_max_generations;
}


int GAWFC::get_population_size() const {
    return m_population_size;
}


int GAWFC::get_boost_factor() const {
    return m_boost_factor;
}


int GAWFC::get_generation_count() const {
    return m_generation_count;
}


}

