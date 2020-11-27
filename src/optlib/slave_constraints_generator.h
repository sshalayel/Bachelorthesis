#ifndef SLAVE_CONSTRAINT_GENERATION_H
#define SLAVE_CONSTRAINT_GENERATION_H

#include "arr.h"
#include "exception.h"
#include "linear_expression.h"
#include "slave_problem.h"
#include <functional>

/// Generates the slaves constraints. Does not depend on a certain LP-Solverframework.
struct slave_constraint_generator
{
    /// Maps indexes to distances.
    using mapping = slave_problem::distance_mapping;
    /// Called for every generated constraint.
    using callback = std::function<void(linear_expression_constraint)>;

    /// Size of the problem.
    size f;
    /// Mapping from indexes to distances.
    mapping slave_mapping;
    /// Element pitch in tacts.
    double pitch;

    /// Used to assert that not diameter_inequality and diameter_equality are used together.
    std::optional<bool> rounded_down_non_diameters;

    /// Generates all needed constraints.
    void all(callback callback);

  protected:
    /// Generates all n^2 constraints \sum_k (k \cdot b_ijk) <= d_ij <= \sum_k ((k+1) \cdot b_ijk) from the binaries and calls the callback for every one of them. Also creates d_0 >= x and d_0 >= -x.
    void diameter_definition(callback callback);

    /// Generates all n^2 constraints d_ii + d_jj -1 <= 2\cdot d_ij <= d_ii + d_jj + 1. Non compatible with diameter_inequality.
    void diameter_inequality(callback callback);

    /// Generates all n^2 constraints d_ii + d_jj = 2 * d_ij, can make the optimisation stuck. Non compatible with diameter_inequality.
    void diameter_equality(callback callback);

    /// Generates all n constraints \sum_k (k^2 \cdot b_iik) <= q_ii <= \sum_k ((k+1)^2 \cdot b_iik) from the binaries and calls the callback for every one of them.
    void quadratic_definition(callback callback);

    /// Generates all constraints q_ii = q_00 + i\Delta (i\Delta - 2x).
    void quadratic_equality(callback callback);

    void root_tangents(callback callback);

    /// Generates all constraints \sum_k b_ijk = 1.
    void binary_sum_to_one(callback callback);

    /// Generates all constraints of the form `s_{iik} + \sum_{h \in H} s_{00h} \le 1`, where H is obtained from the triangle inequality, for all i and k.
    void triangle_inequality_0th_element(callback callback);
};

#endif
