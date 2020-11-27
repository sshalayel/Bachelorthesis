#ifndef RANDOMISATION_H
#define RANDOMISATION_H

#include "arr.h"
#include "coordinates.h"
#include "slave_problem.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <random>
#include <vector>

/// Does all the randomisation stuff, solver independently
struct randomisation
{
    /// Values of the binary variables b_{ijk}.
    std::reference_wrapper<symmetric_arr<proxy_arr<double>>> binaries;
    /// Objective-values for the binary variables b_{ijk}.
    std::reference_wrapper<symmetric_arr<proxy_arr<double>>>
      objective_for_binaries;

    double double_element_pitch_in_tacts;

    /// Needed when using offsets and/or ROIs.
    slave_problem::distance_mapping mapping;

    /// The elements, sorted by decreasing objective value.
    std::vector<unsigned> elements_sorted_by_objective;
    /// The objective values for the elements. objective_by_element[i] = objective of element i.
    std::vector<unsigned> objective_by_element;

    /// Random engine
    std::mt19937 random_engine;
    /// Random distribution
    std::uniform_real_distribution<> random_distribution;

    randomisation(symmetric_arr<proxy_arr<double>>& binaries,
                  symmetric_arr<proxy_arr<double>>& objective_for_binaries,
                  slave_problem::distance_mapping mapping,
                  double element_pitch_in_tacts);

    /// Refresh values.
    void update(symmetric_arr<proxy_arr<double>>& binaries,
                symmetric_arr<proxy_arr<double>>& objective_for_binaries);

    /// Randomises a tof according to the values in binaries and objective_for_binaries, returns false when randomising returned something not lying in ROI.
    bool randomise(time_of_flight& tof);

    /// Fills the objective_by_element vector.
    void fill_objective_by_element();

    /// Fills the elements_sorted_by_objective vector.
    void sort_elements_by_objective();

    /// Chooses a randomized distance in [lower_bound, upper_bound] for sender-receiver-pair (i,j)
    std::optional<unsigned> choose_randomized_distance(
      unsigned i,
      unsigned j,
      std::optional<unsigned> lower_bound,
      std::optional<unsigned> upper_bound);

    /// Computes the value u that determines which interval [0, u] is used to draw random numbers.
    double total_probability_for(unsigned i,
                                 unsigned j,
                                 unsigned start_idx,
                                 unsigned end_idx);

    /// Chooses a random distance in [start_idx, end_idx] for sender-receiver-pair (i,j) with the relaxations distribution.
    std::optional<unsigned> take_from_random(unsigned i,
                                             unsigned j,
                                             unsigned start_idx,
                                             unsigned end_idx,
                                             double random);
};

#endif
