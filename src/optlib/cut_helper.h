#ifndef CUT_HELPER_H
#define CUT_HELPER_H

#include "arr.h"
#include "bound_helper.h"
#include "coordinates.h"
#include "cut_chooser.h"
#include "linear_expression.h"
#include "slave_problem.h"
#include "stop_watch.h"
#include <algorithm>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <variant>

enum slave_cut_options
{
    OFF,
    COMPARE_BOTH_CHOOSER,
    ILP_CHOOSER,
    GREEDY_CHOOSER,
};

/// Helper struct that generates cuts.
struct cut_helper
{
    /// Callback used to add cuts.
    using callback = std::function<void(linear_expression_constraint)>;

    using relaxation_values = slave_problem::relaxation_values;

    /// All variables with a value > threshold are candidates for cuts.
    static constexpr double threshold = 1e-2;

    double pitch_in_tacts;
    double squared_pitch_in_tacts;

    /// Needed to take care of offsets/ROI-starts.
    slave_problem::distance_mapping mapping;

    /// Small helper.
    using bh = bound_helper<unsigned>;

    bh bounds;
    /// Number of probe elements.
    unsigned elements;

    cut_helper(double pitch_in_tacts, slave_problem::distance_mapping mapping);

    /// cut_adder is called for every generated cut, cut_chooser is called for the accumulated_cuts.
    void create_cuts(cut_statistics& added_cuts,
                     relaxation_values current_solutions,
                     callback cut_adder,
                     cut_chooser& cc);

    /// Not called in create_cuts. Creates cuts based on the tangent equation of `f(x) = sqrt(x)`
    void create_tangent_cuts(cut_statistics& added_cuts,
                             relaxation_values current_solutions,
                             callback cut_adder);

    ///@brief Fills out with all indexes chosen for tof(i,j) by data.
    ///
    /// out.size() >=1 when working on the relaxation, else it is == 1.
    void extract_all_over_threshold(relaxation_values data,
                                    unsigned i,
                                    unsigned j,
                                    std::vector<unsigned>& out);

    /// Creates all cuts adopted from a cosine rule.
    unsigned create_cosine_rule_cuts(relaxation_values data,
                                     callback add_cut,
                                     cut_chooser& cc);

    /// Creates a cut adopted from a cosine rule. Returns true when cut could be created.
    bool create_cosine_rule_cut(unsigned i,
                                unsigned j,
                                unsigned idx_j,
                                unsigned k,
                                unsigned idx_k,
                                relaxation_values data,
                                cut_chooser::accumulated_cuts& ac,
                                callback add_cut);
};

#endif
