#include "grb_cut_chooser.h"

grb_cut_chooser::grb_cut_chooser(GRBEnv& env)
  : env(env)
{}

void
grb_cut_chooser::create_variables_from(unsigned element,
                                       const std::vector<unsigned>& indexes,
                                       GRBModel& model,
                                       relaxation_values data,
                                       bool negative,
                                       std::string name,
                                       std::map<unsigned, GRBVar>& out)
{
    for (unsigned i : indexes) {
        if (out.count(i) == 0) {
            double objective = data.binary(element, element, i);
            if (negative) {
                objective = -objective;
            }
            std::stringstream ss;
            ss << name << "_" << element << "_" << element << "_" << i;
            out[i] = model.addVar(0, 1, objective, GRB_BINARY, ss.str());
        }
    }
}

bool
grb_cut_chooser::current_elements::operator<(const current_elements& rhs) const
{
    return std::tie(i, j, h) < std::tie(rhs.i, rhs.j, rhs.h);
}

void
grb_cut_chooser::create_lp(GRBModel& m,
                           relaxation_values data,
                           current_elements e,
                           indexes& idx)
{
    x_k_variables.clear();
    y_l_variables.clear();
    z_f_variables.clear();

    m.set(GRB_IntAttr_ModelSense, GRB_MAXIMIZE);

    create_variables_from(e.j, idx.K, m, data, false, "x", x_k_variables);
    create_variables_from(e.h, idx.L, m, data, false, "y", y_l_variables);

    for (auto& indexes : idx.F) {
        create_variables_from(e.i, indexes, m, data, true, "z", z_f_variables);
    }

    ///Create constraint
    for (unsigned i = 0; i < idx.K.size(); i++) {
        GRBVar& x_k = x_k_variables[idx.K[i]];
        GRBVar& y_l = y_l_variables[idx.L[i]];
        GRBLinExpr rhs = x_k + y_l;
        for (unsigned f : idx.F[i]) {
            GRBVar& z_f = z_f_variables[f];
            m.addConstr(x_k + y_l <= 1.0 + z_f);
        }
    }

    /// No need to create the objective as it is already done in create_variables...
}

void
grb_cut_chooser::add_if_chosen_by_lp(std::map<unsigned, GRBVar> variables,
                                     unsigned current_element,
                                     bool negative,
                                     linear_expression& append_here)
{
    const double sign = negative ? -1.0 : 1.0;
    for (auto&& [idx, var] : variables) {
        if ((var.get(GRB_DoubleAttr_X) > 0.5)) {
            append_here += sign * lef::create_binary_variable(
                                    current_element, current_element, idx);
        }
    }
}

bool
grb_cut_chooser::get_from_lp(relaxation_values data,
                             callback cb,
                             current_elements e,
                             indexes& idx)
{
    assert_that(idx.K.size() == idx.F.size(), "Should have the same size!");
    assert_that(idx.F.size() == idx.L.size(), "Should have the same size!");

    GRBModel m(env);

    create_lp(m, data, e, idx);

    ///log everything to file
    std::ofstream lp_out("ILP-Cuts.lp", std::ios_base::app);
    //std::ofstream of("ILP-Cuts", std::ios_base::app);
    //dump_linear_expression write_constraint{ of };
    std::ofstream gurobilog("ILP-Cuts.gurobilog", std::ios_base::app);
    grb_to_file grb_cg(gurobilog);
    m.setCallback(&grb_cg);
    m.set(GRB_IntParam_OutputFlag, false);

    m.optimize();
    double objective = m.get(GRB_DoubleAttr_ObjVal);

    /// Copy the gurobi output in the wanted file.
    const char* temp_file = "temp-ILP-Cuts.lp";
    m.write(temp_file);
    //append lp to lp_out
    lp_out << std::ifstream(temp_file).rdbuf() << "\n==========\n" << std::endl;

    linear_expression lhs = lef::create_constant(0);

    add_if_chosen_by_lp(x_k_variables, e.j, false, lhs);
    add_if_chosen_by_lp(y_l_variables, e.h, false, lhs);
    add_if_chosen_by_lp(z_f_variables, e.i, true, lhs);

    linear_expression_constraint cut = lhs <= lef::create_constant(1.0);

    if (intermediate_results) {
        intermediate_results(objective, cut);
    }

    if (objective > 1) {

        //write_constraint(cut);
        cb(cut);

        return true;
    }
    return false;
}

unsigned
grb_cut_chooser::choose_from(accumulated_cuts& ac,
                             relaxation_values data,
                             callback add_cut)
{
    unsigned added_cuts = 0;
    for (unsigned i = 0; i < ac.dim1; i++) {
        for (unsigned j = i + 1; j < ac.dim2; j++) {
            for (unsigned k = j + 1; k < ac.dim3; k++) {
                if (!ac(i, j, k).empty()) {
                    if (get_from_lp(data, add_cut, { i, j, k }, ac(i, j, k))) {
                        added_cuts++;
                    }
                }
            }
        }
    }
    return added_cuts;
}

grb_cut_chooser::~grb_cut_chooser() {}
