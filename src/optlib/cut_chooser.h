#ifndef CUT_CHOOSER_H
#define CUT_CHOOSER_H

#include "arr.h"
#include "linear_expression.h"
#include "slave_problem.h"
#include <map>
#include <vector>

/// Accumulates cuts of the type `b_iik + b_jjl <= 1 + b_hhf`, where the left hand side is <= 1.
struct cut_chooser
{
    /// Contans the cuts in the form b_{i,i,K[p]} + b_{j,j,L[p]} <= 1.0 + \sum_{f \in F[p]} b_{k,k,f} for all p.
    struct indexes
    {
        /// The indexes k for b_iik.
        std::vector<unsigned> K;
        /// The indexes l for b_jjl.
        std::vector<unsigned> L;
        /// The indexes f for b_kkf.
        std::vector<std::vector<unsigned>> F;

        /// Returns if one of K, L or F is empty.
        bool empty() const { return K.empty(); }
    };

    /// Contains one tuple of three different transducer elements.
    struct current_elements
    {
        ///Element for z_f.
        unsigned i;
        ///Element for x_k.
        unsigned j;
        ///Element for y_l.
        unsigned h;
        /// Needed for std::set.
        bool operator<(const current_elements& rhs) const;
    };

    /// Every tuple of 3 elements (i,j,k) maps to some constraints that needs to be accumulated.
    using accumulated_cuts = proxy_arr<indexes>;

    /// Callback used to add cuts.
    using callback = std::function<void(linear_expression_constraint)>;

    /// For debugging with objectives.
    using intermediate_results_callback =
      std::function<void(double current_objective,
                         linear_expression_constraint lec)>;
    /// Reveal intermediate_results for debugging purposes.
    intermediate_results_callback intermediate_results;

    /// Create the cuts from the accumulated indexes and add it if its lhs > 1.
    virtual unsigned choose_from(accumulated_cuts& ac,
                                 slave_problem::relaxation_values v,
                                 callback add_cut) = 0;
    virtual ~cut_chooser() {}
};

#endif
