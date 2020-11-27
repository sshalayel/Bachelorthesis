#include "grb_slave.h"

grb_slave::grb_slave(GRBEnv* e,
                     double element_pitch_in_tacts,
                     std::optional<double> slavestop,
                     unsigned offset,
                     std::optional<double> horizontal_roi_start,
                     std::optional<double> horizontal_roi_end,
                     std::ostream& output,
                     grb_resettable_callback* grb_callback,
                     bool verbose)
  : element_pitch_in_tacts(element_pitch_in_tacts)
  , squared_pitch(std::pow(element_pitch_in_tacts, 2))
  , slavestop(slavestop)
  , offset(offset)
  , horizontal_roi_start(horizontal_roi_start)
  , horizontal_roi_end(horizontal_roi_end)
  , output(output)
  , verbose(verbose)
  , started(false)
  , generated(false)
  , vars_proxy(0, 0, 0, nullptr, false)
  , vars_objective(0, 0, 0, nullptr, false)
  , squared_vars_proxy(0, nullptr, false)
  , diameter_vars_proxy(0, 0, nullptr, false)
  , model(*e)
  , grb_callback(grb_callback)
{
    if (verbose) {
        model.setCallback(grb_callback);
    }
}

slave_problem::variables<GRBVar>
grb_slave::variables()
{
    return {
        vars_proxy,
        diameter_vars_proxy,
        squared_vars_proxy,

        representant_x,
    };
}

slave_problem::distance_mapping
grb_slave::mapping()
{
    return {
        [&](unsigned i) { return distance_mapping(i); },
        [&](unsigned i) { return revert_distance_mapping(i); },
    };
}

grb_slave::~grb_slave()
{
    if (grb_callback) {
        delete grb_callback;
    }
}

unsigned
grb_slave::distance_mapping(unsigned k) const
{
    return k + offset;
}

std::optional<unsigned>
grb_slave::revert_distance_mapping(unsigned k) const
{
    assert(vars_proxy.data && "Only use this when some data is loaded!");
    if (k >= offset && k < offset + vars_proxy.dim3) {
        return k - offset;
    } else {
        return std::nullopt;
    }
}

void
grb_slave::generate_constraints(size f)
{
    grb_linear_expression_visitor glev{ variables() };

    slave_constraint_generator scg{
        f,
        mapping(),
        element_pitch_in_tacts,
    };

    scg.all([&](linear_expression_constraint c) { model.addConstr(glev(c)); });
}

void
grb_slave::generate(size f)
{
    if (generated) {
        return;
    }

    assert(f.dim1 == f.dim2);

    model.set(GRB_IntParam_PreCrush, 1);
    model.set(GRB_IntParam_OutputFlag, false);
    model.set(GRB_IntParam_LazyConstraints, true);
    model.set(GRB_StringAttr_ModelName, "slave_problem");
    /// Aggressiveness : -1 (auto), 0 (off), 1 (normal), 2 (aggressive), 3 (very aggressive)
    model.set(GRB_IntParam_Cuts, 3);

    if (slavestop) {
        model.set(GRB_DoubleParam_BestObjStop, *slavestop);
    }
    const int max_distance = distance_mapping(f.dim3);

    representant_x = model.addVar(horizontal_roi_start.value_or(-max_distance),
                                  horizontal_roi_end.value_or(max_distance),
                                  0.0,
                                  GRB_CONTINUOUS,
                                  "x");

    vars_proxy.realloca(
      f.dim1,
      f.dim2,
      f.dim3,
      model.addVars(symmetric_arr<proxy_arr<GRBVar>>::size(f), GRB_BINARY),
      true);
    vars_objective.realloca(f.dim1, f.dim2, f.dim3);
    diameter_vars_proxy.realloca(
      f.dim1,
      f.dim2,
      model.addVars(
        symmetric_arr_2d<proxy_arr, GRBVar>::size({ f.dim1, f.dim2 }),
        GRB_CONTINUOUS),
      true);
    squared_vars_proxy.realloca(
      f.dim1, model.addVars(f.dim1, GRB_CONTINUOUS), true);

    vars_proxy.for_ijkv([](unsigned i, unsigned j, unsigned k, GRBVar& v) {
        std::stringstream ss;
        ss << "b_" << i << "_" << j << "_" << k;
        v.set(GRB_StringAttr_VarName, ss.str());
    });

    diameter_vars_proxy.for_ijkv(
      [](unsigned, unsigned j, unsigned k, GRBVar& v) {
          std::stringstream ss;
          ss << "d_" << j << "_" << k;
          v.set(GRB_StringAttr_VarName, ss.str());
      });

    squared_vars_proxy.for_ijkv([](unsigned, unsigned, unsigned k, GRBVar& v) {
        std::stringstream ss;
        ss << "q_" << k;
        v.set(GRB_StringAttr_VarName, ss.str());
    });

    generate_constraints(f);

    started = true;
    generated = true;
}

void
grb_slave::update_objective(proxy_arr<double>& f)
{
    assert(f.dim1 == vars_proxy.dim1 && f.dim2 == vars_proxy.dim2 &&
           f.dim3 == vars_proxy.dim3);

    GRBLinExpr obj = 0;
    for (unsigned i = 0; i < f.dim1; i++) {
        for (unsigned j = 0; j < f.dim2; j++) {
            for (unsigned k = 0; k < f.dim3; k++) {
                if (std::fabs(f(i, j, k)) > 1e-10) {
                    obj += f(i, j, k) * vars_proxy.at(i, j, k);
                    vars_objective(i, j, k) = f(i, j, k);
                } else {
                    vars_objective(i, j, k) = 0.0;
                }
            }
        }
    }

    model.setObjective(obj, GRB_MAXIMIZE);
}

void
grb_slave::prepare_for_solving(proxy_arr<double>& f)
{
    if (started) {
        this->update_objective(f);
    } else {
        this->generate(f);
        this->update_objective(f);
        model.write("myslave.lp");
    }
    if (grb_callback) {
        grb_callback->reset();
    }
}

bool
grb_slave::assert_feasibility(GRBModel& model)
{
    if (model.get(GRB_IntAttr_Status) == GRB_INFEASIBLE) {
        // do IIS
        std::cerr << "The model is infeasible; computing IIS" << std::endl;
        model.computeIIS();
        std::cerr << "\nThe following constraint(s) "
                  << "cannot be satisfied:" << std::endl;
        GRBConstr* c = model.getConstrs();
        for (int i = 0; i < model.get(GRB_IntAttr_NumConstrs); ++i) {
            if (c[i].get(GRB_IntAttr_IISConstr) == 1) {
                std::cerr << c[i].get(GRB_StringAttr_ConstrName) << std::endl;
            }
        }
        model.write("infeasible.ilp");
        std::cerr << "Model written to infeasible.lp" << std::endl;
        return false;
    }
    return true;
}

void
grb_slave::binary_to_tof(
  std::function<double(unsigned, unsigned, unsigned)> binaries,
  time_of_flight& fill_me)
{
    fill_me.for_ijk([&](unsigned, unsigned i, unsigned j) {
        for (unsigned k = 0; k < vars_proxy.dim3; k++) {
            if (binaries(i, j, k) > 0.5) {
                return distance_mapping(k);
            }
        }
        assert_that(false, "No binary variable has been chosen?");
        return 0u;
    });
}

void
grb_slave::discretise_slave_result(
  std::function<double(unsigned, unsigned)> values,
  time_of_flight& fill_me)
{
    fill_me.for_ijk([&](unsigned, unsigned i, unsigned j) {
        double distance_hint = values(i, j);
        double ret;
        if (distance_hint - std::floor(distance_hint) >= 0.5) {
            ret = std::ceil(distance_hint);
        } else {
            ret = std::floor(distance_hint);
        }
        return ret;
    });
}

void
grb_slave::fill_tof(
  std::function<double(unsigned, unsigned, unsigned)> binaries,
  std::function<double(unsigned, unsigned, unsigned)> diameters,
  std::function<double(unsigned, unsigned, unsigned)> quadratics,
  double x,
  time_of_flight& fill_me,
  bool verbose)
{
    fill_me.for_ijk([&](unsigned, unsigned i, unsigned j) {
        for (unsigned k = 0; k < vars_proxy.dim3; k++) {
            if (binaries(i, j, k) > 0.5) {
                return distance_mapping(k);
            }
        }
        assert_that(false, "No binary variable has been chosen?");
        return 0u;
    });
    fill_me.representant_x = x;

    diameter_t<double> d(vars_proxy.dim1, vars_proxy.dim2);
    d.for_ijk(diameters);
    quadratic_t<double> q(vars_proxy.dim1);
    q.for_ijk(quadratics);

    double y = std::sqrt(q(0) - std::pow(x, 2));
    if (std::isnan(y)) {
        y = 0.0;
        if (verbose) {
            std::cerr << "Found a ToF with y = NaN! q_0 was " << q(0)
                      << " and x^2 was " << std::pow(x, 2) << std::endl;
        }
    }
    fill_me.extension =
      time_of_flight::cgdump2_extension(y, std::move(d), std::move(q));
}

void
grb_slave::get_results(columns& columns)
{
    assert(assert_feasibility(this->model));
    double obj = model.get(GRB_DoubleAttr_ObjVal);

    time_of_flight tof(
      vars_proxy.dim1, vars_proxy.dim2, representant_x.get(GRB_DoubleAttr_X));

    fill_tof(
      [&](unsigned i, unsigned j, unsigned k) {
          return vars_proxy(i, j, k).get(GRB_DoubleAttr_X);
      },
      [&](unsigned, unsigned i, unsigned j) {
          return diameter_vars_proxy(i, j).get(GRB_DoubleAttr_X);
      },
      [&](unsigned, unsigned, unsigned i) {
          return squared_vars_proxy(i).get(GRB_DoubleAttr_X);
      },
      representant_x.get(GRB_DoubleAttr_X),
      tof);

    columns.push_back({ std::move(tof),
                        {
                          obj,
                          model.get(GRB_DoubleAttr_Runtime),
                          model.get(GRB_DoubleAttr_ObjVal),
                          model.get(GRB_DoubleAttr_ObjBound),
                          model.get(GRB_DoubleAttr_NodeCount),
                          model.get(GRB_IntAttr_SolCount),
                        } });
}

void
grb_slave::add_solution_start_hints(size s, std::vector<time_of_flight>& hints)
{
    this->generate(s);

    unsigned current_start = model.get(GRB_IntAttr_NumStart);
    model.set(GRB_IntAttr_NumStart, current_start + hints.size());
    for (time_of_flight& tof : hints) {
        model.set(GRB_IntParam_StartNumber, current_start++);

        for (unsigned i = 0; i < vars_proxy.dim1; i++) {
            for (unsigned j = i; j < vars_proxy.dim2; j++) {
                assert(vars_proxy.dim3 > tof.at(i, j) - offset);
                vars_proxy.at(i, j, tof.at(i, j) - offset)
                  .set(GRB_DoubleAttr_Start, 1.0);
            }
        }
    }
}

void
grb_slave::run(proxy_arr<double>& f, columns& columns)
{
    prepare_for_solving(f);

    model.optimize();

    get_results(columns);
}
