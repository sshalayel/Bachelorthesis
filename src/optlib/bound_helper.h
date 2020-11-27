#ifndef BOUND_HELPER_H
#define BOUND_HELPER_H

#include "coordinates.h"
#include "linear_expression.h"
#include <cmath>
#include <utility>

/// Helper class that computes lower and upper bounds for variables.
template<typename Input>
struct bound_helper
{
    /// Represents a variable lying on the diagonal.
    struct variable
    {
        /// The index i of tof_{ii}.
        unsigned idx;
        /// The length (with offset!!!) of tof_{ii} or the corresponding variable responsible s_{iik}.
        Input diameter;
        /// The square (length/variable) of diameter.
        Input square = diameter * diameter;

        /// Helper function to compute (*this + 1)^2 for length(double) or variables(GRBLinExpr).
        template<typename Output>
        Output plus_1_and_squared();
        /// Returns the lower_bound of coef * squared depending on coefs sign.
        template<typename Output>
        Output upper_bound(double coef);
        /// Returns the upper_bound of coef * squared depending on coefs sign.
        template<typename Output>
        Output lower_bound(double coef);
    };

    /// Helps generating variables from a tof.
    struct variable_from_tof
    {
        /// The source-tof.
        time_of_flight& tof;
        /// Get the ith-variable on diagonal.
        variable get(unsigned i)
        {
            return { i, tof(i, i), tof(i, i) * tof(i, i) };
        }
        /// Get the ith-variable on diagonal.
        variable operator()(unsigned i) { return get(i); }
    };

    ///Helper that generates variable from 2 arrs, one with diameter and one with square values.
    template<typename Diameter, typename Squared>
    struct variable_generator
    {
        Diameter d;
        Squared s;

        /// Create variable from d and s using index i.
        variable get(unsigned i) { return { i, d(i, i), s(i) }; }
        variable operator()(unsigned i) { return get(i); }
    };

    bound_helper(double squared_pitch);
    const double squared_pitch;

    ///@brief Returns the bounds for the squared value of one variable when 2 other variables are chosen.
    ///
    /// For fixed j and k, returns an lower bound lb and an upper bound up s.t. lb <= std::fabs(j-k) * squared_i <= ub for some real squared_i (not the floor one!).
    /// Templating allows to use the same function for constraint-generation and for numeric value computation.
    /// d(m,n) should contain the diameters for sender-receiver-pair (m,n).
    /// s(e) should contain the squared diameter for all elements e.
    /// Output can be double or GRBLinExpr.
    template<typename output>
    void get(variable i,
             variable j,
             variable k,
             std::pair<output&, output&> bounds);

    /// Only does the normals bounds, which means that the values of i are not needed.
    template<typename output>
    void get(unsigned idx_i,
             variable j,
             variable k,
             std::pair<output&, output&> bounds);

    ///Like get but divides the result by std::fabs(j - k) to return the ,,true'' bounds L<=sq_{ii}<=U.
    template<typename Output>
    void get_corrected(unsigned idx_i,
                       variable j,
                       variable k,
                       std::pair<Output&, Output&> bounds);

    /// Generates the bounds needed for feasible cosines -bound<=middle<=bound.
    template<typename Output>
    void get_cosine_bounds(variable i,
                           variable j,
                           std::pair<Output&, Output&> approximations,
                           Output& bound);
};

template<typename Input>
bound_helper<Input>::bound_helper(double squared_pitch)
  : squared_pitch(squared_pitch)
{}

template<typename Input>
template<typename Output>
Output
bound_helper<Input>::variable::plus_1_and_squared()
{
    return square + 2.0 * diameter + 1.0;
}

template<typename Input>
template<typename Output>
Output
bound_helper<Input>::variable::lower_bound(double coef)
{
    if (coef > 0) {
        return coef * square - coef * diameter + 0.25;
    } else {
        return coef * square + coef * diameter + 0.25;
    }
}

template<typename input>
template<typename output>
output
bound_helper<input>::variable::upper_bound(double coef)
{
    if (coef > 0) {
        return coef * square + coef * diameter + 0.25;
    } else {
        return coef * square - coef * diameter + 0.25;
    }
}

template<typename Input>
template<typename output>
void
bound_helper<Input>::get_corrected(unsigned idx_i,
                                   variable j,
                                   variable k,
                                   std::pair<output&, output&> bounds)
{
    const double correction = std::fabs((double)j.idx - (double)k.idx);
    get(idx_i, j, k, bounds);
    bounds.first /= correction;
    bounds.second /= correction;
}

template<typename Input>
template<typename Output>
void
bound_helper<Input>::get(unsigned idx_i,
                         variable j,
                         variable k,
                         std::pair<Output&, Output&> bounds)
{
    get({ idx_i, Input(), Input() }, j, k, bounds);
}

template<typename Input>
template<typename Output>
void
bound_helper<Input>::get(variable i,
                         variable j,
                         variable h,
                         std::pair<Output&, Output&> bounds)
{
    ///  Assumption 1: i != j != h != i
    assert(j.idx != h.idx && h.idx != i.idx);

    ///  Assumption 2: i < h && j < h OR h < i && h < j
    if ((i.idx < h.idx && j.idx > h.idx) || (h.idx < i.idx && h.idx > j.idx)) {
        std::swap(j, h);
    }

    const double JH = std::fabs(((double)j.idx) - ((double)h.idx));
    const double IH = std::fabs(((double)i.idx) - ((double)h.idx));
    const double constant = 4.0 * squared_pitch * JH * IH * (IH - JH);

    bounds.first = j.template lower_bound<Output>(IH) +
                   h.template lower_bound<Output>(JH - IH) + constant;

    bounds.second = j.template upper_bound<Output>(IH) +
                    h.template upper_bound<Output>(JH - IH) + constant;
}

template<typename Input>
template<typename Output>
void
bound_helper<Input>::get_cosine_bounds(
  variable i,
  variable j,
  std::pair<Output&, Output&> approximated_bound,
  Output& bound)
{
    //upper bound that needs to be respected by absolute value
    bound = 4.0 * (i.diameter + 1.0) * std::fabs((double)j.idx - i.idx) *
            std::sqrt(squared_pitch);

    //lower bound that can be computed (as the real value is not known)
    approximated_bound.first =
      i.square + std::pow((double)j.idx - i.idx, 2) * squared_pitch * 4.0 -
      j.template plus_1_and_squared<Output>();

    //upper bound that can be computed (as the real value is not known)
    approximated_bound.second =
      i.template plus_1_and_squared<Output>() +
      std::pow((double)j.idx - i.idx, 2) * squared_pitch * 4.0 - j.square;
}

#endif
