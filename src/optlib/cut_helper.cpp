#include "cut_helper.h"
cut_helper::cut_helper(double pitch_in_tacts,
                       slave_problem::distance_mapping mapping)
  : pitch_in_tacts(pitch_in_tacts)
  , squared_pitch_in_tacts(pitch_in_tacts * pitch_in_tacts)
  , mapping(mapping)
  , bounds(squared_pitch_in_tacts)
{}

void
cut_helper::create_tangent_cuts(cut_statistics& added_cuts,
                                relaxation_values current_solutions,
                                callback cut_adder)
{
    const double epsilon = 1e-2;
    const unsigned elements = current_solutions.binary.get().dim2;
    const auto& diameter = current_solutions.diameter.value();
    const auto& squared = current_solutions.squared.value();
    for (unsigned i = 0; i < elements; i++) {
        double d_ii = diameter(i, i);
        double q_i = squared(i);
        if (d_ii - sqrt(q_i) > epsilon) {
            linear_expression var_d_ii = lef::create_diameter_variable(i, i);
            linear_expression var_q_i = lef::create_quadratic_variable(i);

            double sqrt_q_i = std::sqrt(q_i);
            cut_adder(var_d_ii <=
                      (1.0 / (2.0 * sqrt_q_i)) * (var_q_i - q_i) + sqrt_q_i);
            added_cuts.tangent++;
        }
    }
}

void
cut_helper::extract_all_over_threshold(relaxation_values v,
                                       unsigned i,
                                       unsigned j,
                                       std::vector<unsigned>& out)
{
    assert_that(mapping.to_distance, "Mapping cannot be empty!");
    for (unsigned k = 0; k < v.binary.get().dim3; k++) {
        if (v.binary(i, j, k) > threshold) {
            out.push_back(k);
        }
    }
}

void
cut_helper::create_cuts(cut_statistics& added_cuts,
                        relaxation_values v,
                        callback add_cut,
                        cut_chooser& cc)
{
    elements = v.binary.get().dim1;

    //create_quadratic_cuts(added_cuts, v, cb);
    added_cuts.cosine_rule += create_cosine_rule_cuts(v, add_cut, cc);
}

bool
cut_helper::create_cosine_rule_cut(unsigned i,
                                   unsigned j,
                                   unsigned idx_j,
                                   unsigned k,
                                   unsigned idx_k,
                                   relaxation_values data,
                                   cut_chooser::accumulated_cuts& ac,
                                   callback add_cut)
{
    // Bounds for i when j and k are fixed.
    double squared_lower_bound;
    double squared_upper_bound;

    bounds.template get_corrected<double>(
      i,
      { j, mapping.to_distance(idx_j) },
      { k, mapping.to_distance(idx_k) },
      { squared_lower_bound, squared_upper_bound });

    auto& current_ac = ac(i, j, k);
    current_ac.F.push_back({});
    std::vector<unsigned>& current_z_fs = current_ac.F.back();

    const unsigned lower_root = std::floor(std::sqrt(squared_lower_bound));
    for (unsigned n = lower_root; n * n < squared_upper_bound; n++) {
        const std::optional<unsigned> idx = mapping.to_index(n);
        if (idx) {
            current_z_fs.push_back(*idx);
        }
    }
    if (!current_z_fs.empty()) {
        current_ac.K.push_back(idx_j);
        current_ac.L.push_back(idx_k);
    } else {
        /// Out-of-ROI, don't add this cut.
        current_ac.F.pop_back();
    }
    return false;
}

unsigned
cut_helper::create_cosine_rule_cuts(relaxation_values data,
                                    callback add_cut,
                                    cut_chooser& cc)
{
    assert(data.squared);

    std::vector<unsigned> indexes_i;
    std::vector<unsigned> indexes_j;
    std::vector<unsigned> indexes_k;

    //stop_watch sw;
    // First compute all elements that probably needs to be cutted, that is, where q_i = \sum_k k^2 b_iik or q_i = \sum_k (k+1)^2 b_iik.
    std::vector<unsigned> elements_to_cut;
    for (unsigned i = 0; i < elements; i++) {
        double lower_sum = 0.0;
        double upper_sum = 0.0;
        for (unsigned k = 0; k < data.binary.get().dim3; k++) {
            const double b = data.binary(i, i, k);
            lower_sum += std::pow(mapping.to_distance(k) - 0.5, 2) * b;
            upper_sum += std::pow(mapping.to_distance(k) + 0.5, 2) * b;
        }
        const double q_value = data.squared->get().at(i);
        const double threshold = 1e-6;
        if ( //std::fabs(q_value - lower_sum) < threshold || // not needed: only can cheat by making smaller, not bigger!
          std::fabs(q_value - upper_sum) < threshold) {
            elements_to_cut.push_back(i);
        }
    }
    //std::cout << elements_to_cut.size() << " elements to cut out of "
    //<< elements << " possible elements in " << sw.elapsed()
    //<< " seconds" << std::endl;
    //assert_that(
    //elements_to_cut.size() > 3,
    //"for testing: elements to cut should not avoid cuts being created!");

    assert_that(data.binary.get().dim1 == data.binary.get().dim2,
                "Should have same number of sender/receiver!");
    cut_chooser::accumulated_cuts ac(
      data.binary.get().dim1, data.binary.get().dim1, data.binary.get().dim1);

    unsigned ret = 0;
    //forall s_jjm and s_kkn in node-relaxation-solution : whitelist all s_iip for feasible p's as lazy constraint.
    for (unsigned i = 0; i < elements_to_cut.size(); i++) {
        for (unsigned j = i + 1; j < elements_to_cut.size(); j++) {
            indexes_j.clear();
            extract_all_over_threshold(
              data, elements_to_cut[j], elements_to_cut[j], indexes_j);
            for (unsigned index_j : indexes_j) {
                for (unsigned k = j + 1; k < elements_to_cut.size(); k++) {
                    indexes_k.clear();
                    extract_all_over_threshold(
                      data, elements_to_cut[k], elements_to_cut[k], indexes_k);
                    for (unsigned index_k : indexes_k) {
                        if (create_cosine_rule_cut(elements_to_cut[i],
                                                   elements_to_cut[j],
                                                   index_j,
                                                   elements_to_cut[k],
                                                   index_k,
                                                   data,
                                                   ac,
                                                   add_cut)) {
                            ret++;
                        }
                    }
                }
            }
        }
    }
    ret += cc.choose_from(ac, data, add_cut);
    return ret;
}
