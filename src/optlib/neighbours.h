#ifndef NEIGHBOURS_H
#define NEIGHBOURS_H

#include "coordinates.h"
#include <deque>
#include <list>

/// Represents a tree.
template<typename T>
class tree
{
  public:
    class iterator;
    /// Data at current node.
    T data;
    /// Node that lies above.
    tree& parent;
    /// Position compared to parent, content is undefined if this is root.
    unsigned address;
    /// Current node lying at depth, is 0 for the root.
    unsigned depth;
    /// Nodes that lie below.
    std::list<tree<T>> childrens;

    /// Construct new root.
    tree(T data = T())
      : data(data)
      , parent(*this)
      , depth(0)
    {}

    tree(const tree& t) = delete;

    tree(tree&& t)
      : data(std::move(t.data))
      , parent(t.parent)
      , address(t.address)
      , depth(t.depth)
      , childrens(std::move(t.childrens))
    {}

    /// Construct new node. Normally you should only need a root and add_child().
    tree(T data, tree& parent)
      : data(data)
      , parent(parent)
      , address(parent.childrens.size())
      , depth(parent.depth + 1){};

    /// Appends a new node to the list of childrens and returns a reference to it (if needed).
    tree& add_child(T data)
    {
        childrens.push_back(tree(data, *this));
        return childrens.back();
    }

    /// Returns all datas encountered when walking from the root to this node. (Without the data from the root).
    void data_until_leaf(std::list<T>& data)
    {
        tree* current = this;
        while (&current->parent != current) {
            data.push_front(current->data);
            current = &current->parent;
        }
    }

    /// Returns a iterator pointing at roots first node.
    iterator begin() { return { this, false }; }

    /// Returns a iterator pointing at roots first leaf.
    iterator leaf_begin() { return { this, true }; }

    /// Returns a iterator pointing towards nothing.
    iterator end() { return { nullptr, false }; }

    /// Returns a iterator pointing towards nothing.
    iterator leaf_end() { return { nullptr, true }; }

    /// @brief A Depth-first iterator for the tree class.
    ///
    /// Use ++ and * to navigate through the nodes (Root excepted).
    /// 2 Modes : iterating on all nodes or iterating on all leafs.
    class iterator
    {
      public:
        /// needed for stl-functions
        using difference_type = unsigned;
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using pointer = T*;
        using reference = T&;

        /// Contains all nodes from root to actual node.
        std::list<typename std::list<tree>::iterator> address;
        /// Knows when some node doesn't have any brother.
        std::list<typename std::list<tree>::iterator> address_end;
        bool only_leafs;

        /// Iterator using start as root.
        iterator(tree* start, bool only_leafs)
          : only_leafs(only_leafs)
        {
            if (start != nullptr) {
                address.push_back(start->childrens.begin());
                address_end.push_back(start->childrens.end());

                if (only_leafs) {
                    while (this->go_down_one_step()) {
                    }
                }
            }
        }

        /// Tries to go to right brother of current node. If this brother doesn't exist it will be replaced by one brother of the parent/grand-parent/... Returns false when all the tree has been traversed.
        bool go_left_one_step()
        {
            bool ret = true;
            //go up until we can go left
            while ((ret = !address.empty()) &&
                   ++(address.back()) == address_end.back()) {
                address.pop_back();
                address_end.pop_back();
            }
            return ret;
        }

        /// Tries to go to the first child of current node, returns false when it doesn't exist.
        bool go_down_one_step()
        {
            if (address.back()->childrens.empty()) {
                return false;
            } else {
                address_end.push_back(address.back()->childrens.end());
                address.push_back(address.back()->childrens.begin());
                return true;
            }
        }

        /// Comparison needed for iterators.
        bool operator!=(iterator it) { return !(*this == it); }

        /// Comparison needed for iterators.
        bool operator==(iterator it)
        {
            return address.size() == it.address.size() &&
                   std::equal(address.begin(),
                              address.end(),
                              it.address.begin(),
                              it.address.end());
        }

        /// Goes to the next node (in a depth-first fashion);
        iterator& operator++()
        {
            if (only_leafs) {
                if (this->go_left_one_step()) {
                    while (this->go_down_one_step()) {
                    }
                }
            } else {
                //go up or down when failing
                go_down_one_step() || go_left_one_step();
            }
            return *this;
        }

        /// Returns current node.
        tree& operator*()
        {
            assert(!address.empty());
            return *address.back();
        }
    };
};

/// @brief Helps building up trees.
///
/// Can be used to generate trees, such that the values from root to leaf makes sense to the Unary-Predicate. Will be used to generate neighbours: the data from root to leaf will represent a time-of-flight that needs to be valid (checked by the predicate). May generate big trees.
template<typename tree, typename InputIt, typename UnaryPredicate>
class builder
{
  public:
    /// The node to which the childs are appended.
    tree& root;
    /// The iteratorstart of the current time-of-flight, which forms the point where we will find neighbours.
    InputIt data;
    /// The iteratorend of the current time-of-flight, which forms the point where we will find neighbours.
    InputIt data_end;
    /// Controls which childs are created.
    UnaryPredicate f;
    builder(tree& root, InputIt data, InputIt data_end, UnaryPredicate f)
      : root(root)
      , data(data)
      , data_end(data_end)
      , f(f)
    {}

    /// Generates (in the worstcase) all radius ** std::distance(data, data_end) nodes ands appends it to root. Because valid time-of-flights only have 2 degrees of freedoms, it should only generate (radius ** 2) * (2 ** (std::distance(data, data_end) - 2)) nodes (with 2 entries, the time-of-flight already defines one point and hopefully the second part doesn't get too big).
    void build(int radius)
    {
        std::vector<tree*> current_nodes;
        std::vector<tree*> nodes_for_next_gen;

        current_nodes.push_back(&root);

        for (; data != data_end; ++data) {   // O(n)
            while (!current_nodes.empty()) { // O((2*r)^n)
                tree& current = *current_nodes[current_nodes.size() - 1];

                for (int r = -radius; r <= radius; r++) { // O(2*r)
                    if (r == 0) {
                        continue;
                    }
                    if (f(current, *data + r)) { // O(n)
                        nodes_for_next_gen.push_back(
                          &current.add_child(*data + r));
                    }
                }

                current_nodes.pop_back();
            }
            std::swap(nodes_for_next_gen, current_nodes);
        }
    }
};

/// Finds the neighbours of a given time of flight. The difficulty is to return only valid time of flights.
class neighbours
{
  public:
    double element_pitch_in_tacts;

    /// Needs pitch for time-of-flight-cosinus checks.
    neighbours(double element_pitch_in_tacts);

    /// Given to the tree builder to find the neighbours.
    template<typename InputIt>
    bool tof_feasible_in_range(InputIt start, InputIt end, unsigned n)
    {
        return coordinate_tools::tof_feasible_in_range(
          start, end, n, element_pitch_in_tacts);
    }

    /// @brief Computes the neighbours.
    ///
    /// CB is called on every generated tree leaf (that correspond to some valid time-of-flight). Make sure that nodes are passed by reference to cb.
    template<typename CB>
    void all_in_circle(time_of_flight& center, unsigned radius, CB cb)
    {
        tree<unsigned> root;

        builder b{ root,
                   center.diagonal_begin(),
                   center.diagonal_end(),
                   [&](tree<unsigned>& node, unsigned current) {
                       std::list<unsigned> entries;
                       node.data_until_leaf(entries);
                       entries.push_back(current);
                       return tof_feasible_in_range(
                         entries.begin(), entries.end(), entries.size());
                   } };
        b.build(radius);

        std::for_each(root.leaf_begin(), root.leaf_end(), cb);
    }
};
#endif
