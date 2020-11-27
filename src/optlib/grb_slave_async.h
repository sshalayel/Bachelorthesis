#ifndef GRB_SLAVE_ASYNC_H
#define GRB_SLAVE_ASYNC_H
#include "slave_problem.h"
#include <thread>

/// Used to find new columns asynchronously.
template<typename Callback>
class grb_slave_async
  : public grb_slave
  , public slave_problem_async
{
  public:
    grb_slave_async(double element_pitch_in_tacts,
                    double slavestop,
                    unsigned offset,
                    std::optional<double> horizontal_roi_start,
                    std::optional<double> horizontal_roi_end,
                    std::ostream& output,
                    std::mutex& output_mutex,
                    bool verbose,
                    Callback solution_callback,
                    double threshold,
                    slave_cut_options slave_cuts,
                    std::string prefix_for_output,
                    slave_callback_options options);

    ~grb_slave_async() override;

    Callback solution_callback;

    GRBEnv* env;

    double threshold;

    /// Slaves cannot be interrupted until the current MIPGap is smaller then min_gap.
    std::optional<double> max_allowed_gap;

    /// A thread because gurobi misses a optimisation-ended-callback.
    std::thread thread;

    ///@brief Returns the current MIPGap, because the Gurobi MIPGap attribute throws errors when optimizing asynchronously.
    ///
    /// GRB does not throw exceptions when querying the values needed to compute the mip-gap, so we get them and compute it manually.
    double get_MIP_gap() const;

    void cancel() override;
    void run_async(proxy_arr<double>& f) override;
    bool ready() override;
};

template<typename Callback>
double
grb_slave_async<Callback>::get_MIP_gap() const
{
    const double bound = model.get(GRB_DoubleAttr_ObjBound);
    const double val = model.get(GRB_DoubleAttr_ObjVal);
    return std::fabs(bound - val) / std::fabs(val);
}

template<typename Callback>
bool
grb_slave_async<Callback>::ready()
{
    try {
        const bool ready =
          model.get(GRB_IntAttr_Status) != GRB_INPROGRESS || !max_allowed_gap ||
          (max_allowed_gap && get_MIP_gap() < *max_allowed_gap);
        return ready;
    } catch (GRBException& e) {
        std::cerr << e.getMessage();
        abort();
    }
}

template<typename Callback>
void
grb_slave_async<Callback>::cancel()
{
    if (model.get(GRB_IntAttr_Status) == GRB_INPROGRESS) {
        //std::cout << "Interrupted with MIPGap " << get_MIP_gap() << "%/ "
        //<< max_allowed_gap.value_or(-1) << "%" << std::endl;
        model.terminate();
    }

    /// Wait for thread to end (it should end now because optimisation was aborted).
    if (thread.joinable()) {
        thread.join();
    }
}

template<typename Callback>
void
grb_slave_async<Callback>::run_async(proxy_arr<double>& f)
{
    cancel();
    prepare_for_solving(f);

    thread = std::thread(
      [](grb_slave_async<Callback>* self) {
          self->model.optimizeasync();
          self->model.sync();
          // make sure it is not infeasible.
          assert_that(assert_feasibility(self->model), "Infeasible slave!");

          // Get the optimal result if ignored in callback because smaller then threshold.
          int status = self->model.get(GRB_IntAttr_Status);
          bool optimal = status == GRB_OPTIMAL;
          bool user_objective_reached = status == GRB_USER_OBJ_LIMIT;

          typename column::optimality optimality =
            optimal ? column::OPTIMAL : column::USER_BOUND_REACHED;

          if (optimal || user_objective_reached) {
              columns c;
              self->get_results(c);
              for (column& col : c) {
                  col.optimality = optimality;
                  // add if not already added
                  if (col.stats.objective < self->threshold) {
                      self->solution_callback(std::move(col));
                  }
              }
          }
      },
      this);
}

template<typename Callback>
grb_slave_async<Callback>::~grb_slave_async()
{
    if (model.get(GRB_IntAttr_Status) == GRB_INPROGRESS) {
        model.terminate();
    }

    /// Wait for thread to end (it should end now because optimisation was aborted).
    if (thread.joinable()) {
        thread.join();
    }

    delete env;
}

template<typename Callback>
grb_slave_async<Callback>::grb_slave_async(
  double element_pitch_in_tacts,
  double slavestop,
  unsigned offset,
  std::optional<double> horizontal_roi_start,
  std::optional<double> horizontal_roi_end,
  std::ostream& output,
  std::mutex& output_mutex,
  bool verbose,
  Callback solution_callback,
  double threshold,
  slave_cut_options slave_cuts,
  std::string prefix_for_output,
  slave_callback_options options)
  : grb_slave(env = new GRBEnv(),
              element_pitch_in_tacts,
              slavestop,
              offset,
              horizontal_roi_start,
              horizontal_roi_end,
              output,
              grb_no_op_callback(),
              verbose)
  , solution_callback(solution_callback)
  , threshold(threshold)
{
    /// grb_callback_constructor needs grb_slave, so it have to be constructed after grb_slave constructor finished
    this->grb_callback = new grb_slave_async_cb<grb_slave_async<Callback>>(
      *this, output, output_mutex, slave_cuts, prefix_for_output, options);
    if (verbose) {
        this->model.setCallback(grb_callback);
    }
}

#endif
