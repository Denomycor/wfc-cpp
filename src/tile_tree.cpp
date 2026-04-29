#include "tile_tree.hpp"
#include "abstract_wfc.hpp"
#include <cassert>
#include <memory>
#include <unordered_set>


namespace wfc {

TileNode::TileNode()
:m_id_counter(std::make_shared<int>(0)), m_id(0), m_name("name"), m_childs(), m_parents(), m_constraints(), m_layer(0)
{}


TileNode::TileNode(TileNode* parent, const std::string& name)
:m_id_counter(parent->m_id_counter), m_id((*parent->m_id_counter)+1), m_name(name), m_childs(), m_parents({parent}), m_constraints(), m_layer(parent->m_layer+1)
{
    (*m_id_counter)++;
}


TileNode& TileNode::add_child(const std::string& name, double weight){
    m_childs[name] = TileConnection{
        std::make_shared<TileNode>(this, name),
        weight
    };
    m_is_leaf = false;
    return *(m_childs[name].child);
}


TileNode& TileNode::add_child(TileNode& node, double weight){
    assert(node.m_parents.size() > 0 && "Added node must already have at least 1 parent");
    m_childs[node.m_name] = TileConnection{
        node.m_parents[0]->m_childs[node.m_name].child,
        weight
    };
    node.m_parents.push_back(this);
    m_is_leaf = false;
    return *(m_childs[node.m_name].child);
}


double TileNode::get_weight(const std::string& name) const {
    return m_childs.at(name).weight;
}


const TileNode& TileNode::get_node(const std::string& name) const {
    return *(m_childs.at(name).child);
}


int TileNode::get_layer() const {
    return m_layer;
}


bool TileNode::is_leaf() const{
    return m_is_leaf;
}


bool TileNode::is_meta() const{
    return !m_is_leaf;
}


void TileNode::collect_descendants(TileNode* node, std::unordered_set<TileNode*>& out) {
    if (!node || out.count(node)) return;

    out.insert(node);

    for (auto& [_, c] : node->m_childs) {
        collect_descendants(c.child.get(), out);
    }
}

void TileNode::add_constraint(TileNode& adjacent, Directions dir) {
    std::unordered_set<TileNode*> descA;
    std::unordered_set<TileNode*> descB;

    collect_descendants(this, descA);
    collect_descendants(&adjacent, descB);

    for (TileNode* a : descA) {
        for (TileNode* b : descB) {
            a->set_constraints(*b, dir);
        }
    }
    propagate_block_up(adjacent, dir);
}


void TileNode::propagate_block_up(TileNode& other, Directions dir) {
    for (auto& pA : m_parents) {
        for (auto& pB : other.m_parents) {

            if (!pA->has_valid_pair(*pB, dir)) {
                pA->set_constraints(*pB, dir);

                // recurse upward
                pA->propagate_block_up(*pB, dir);
            }
        }
    }
}


bool TileNode::has_valid_pair(TileNode& other, Directions dir) {
    for (auto& [_, ca] : m_childs) {
        for (auto& [__, cb] : other.m_childs) {

            TileNode* a = ca.child.get();
            TileNode* b = cb.child.get();

            // ensure constraint exists
            if (a->m_constraints[dir].size() > b->m_id &&
                a->m_constraints[dir][b->m_id]) {
                return true;
            }
        }
    }

    return false;
}


void TileNode::set_constraints(TileNode& adjacent, Directions dir){
    if(m_constraints[dir].size() < adjacent.m_id+1){
        m_constraints[dir].resize(adjacent.m_id+1, 1);
    }
    m_constraints[dir][adjacent.m_id] = 0;
    if(adjacent.m_constraints[get_opposite(dir)].size() < m_id+1){
        adjacent.m_constraints[get_opposite(dir)].resize(m_id+1, 1);
    }
    adjacent.m_constraints[get_opposite(dir)][m_id] = 0;
}


void TileNode::clean_constraints(){
    int size = get_tree_tiles();
    for(auto& c : m_constraints){
        if(c.size() < size){
            c.resize(size,1);
        }
    }
    for(auto&[_, c] : m_childs){
        c.child->clean_constraints();
    }
}


int TileNode::get_tree_tiles() const {
    return (*m_id_counter) + 1;
}


TileNodeCache::TileNodeCache(const TileNode& tree)
: nodes(tree.get_tree_tiles())
{
    fill(tree);
}


void TileNodeCache::fill(const TileNode& tree){
    for(auto& [_, c] : tree.m_childs){
        nodes[c.child->m_id] = c.child.get();
        fill(tree);
    }
}


}

