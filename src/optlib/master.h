#ifndef MASTER_H
#define MASTER_H

#include "arr.h"
#include "convolution.h"
#include "coordinates.h"
#include "slave_problem.h"
#include <cassert>
#include <list>
#include <optional>

/// @brief Represents the Reduced Master Problem
///
/// One Constraint looks like :
/// A_{receiver, takt} * x_{receive_takt_1, ..., receive_takt_last_receiver} - s^{+}_{receiver, takt} + s^{-}_{receiver, takt}
/// = measured_signal_{receiver, takt}
/// where :
/// A_{receiver, takt} * x_{receive_takt_1, ..., receive_takt_last_receiver}
/// = \sum_{receive_takt_1, ..., receive_takt_m} x_{receive_takt_1, ..., receive_takt_last_receiver} * reference(takt - receiver)
/// so there are receiver_count * measurement_length slack variables
class master_problem
{
  public:
    /// The magical value also used by Gurobi to differ between different solving methods.
    enum solver : unsigned
    {
        SIMPLEX = 0,
        DUAL_SIMPLEX = 1,
        BARRIER = 2,
    };

    bool verbose;

    std::ostream& output;

    solver s;

    master_problem(bool verbose, std::ostream& output, solver s);
    virtual ~master_problem();

    /// Maps variables names (indexes) to their corresponding values from last solve().
    std::list<time_of_flight> name2tof;

    /// Maps variables names (indexes) to their corresponding values from last solve().
    std::vector<double> name2amplitude;

    /// Initialises the slack_variables, runs set_start_variable if not run yet and then solves the reduced master problem.
    virtual void solve_reduced_problem(
      int elements,
      unsigned offset,
      arr<>& measurement,
      arr<>& reference_signal,
      std::optional<solver> enforce_particular_solver,
      dual_solution& out,
      double& obj) = 0;

    /// Removes the unused variables (that have value < threshold in solution) from the master.
    virtual unsigned clean() = 0;

    /// Add one Variable.
    virtual void add_variable(time_of_flight& variable,
                              const arr<>& reference_signal,
                              std::optional<double> warm_start_value) = 0;

    /// Return primal solution of last solve()-call.
    virtual void get_primal(std::vector<double>& primal_values,
                            arr<>& pos_slack,
                            arr<>& neg_slack) = 0;
    /// Return primal solution of last solve()-call without slack.
    virtual void get_primal(std::vector<double>& primal_values) = 0;
};

#endif // MASTER_H