#pragma once
#include "abstract_wfc.hpp"
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include <unordered_map>


namespace wfc {


class TileNode;
struct TileNodeCache;


struct TileConnection {
    std::shared_ptr<TileNode> child;
    double weight;
};


class TileNode {
private:
    std::shared_ptr<int> m_id_counter;
    int m_id;
    std::string m_name;
    std::unordered_map<std::string, TileConnection> m_childs;
    std::vector<TileNode*> m_parents;
    std::array<boost::dynamic_bitset<>, Directions::COUNT> m_constraints;
    int m_layer;
    bool m_is_leaf = true;

    void set_constraints(TileNode& adjacent, Directions dir);
    void collect_descendants(TileNode* node, std::unordered_set<TileNode*>& out);
    bool has_valid_pair(TileNode& other, Directions dir);
    void propagate_block_up(TileNode& other, Directions dir);


public:
    TileNode();
    TileNode(TileNode* parent, const std::string& name);

    TileNode& add_child(const std::string& name, double weight);
    TileNode& add_child(TileNode& node, double weight);

    void add_constraint(TileNode& adjacent, Directions dir);
    void clean_constraints();

    double get_weight(const std::string& name) const;
    const TileNode& get_node(const std::string& name) const;
    int get_layer() const;
    int get_tree_tiles() const;

    bool is_leaf() const;
    bool is_meta() const;

    friend TileNodeCache;
};


struct TileNodeCache {
    std::vector<TileNode*> nodes;
    std::vector<std::vector<TileNode*>> layers;
    
    TileNodeCache(const TileNode& tree);

private:
    void fill(const TileNode& tree);
};


}

