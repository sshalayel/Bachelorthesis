#ifndef GRB_CUT_CHOOSER
#define GRB_CUT_CHOOSER

#include "cut_chooser.h"
#include "grb_to_file.h"
#include "gurobi_c++.h"
#include <fstream>
#include <map>

/// Takes an accumulated_cuts instance and finds the best cuts out of it using an ILP.
struct grb_cut_chooser : cut_chooser
{
    GRBEnv& env;
    grb_cut_chooser(GRBEnv& env);

    using relaxation_values = slave_problem::relaxation_values;

    /// Filled by create_lp, maps entries in K to GRBVars.
    std::map<unsigned, GRBVar> x_k_variables;
    /// Filled by create_lp, maps entries in L to GRBVars.
    std::map<unsigned, GRBVar> y_l_variables;
    /// Filled by create_lp, maps entries in F to GRBVars.
    std::map<unsigned, GRBVar> z_f_variables;

    /// Creates all needed GRBVar and takes care of duplicate entries.
    void create_variables_from(unsigned element,
                               const std::vector<unsigned>& indexes,
                               GRBModel& m,
                               relaxation_values data,
                               bool negative,
                               std::string name,
                               std::map<unsigned, GRBVar>& out);

    /// Adds all variables in variables to the linear_expression that were chosen by the ILP.
    void add_if_chosen_by_lp(std::map<unsigned, GRBVar> variables,
                             unsigned current_element,
                             bool negative,
                             linear_expression& append_here);

    /// Create the new cut from the accumulated indexes and add it if its lhs > 1.
    bool get_from_lp(relaxation_values v,
                     callback add_cut,
                     current_elements e,
                     indexes& idx);

    /// Creates the variables, constraints and objectives of the LP.
    void create_lp(GRBModel& m,
                   relaxation_values data,
                   current_elements e,
                   indexes& idx);

    unsigned choose_from(accumulated_cuts& ac,
                         slave_problem::relaxation_values v,
                         callback add_cut) override;

    ~grb_cut_chooser() override;
};
#endif
