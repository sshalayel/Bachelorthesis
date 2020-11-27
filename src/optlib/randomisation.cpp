#include "randomisation.h"

randomisation::randomisation(
  symmetric_arr<proxy_arr<double>>& binaries,
  symmetric_arr<proxy_arr<double>>& objective_for_binaries,
  slave_problem::distance_mapping mapping,
  double element_pitch_in_tacts)
  : binaries(binaries)
  , objective_for_binaries(objective_for_binaries)
  , double_element_pitch_in_tacts(2.0 * element_pitch_in_tacts)
  , mapping(mapping)
  , random_engine(std::random_device{}())
  , random_distribution(0.0, 1.0)
{
    assert(binaries.dim1 == binaries.dim2);

    fill_objective_by_element();
    sort_elements_by_objective();
}

void
randomisation::update(symmetric_arr<proxy_arr<double>>& binaries,
                      symmetric_arr<proxy_arr<double>>& objective_for_binaries)
{
    this->binaries = binaries;
    this->objective_for_binaries = objective_for_binaries;

    fill_objective_by_element();
    sort_elements_by_objective();
}

void
randomisation::fill_objective_by_element()
{
    objective_by_element.clear();
    objective_by_element.reserve(binaries.get().dim1);

    for (unsigned i = 0; i < binaries.get().dim1; i++) {
        double objective = 0.0;
        for (unsigned k = 0; k < binaries.get().dim3; k++) {
            const double current_objective =
              objective_for_binaries(i, i, k) * binaries(i, i, k);
            objective += current_objective;
        }
        objective_by_element.push_back(objective);
    }
}

void
randomisation::sort_elements_by_objective()
{
    assert(objective_by_element.size() == binaries.get().dim1 &&
           "No objectives?");

    elements_sorted_by_objective.resize(binaries.get().dim1);
    //Write indexes.
    std::iota(elements_sorted_by_objective.begin(),
              elements_sorted_by_objective.end(),
              0);

    //Sort by objective.
    std::sort(elements_sorted_by_objective.begin(),
              elements_sorted_by_objective.end(),
              [&](unsigned a, unsigned b) {
                  // needs to be true when a should be before b
                  return objective_by_element[a] > objective_by_element[b];
              });
}

std::optional<unsigned>
randomisation::choose_randomized_distance(unsigned i,
                                          unsigned j,
                                          std::optional<unsigned> _lower_bound,
                                          std::optional<unsigned> _upper_bound)
{
    // Indexes are [a_idx, b_idx) but distances are [a,b] !!!
    unsigned lower_bound = _lower_bound.value_or(mapping.to_distance(0));
    unsigned upper_bound =
      _upper_bound.value_or(mapping.to_distance(binaries.get().dim3 - 1));

    if (auto lower_bound_idx_ = mapping.to_index(lower_bound)) {
        unsigned lower_bound_idx = *mapping.to_index(lower_bound);
        if (auto upper_bound_idx_ = mapping.to_index(upper_bound)) {
            unsigned upper_bound_idx = *mapping.to_index(upper_bound) + 1;
            if (lower_bound > upper_bound ||
                lower_bound_idx >= upper_bound_idx) {
                std::cerr << " ==== (RANDOMISATION) ==== bound [" << lower_bound
                          << ", " << upper_bound << "] with indexes "
                          << lower_bound_idx << " and " << upper_bound_idx
                          << " is empty!" << std::endl;
                return {};
            }

            double total_probability =
              total_probability_for(i, j, lower_bound_idx, upper_bound_idx);

            const double next_random_value = random_distribution(random_engine);

            unsigned random_idx;

            if (total_probability < 1e-5) {
                random_idx =
                  next_random_value * (upper_bound_idx - lower_bound_idx) +
                  lower_bound_idx; //equally distributed
            } else if (auto r = take_from_random(
                         i,
                         j,
                         lower_bound_idx,
                         upper_bound_idx,
                         next_random_value *
                           total_probability)) { //distributed by relaxation
                random_idx = *r;
            } else {
                return {};
            }

            return mapping.to_distance(random_idx);
        } else {
            std::cerr
              << " ==== RANDOMISATION ==== Error in randomisation: upper_bound"
              << upper_bound << " is outside of ROI!" << std::endl;
            return {};
        }
    } else {
        std::cerr
          << " ==== RANDOMISATION ==== Error in randomisation: lower_bound"
          << lower_bound << " is outside of ROI!" << std::endl;
        return {};
    }
}

std::optional<unsigned>
randomisation::take_from_random(unsigned i,
                                unsigned j,
                                unsigned start_idx,
                                unsigned end_idx,
                                double random)
{
    if (start_idx >= end_idx) {
        std::cerr << " ==== RANDOMISATION ==== end_idx " << end_idx
                  << " should be smaller then start_idx " << start_idx << "!"
                  << std::endl;
        return {};
    }
    for (unsigned k = start_idx; k < end_idx; k++) {
        random -= binaries(i, j, k);
        if (random <= 0) {
            return k;
        }
    }
    std::cerr
      << " ==== RANDOMISATION ==== No value could be chosen by randomisation"
      << std::endl;
    return {};
}

double
randomisation::total_probability_for(unsigned i,
                                     unsigned j,
                                     unsigned start_idx,
                                     unsigned end_idx)
{
    assert(start_idx < end_idx);
    if (start_idx == 0 && end_idx == binaries.get().dim3) {
        return 1.0;
    } else {
        double total_probability = 0.0;
        for (unsigned k = start_idx; k < end_idx; k++) {
            total_probability += binaries(i, j, k);
        }
        return total_probability;
    }
}

bool
randomisation::randomise(time_of_flight& tof)
{
    assert(elements_sorted_by_objective.size() == binaries.get().dim1);

    bounds cos{ -1, 1 };
    /// Needs to be respected by all distances.
    const bounds absolute(mapping.to_distance(0),
                          mapping.to_distance(binaries.get().dim3) - 1);

    unsigned upper_bound = mapping.to_distance(binaries.get().dim3) - 1;
    unsigned lower_bound = mapping.to_distance(0);

    const unsigned first = elements_sorted_by_objective.front();
    const auto first_distance =
      choose_randomized_distance(first, first, lower_bound, upper_bound);

    if (!first_distance) {
        std::cerr << " ==== RANDOMISATION ==== Chosen randomised distance in "
                     "randomisation is out of bounds!"
                  << std::endl;
        return false;
    }
    const bounds first_bound(*first_distance - 0.5, *first_distance + 0.5);
    tof.at(first, first) = *first_distance;

    for (unsigned _i = 1; _i < elements_sorted_by_objective.size(); _i++) {
        const unsigned i = elements_sorted_by_objective[_i];
        /// Distance between element i and first element.
        const double delta =
          std::fabs((double)i - first) * double_element_pitch_in_tacts;

        /// bounds for allowed distances for tof_{ii}.
        bounds allowed = coordinate_tools::squared_length_of_c(
                           first_bound,
                           { delta },
                           // mirror the angle to the other side if i > first
                           i > first ? cos : -1.0 * cos)
                           .root();
        if (!allowed.is_intersecting(absolute)) {
            /// We are randomising something that does not lie in the ROI.
            ///TODO: add a cut or something.
            //exception("randomisation failed").raise();
            return false;
        }
        allowed = allowed.intersect(absolute).round();

        /// Choose one distance.
        if (auto r =
              choose_randomized_distance(i, i, allowed.lower, allowed.upper)) {
            tof.at(i, i) = *r;
        } else {
            std::cerr << " ==== RANDOMISATION ==== Chosen randomised distance "
                         "is not in ROI!"
                      << std::endl;
            return false;
        }

        /// Compute new cosine-bounds.
        bounds new_cosine = coordinate_tools::cosinus_gamma(
          first_bound, delta, bounds(tof.at(i, i), tof.at(i, i) + 1));

        bounds current_cosine = i > first ? new_cosine : -1.0 * new_cosine;
        if (!cos.is_intersecting(current_cosine)) {
            std::cerr << " ==== RANDOMISATION ==== Randomisation failed: "
                         "current cosine bound became "
                         "empty when intersecting old bound "
                      << cos << " with new bound " << current_cosine
                      << std::endl;
            return false;
        }

        /// Update new cosine-bounds.
        cos = cos.intersect(current_cosine);
    }
    return true;
}
