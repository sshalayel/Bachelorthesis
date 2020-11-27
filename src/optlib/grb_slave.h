#ifndef GRB_SLAVE_H
#define GRB_SLAVE_H

#include "arr.h"
#include "bound_helper.h"
#include "constraint_pool.h"
#include "coordinates.h"
#include "grb_callback.h"
#include "gurobi_c++.h"
#include "slave_constraints_generator.h"
#include "slave_problem.h"
#include "statistics.h"
#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <sstream>
#include <tuple>

/// A slave_problem implementation using Gurobi.
class grb_slave : public slave_problem
{
  protected:
    /// Generates the problem (without the objective).
    void generate(size size);
    /// Generates the objective.
    void update_objective(proxy_arr<double>& f);

  public:
    double element_pitch_in_tacts;
    const double squared_pitch;
    std::optional<double> slavestop;
    unsigned offset;
    std::optional<double> horizontal_roi_start;
    std::optional<double> horizontal_roi_end;
    std::ostream& output;
    bool verbose;
    bool started;
    bool generated;
    symmetric_arr<proxy_arr<GRBVar>> vars_proxy;
    symmetric_arr<proxy_arr<double>> vars_objective;
    arr_1d<proxy_arr, GRBVar> squared_vars_proxy;
    symmetric_arr_2d<proxy_arr, GRBVar> diameter_vars_proxy;
    GRBModel model;

    /// The x-coordinate of the representant of the current cell, where the 0th element forms the origin.
    GRBVar representant_x;

    grb_resettable_callback* grb_callback;

    variables<GRBVar> variables();

    distance_mapping mapping();

    void run(proxy_arr<double>& f, columns& vars) override;
    void add_solution_start_hints(size size,
                                  std::vector<time_of_flight>& starts) override;

    virtual void prepare_for_solving(proxy_arr<double>& f);

    virtual void get_results(columns& vars);

    virtual unsigned distance_mapping(unsigned k) const;
    virtual std::optional<unsigned> revert_distance_mapping(
      unsigned distance) const;

    /// Generates all constraints for the slave by calling all generate_*_constraints-methods.
    void generate_constraints(size f);
    /// Dump an infeasible.lp if LP is infeasible.
    static bool assert_feasibility(GRBModel& m);

    /// Transforms the continuous d-variable into the discrete ToF. Everybody gets rounded in the same way.
    void discretise_slave_result(
      std::function<double(unsigned, unsigned)> values,
      time_of_flight& fill_me);

    /// Fills a ToF with values from the field, even those for cgdump2.
    void fill_tof(
      std::function<double(unsigned, unsigned, unsigned)> binaries,
      std::function<double(unsigned, unsigned, unsigned)> diameters,
      std::function<double(unsigned, unsigned, unsigned)> quadratics,
      double x,
      time_of_flight& fill_me,
      bool verbose = true);

    void binary_to_tof(
      std::function<double(unsigned, unsigned, unsigned)> binaries,
      time_of_flight& tof);

    grb_slave(GRBEnv* e,
              double element_pitch_in_tacts,
              std::optional<double> slavestop,
              unsigned offset,
              std::optional<double> horizontal_roi_start,
              std::optional<double> horizontal_roi_end,
              std::ostream& output,
              grb_resettable_callback* grb_callback,
              bool verbose);

    ~grb_slave() override;
};

#endif
