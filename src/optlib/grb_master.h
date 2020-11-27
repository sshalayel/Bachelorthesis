#ifndef GRB_MASTER_H
#define GRB_MASTER_H

#include "arr.h"
#include "convolution.h"
#include "coordinates.h"
#include "grb_callback.h"
#include "gurobi_c++.h"
#include "master.h"
#include <cassert>
#include <list>
#include <optional>

/// A master_problem implementation using Gurobi.
class grb_master : public master_problem
{
  public:
    grb_master(GRBEnv* e,
               bool verbose,
               std::ostream& output,
               solver s,
               double master_solution_threshold);
    ~grb_master() override;

    GRBCallback* delete_me = nullptr;

    proxy_arr<GRBVar>* pos_slack_var = nullptr;

    proxy_arr<GRBVar>* neg_slack_var = nullptr;

    /// Maps variables names (indexes) to their corresponding variables.
    std::list<GRBVar> name2var;
    bool started;
    unsigned constraints, offset;

    GRBEnv* e;
    GRBModel* model;
    unsigned senders, receivers, measurement_samples;

    /// Masterclean : variables having an value < threshold are automatically removed from the master.
    double threshold;

    virtual void solve_reduced_problem(
      int elements,
      unsigned offset,
      arr<>& measurement,
      arr<>& reference_signal,
      std::optional<solver> enforce_particular_solver,
      dual_solution& out,
      double& obj) override;

    virtual void add_variable(time_of_flight& variable,
                              const arr<>& reference_signal,
                              std::optional<double> warm_start_value) override;

    virtual void get_primal(std::vector<double>& primal_values) override;

    virtual void get_primal(std::vector<double>& primal_values,
                            arr<>& pos_slack,
                            arr<>& neg_slack) override;

    virtual unsigned clean() override;

    /// Generate some columns, will be called automatically on start.
    virtual void set_start_variables(int elements,
                                     arr<>& measurement,
                                     arr<>& reference_signal);
};

/// A grb_master implementation that calls std::this_thread::yield() to become slower.
class slow_grb_master : public grb_master
{
  public:
    slow_grb_master(GRBEnv* e,
                    bool verbose,
                    std::ostream& output,
                    solver s,
                    double master_solution_threshold);
    ~slow_grb_master() override;
};

#endif
