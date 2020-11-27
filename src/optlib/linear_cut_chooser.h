#ifndef LINEAR_CUT_CHOOSER
#define LINEAR_CUT_CHOOSER

#include "cut_chooser.h"
#include "linear_expression.h"
#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <vector>

/// Chooses Cut in linear runtime.
struct linear_cut_chooser : cut_chooser
{
    /// @brief Used in bookkeeping. Represents a x_k, y_l or z_f.
    ///
    /// If this is a x_k, then all y_l and z_f appearing in x_k's constraints are referenced in the vectors.
    /// Analoguous for y_l.
    /// A z_f does not reference anything.
    struct constraint_part
    {
        constraint_part(unsigned index);
        /// The index (k for x_k, l for y_l and f for z_f).
        unsigned index;

        /// To decide if the algorithm already saw this value or not.
        enum status
        {
            CHOOSEN,
            NOT_CHOOSEN,
            NOT_DECIDED_YET
        } taken = NOT_DECIDED_YET;

        /// Represents for an x_k or y_f the corresponding y_f or x_k and the corresponding z_f for the combination. Only contains z_{f+1} for an z_f in z.
        struct neighbour
        {
            /// This contains the corresponding positions in bookkeeping-struct of y_l if this is an x_k OR the x_k if this is an y_l. It is empty for z_f. Empty for a z_f.
            std::weak_ptr<constraint_part> x_or_y;
            /// Contains the smallest z for x_or_y.
            std::weak_ptr<constraint_part> z;
            /// Contains the number of z for x_or_y.
            unsigned z_count = 0;
        };

        /// The neighbours.
        std::vector<neighbour> neighbours;

        /// Computes the intersection of all z_f between this and the other, and checks if all are != CHOOSEN. If this is the case, then it will return the sum of all relaxation_values of the z_f, else it returns std::nullopt.
        std::optional<double> all_z_f_in_intersection_non_choosen(
          slave_problem::relaxation_values values,
          unsigned element,
          neighbour other);
        /// Sets all z_f to choosen from a neighbour.
        void choose_all_z_f_in_intersection(constraint_part::neighbour other);

        /// Returns the next z, asserts if not called for a z.
        std::weak_ptr<constraint_part> next_z();
    };

    /// Allows fast O(1) constraint access using constraint_part.
    struct bookkeeping
    {
        /// A pointer on the list xs, ys or zs.
        struct pointer
        {
            enum name
            {
                X,
                Y,
                Z
            } name;
            unsigned position;
        };

        /// The pointer on the x list.
        std::optional<pointer> x = { { pointer::X, 0 } };
        /// The pointer on the y list.
        std::optional<pointer> y = { { pointer::Y, 0 } };
        /// The pointer on the z list.
        std::optional<pointer> z = { { pointer::Z, 0 } };

        /// Contains the constraint_parts for x_k.
        std::vector<std::shared_ptr<constraint_part>> xs;
        /// Contains the constraint_parts for x_k.
        std::vector<std::shared_ptr<constraint_part>> ys;
        /// Contains the constraint_parts for x_k.
        std::vector<std::shared_ptr<constraint_part>> zs;

        /// Used for duplicate-removing of accumulated_cuts.
        std::map<unsigned, unsigned> k_indexes_to_positions;
        /// Used for duplicate-removing of accumulated_cuts.
        std::map<unsigned, unsigned> l_indexes_to_positions;
        /// Used for duplicate-removing of accumulated_cuts.
        std::map<unsigned, unsigned> f_indexes_to_positions;

        slave_problem::relaxation_values relaxation_values;
        indexes& current_indexes;
        current_elements& elements;

        bookkeeping(current_elements& current_elements,
                    indexes& indexes,
                    slave_problem::relaxation_values v);

        /// Import the constraints without the duplicates from indexes.
        void insert_without_duplicates_from(indexes& indexes);
        /// Connect the constraint_parts together like given by indexes.
        void add_connections_from(indexes& indexes);
        /// Fills sorted_* and sort them by relaxation_value.
        void sort(slave_problem::relaxation_values v,
                  current_elements& current_elements);

        /// Returns the pointer with greatest relaxation_value.
        std::optional<pointer>& get_greatest_pointer();

        /// Advances the pointer, returns std::nullopt if the end of xs,ys or zs is reached.
        void advance(std::optional<pointer>& p);

        /// Compute Relaxationvalue of value pointed by p.
        double relaxation_value_of(pointer p);

        static bool sort_helper(slave_problem::relaxation_values v,
                                unsigned e,
                                std::weak_ptr<constraint_part> a,
                                std::weak_ptr<constraint_part> b);
        /// Helper method.
        static void set_left_to_smallest(std::weak_ptr<constraint_part>& a,
                                         std::weak_ptr<constraint_part> b);
        /// Helper method.
        static void set_left_to_greatest(unsigned& a, unsigned b);

        /// Collect results after end and create cut.
        bool add_cut(callback add_cut);

        /// Collect the choosen variables from cps into the linear_expression, returns the relaxation_value of added variables.
        double collect(std::vector<std::shared_ptr<constraint_part>>& cps,
                       unsigned element,
                       linear_expression& append_here);
    };

    bool choose_from(current_elements current_elements,
                     indexes indexes,
                     slave_problem::relaxation_values,
                     callback add_cut);

    unsigned choose_from(accumulated_cuts& ac,
                         slave_problem::relaxation_values v,
                         callback add_cut) override;
};

#endif
