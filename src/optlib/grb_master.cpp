#include "grb_master.h"

void
grb_master::solve_reduced_problem(
  int elements,
  unsigned offset,
  arr<>& measurement,
  arr<>& reference_signal,
  std::optional<solver> enforce_particular_solver,
  dual_solution& out,
  double& obj)
{
    assert(measurement.same_dim(out.values) && "Incompatible dimensions!");
    this->offset = offset;

    if (!started) {
        pos_slack_var = new proxy_arr<GRBVar>(
          measurement.dim1, measurement.dim2, measurement.dim3);
        neg_slack_var = new proxy_arr<GRBVar>(
          measurement.dim1, measurement.dim2, measurement.dim3);
        this->set_start_variables(elements, measurement, reference_signal);
        started = true;
    }

    if (enforce_particular_solver) {
        model->set(GRB_IntParam_Method, *enforce_particular_solver);
    }

    try {
        model->optimize();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    double* dual = model->get(GRB_DoubleAttr_Pi,
                              model->getConstrs(),
                              model->get(GRB_IntAttr_NumConstrs));
    get_primal(name2amplitude);

    out.stats = {
        model->get(GRB_DoubleAttr_ObjVal),
        model->get(GRB_DoubleAttr_Runtime),
        model->get(GRB_DoubleAttr_NodeCount),
    };

    std::copy(dual, dual + out.values.size(), out.values.data);
    obj = model->get(GRB_DoubleAttr_ObjVal);
    delete[] dual;

    ///set old value back
    if (enforce_particular_solver) {
        model->set(GRB_IntParam_Method, s);
    }
}

grb_master::grb_master(GRBEnv* e,
                       bool verbose,
                       std::ostream& output,
                       solver s,
                       double master_solution_threshold)
  : master_problem(verbose, output, s)
  , started(false)
  , e(e)
  , threshold(master_solution_threshold)
{
    if (verbose) {
        delete_me = new grb_to_file(output, " ====(Master)==== ");
    }
}

grb_master::~grb_master()
{
    if (delete_me) {
        delete delete_me;
    }
    delete pos_slack_var;
    delete neg_slack_var;
};

void
grb_master::set_start_variables(int elements,
                                arr<>& measurement,
                                arr<>& reference_signal)
{
    assert(e && "Env not set!");

    senders = measurement.dim1;
    receivers = measurement.dim2;
    measurement_samples = measurement.dim3;

    this->model = new GRBModel(*this->e);

    model->set(GRB_IntParam_OutputFlag, 0);
    model->set(GRB_IntParam_Method, s);

    if (verbose) {
        model->setCallback(delete_me);
    }

    constraints = measurement.size();

    arr<> objective_coefficient(
      measurement.dim1, measurement.dim2, measurement.dim3);
    for (unsigned i = 0; i < measurement.dim1; i++) {
        for (unsigned j = 0; j < measurement.dim2; j++) {
            for (unsigned k = 0; k < measurement.dim3; k++) {
                std::string name;
                name += std::to_string(i);
                name += '_';
                name += std::to_string(j);
                name += '_';
                name += std::to_string(k);

                GRBVar pos_slack_ =
                  this->model->addVar(0.0,          //lower bound
                                      GRB_INFINITY, //upper bound
                                      1.0,          //objective value
                                      GRB_CONTINUOUS,
                                      "pos_slack_" + name);
                GRBVar neg_slack_ =
                  this->model->addVar(0.0,          //lower bound
                                      GRB_INFINITY, //upper bound
                                      1.0,          //objective value
                                      GRB_CONTINUOUS,
                                      "neg_slack_" + name);
                GRBLinExpr e = -pos_slack_ + neg_slack_;
                this->model->addConstr(e == measurement(i, j, k));
                pos_slack_var->at(i, j, k) = pos_slack_;
                neg_slack_var->at(i, j, k) = neg_slack_;
            }
        }
    }
}

void
grb_master::add_variable(time_of_flight& variable,
                         const arr<>& reference_signal,
                         std::optional<double> warm_start_value)
{
    assert((unsigned)model->get(GRB_IntAttr_NumConstrs) == constraints &&
           "Should be equal!");
    assert(variable.senders == senders && variable.receivers == receivers &&
           "Variable incompatible with model!");

    GRBColumn add_me;
    proxy_arr<GRBConstr> constr_proxy(
      senders, receivers, measurement_samples, model->getConstrs(), true);
    for (unsigned i = 0; i < senders; i++) {
        for (unsigned j = 0; j < receivers; j++) {
            const int t_helper = (int)offset - (int)variable.at(i, j);
            for (unsigned k = 0; k < measurement_samples; k++) {
                const int t = k + t_helper;
                double x;
                //add constraint if not null
                if (t >= 0 && t < (int)reference_signal.dim3 &&
                    std::abs(x = reference_signal(i, j, t)) > 1e-13) {
                    add_me.addTerm(x, constr_proxy(i, j, k));
                }
            }
        }
    }

    GRBVar var = model->addVar(
      0.0,          //lower bound
      GRB_INFINITY, //upper bound
      0.0,          // objective value
      GRB_CONTINUOUS,
      add_me,
      "v" + std::to_string(name2tof.size())); //name : v + vec_position

    if (warm_start_value) {
        var.set(GRB_DoubleAttr_Start, *warm_start_value);
    }

    name2tof.push_back(std::move(variable));
    name2amplitude.push_back(0.0);
    name2var.push_back(var);
}

unsigned
grb_master::clean()
{
    unsigned deleted_vars = 0;

    auto var = name2var.begin();
    auto tof = name2tof.begin();
    while (var != name2var.end()) {
        double value = var->get(GRB_DoubleAttr_X);
        const int basic = 0;
        //do not remove basic variables as they require to start solving the LP from start.
        if (value < threshold && var->get(GRB_IntAttr_VBasis) != basic) {
            model->remove(*var);
            var = name2var.erase(var);
            tof = name2tof.erase(tof);
            deleted_vars++;
        } else {
            var++;
            tof++;
        }
    }
    /// Reconstruct the name2amplitude vector.
    get_primal(name2amplitude);
    return deleted_vars;
}

void
grb_master::get_primal(std::vector<double>& primal_values)
{
    primal_values.clear();

    for (auto& var : name2var) {
        double value = var.get(GRB_DoubleAttr_X);
        primal_values.push_back(value);
    }
}

void
grb_master::get_primal(std::vector<double>& primal_values,
                       arr<>& pos_slack,
                       arr<>& neg_slack)
{
    // slack
    for (unsigned i = 0; i < pos_slack.dim1; i++) {
        for (unsigned j = 0; j < pos_slack.dim2; j++) {
            for (unsigned k = 0; k < pos_slack.dim3; k++) {
                pos_slack.at(i, j, k) =
                  pos_slack_var->at(i, j, k).get(GRB_DoubleAttr_X);
                neg_slack.at(i, j, k) =
                  neg_slack_var->at(i, j, k).get(GRB_DoubleAttr_X);
            }
        }
    }

    get_primal(primal_values);
}

slow_grb_master::slow_grb_master(GRBEnv* e,
                                 bool verbose,
                                 std::ostream& output,
                                 solver s,
                                 double master_solution_threshold)
  : grb_master(e, verbose, output, s, master_solution_threshold)
{
    if (verbose) {
        delete_me = new slow_grb_to_file(output);
    }
}

slow_grb_master::~slow_grb_master() {}
