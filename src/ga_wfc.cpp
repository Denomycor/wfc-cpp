#include "ga_wfc.hpp"
#include "abstract_wfc.hpp"
#include "wfc.hpp"
#include <algorithm>
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
    m_current.reserve(examples.size());
    for(const auto& v: examples){
        assert(v.get_width() == m_wfc_size.x && v.get_height() == m_wfc_size.y && v.get_depth() == m_wfc_size.z &&
               "Provided examples do not match specified size");
        m_current.emplace_back<Individual>({v, 0.0});
    }
}


void GAWFC::setup(){
    auto[weights, constraints] = get_wfc_parameters(m_current[0].genome);
    m_weights = std::move(weights);
    m_constraints = std::move(constraints);
}


GAWFC::Individual GAWFC::run(){
    while (m_generation_count < m_max_generations) {

        for(std::size_t i = 0; i < m_current.size(); i++){
            m_pool.enqueue([this](std::size_t i){
                WFC wfc(m_wfc_size, m_weights, m_constraints, m_seed + i, false);
                wfc.init();
                wfc.run_boosted(m_current[i].genome, m_boost_factor);
                auto result = wfc.get_result();
                m_candidates[i] = {std::move(result), fitness(result)};
            }, i);
        }
        m_pool.wait();

        m_current = make_new_generation(m_candidates);

        m_generation_count++;
    }

    auto it = std::max_element(
        m_current.begin(),
        m_current.end(),
        [](const Individual& a, const Individual& b) {
            return a.fitness < b.fitness;
        }
    );

    Individual* best = (it != m_current.end()) ? &*it : &(m_current[0]);
    return *best;
}


const GAWFC::Individual& GAWFC::tournament_select(const PopulationT& pop, int k) {
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

    int axis = m_rng.next_int(0, 2);

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
        const auto& p1 = tournament_select(pop);
        const auto& p2 = tournament_select(pop);

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


}

