# pragma once
#include "abstract_wfc.hpp"
#include "array3d.hpp"
#include "random.hpp"
#include "thread_pool.hpp"
#include <vector>


namespace wfc {


class GAWFC {
public:
    using GenomeT = Array3D<unsigned int>;
    struct Individual {
        GenomeT genome;
        double fitness;
    };
    using PopulationT = std::vector<Individual>;

private:
    void setup();

protected:
    PopulationT m_current, m_candidates;
    Random m_rng;
    ThreadPool m_pool;

    TileWeights m_weights;
    AdjacencyConstraints m_constraints;
    Vec3u m_wfc_size;

    int m_seed;
    int m_max_generations;
    int m_population_size;
    double m_boost_factor;
    int m_generation_count = 0;

public:
    GAWFC(const Vec3u& wfc_size, int max_generations, int population_size, int seed, double boost_factor);

    void init_examples(const std::vector<GenomeT>& examples);

    virtual double fitness(const GenomeT& result) const = 0;
    const Individual& tournament_select(const PopulationT& pop, int k = 3);
    GenomeT crossover(const GenomeT& a, const GenomeT& b);
    void mutate(GenomeT& g, double pmut, unsigned int tile_count);
    PopulationT make_new_generation(const PopulationT& pop);
    void add_elites(const PopulationT& pop, PopulationT& next, int elite_count = 2);

    Individual run();

};


}

