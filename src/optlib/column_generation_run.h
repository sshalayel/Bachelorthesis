#ifndef COLUMN_GENERATION_RUN_H
#define COLUMN_GENERATION_RUN_H

#include "arr.h"
#include "config.h"
#include "constraint_pool.h"
#include "coordinates.h"
#include "fftw_arr.h"
#include "master.h"
#include "slave_output_settings.h"
#include "slave_problem.h"
#include "stop_watch.h"
#include <fstream>
#include <functional>
#include <limits>
#include <optional>
#include <thread>

///@brief Helper class that contains all the state needed to run column_generation
///
/// Hides the low-level implementation details from the column_generation class.
struct column_generation_run_interface
{
    /// Runs the master for the first time.
    virtual double initial_master_run(
      master_problem& mp,
      std::vector<time_of_flight>& warm_start,
      std::optional<std::reference_wrapper<std::vector<double>>>
        warm_start_values,
      bool slowly) = 0;

    /// Checks the dualsolutionpool, executes the convolution and runs the slave.
    virtual void slave_run(slave_problem& sp, convolution& conv) = 0;

    /// Add new columns from the slave into the master and run it.
    virtual double master_update_and_run(master_problem& mp) = 0;

    virtual const columns& columns_for_masters_update() = 0;

    virtual ~column_generation_run_interface(){};
};

/// Implementation of the column_generation_run_interface.
template<template<typename> class ConvolutionArray>
struct column_generation_run : public column_generation_run_interface
{
    column_generation_run(arr<>& measurement,
                          arr<>& reference_signal,
                          config c);

    /// Runs the master for the first time.
    virtual double initial_master_run(
      master_problem& mp,
      std::vector<time_of_flight>& warm_start,
      std::optional<std::reference_wrapper<std::vector<double>>>
        warm_start_values,
      bool slowly) override;

    /// Checks the dualsolutionpool, executes the convolution and runs the slave.
    virtual void slave_run(slave_problem& sp, convolution& conv) override;

    /// Add new columns from the slave into the master and run it.
    virtual double master_update_and_run(master_problem& mp) override;

    virtual const columns& columns_for_masters_update() override;

    /// Contains the measurement.
    arr<>& measurement;

    /// Contains the reference_signal.
    arr<>& reference_signal;

    /// Contains the actual configuration.
    config c;

    /// Needed for the convolutions, is only computed once.
    arr_1d<ConvolutionArray, double> inverted_reference_signal;

    /// Thats where the slaves lays the solutions for the master.
    columns master_input;

    /// Masters dual for convolution.
    ConvolutionArray<double> dual_values;

    /// Masters dual for convolution.
    dual_solution dual;

    /// Untrimmed convoluted data.
    ConvolutionArray<double> _convoluted;

    /// Trimmed convoluted data.
    ConvolutionArray<double> convoluted;

    /// The current slave objective.
    double slave_obj;

    /// The current master objective.
    double master_obj;

    /// The statistics about the current run.
    statistics stats;

    /// Contains the time when last master ended.
    stop_watch swe;

    /// File where to print the slave_statistics.
    std::ofstream slave_statistics_output;
    /// File where to print the master_statistics.
    std::ofstream master_statistics_output;
    /// File where to print the times.
    std::ofstream time_output;

    ~column_generation_run() override {}
};

using namespace std::chrono_literals;
/// A column_generation_run with support for asynchronous slaves.
template<template<typename> class ConvolutionArray>
struct column_generation_run_async
  : public column_generation_run<ConvolutionArray>
{
    using column_generation_run<ConvolutionArray>::column_generation_run;

    /// Checks the dualsolutionpool, executes the convolution and runs the slave.
    virtual void slave_run(slave_problem& sp, convolution& conv) override;

    /// The current solutions for the master. May contain columns that the master won't see because they may not separate the dual.
    constraint_pool pool;

    /// Used to tell the slave when it can print.
    slave_output_settings can_print = { 2000ms };
};

template<template<typename> class ConvolutionArray>
column_generation_run<ConvolutionArray>::column_generation_run(
  arr<>& measurement,
  arr<>& reference_signal,
  config c)
  : measurement(measurement)
  , reference_signal(reference_signal)
  , c(c)
  , inverted_reference_signal(reference_signal.dim3)
  , dual_values(measurement.dim1, measurement.dim2, measurement.dim3)
  , dual(dual_values, master_statistics{ 0, 0, 0 })
  , _convoluted(measurement.dim1,
                measurement.dim2,
                measurement.dim3 + reference_signal.dim3 - 1)
  , convoluted(measurement.dim1, measurement.dim2, measurement.dim3)
  , slave_statistics_output(c.output + ".slavestats")
  , master_statistics_output(c.output + ".masterstats")
  , time_output(c.output + ".times")
{
    reference_signal.invert(inverted_reference_signal);
}

template<template<typename> class ConvolutionArray>
const columns&
column_generation_run<ConvolutionArray>::columns_for_masters_update()
{
    return master_input;
}

template<template<typename> class ConvolutionArray>
double
column_generation_run<ConvolutionArray>::initial_master_run(
  master_problem& mp,
  std::vector<time_of_flight>& warm_start,
  std::optional<std::reference_wrapper<std::vector<double>>> warm_start_values,
  bool slowly)
{
    swe.elapsed(stop_watch::SET_TO_ZERO);
    mp.solve_reduced_problem(this->c.elements,
                             this->c.get_roi_start(),
                             this->measurement,
                             this->reference_signal,
                             master_problem::BARRIER,
                             this->dual,
                             this->master_obj);
    stats.master_time.push_back(swe.elapsed(stop_watch::SET_TO_ZERO));
    stats.add_statistic_for_master(dual.stats);
    stats.print_last_iteration(
      master_statistics_output, slave_statistics_output, time_output);

    if (slowly) {
        for (time_of_flight& tof : warm_start) {
            master_input.push_back(
              { std::move(tof), slave_statistics{ 0, 0, 0, 0, 0, 0 } });
            master_update_and_run(mp);
        }

    } else {
        if (warm_start_values) {
            for (unsigned i = 0; i < warm_start.size(); i++) {
                mp.add_variable(warm_start[i],
                                this->reference_signal,
                                warm_start_values->get()[i]);
            }
        } else {
            for (unsigned i = 0; i < warm_start.size(); i++) {
                mp.add_variable(
                  warm_start[i], this->reference_signal, std::nullopt);
            }
        }

        if (warm_start.size() > 0) {
            mp.solve_reduced_problem(this->c.elements,
                                     this->c.get_roi_start(),
                                     this->measurement,
                                     this->reference_signal,
                                     master_problem::BARRIER,
                                     this->dual,
                                     this->master_obj);
        }
    }

    stats.master_time.push_back(swe.elapsed(stop_watch::SET_TO_ZERO));
    return this->master_obj;
}

template<template<typename> class ConvolutionArray>
void
column_generation_run<ConvolutionArray>::slave_run(slave_problem& sp,
                                                   convolution& conv)
{
    master_input.clear();

    conv.convolve(dual.values, inverted_reference_signal, _convoluted);
    assert(reference_signal.dim3 < measurement.dim3);
    // remove things interfering before the measurement, because they have negative coordinates
    _convoluted.sub_to(convoluted, 0, 0, reference_signal.dim3 - 1);
    sp.run(convoluted, master_input);
}

template<template<typename> class ConvolutionArray>
void
column_generation_run_async<ConvolutionArray>::slave_run(slave_problem& sp,
                                                         convolution& conv)
{
    this->master_input.clear();

    //TODO: tweak this epsylon!
    const double epsylon = 0.1;
    auto dual_separation_checker = [&](column_with_origin& c) {
        return c.c.tof.dot_product_with_dual(this->reference_signal,
                                             this->dual.values,
                                             this->c.get_roi_start()) > epsylon;
    };

    bool found =
      pool.consume_non_blocking(this->master_input, dual_separation_checker);

    if (found) {
        return;
    }
    assert(this->master_input.empty());

    conv.convolve(
      this->dual.values, this->inverted_reference_signal, this->_convoluted);
    assert(this->reference_signal.dim3 < this->measurement.dim3);
    // remove things interfering before the measurement, because they have negative coordinates
    this->_convoluted.sub_to(
      this->convoluted, 0, 0, this->reference_signal.dim3 - 1);

    sp.run(this->convoluted, this->master_input);

    /// Allow printing.
    {
        std::unique_lock l{ can_print.mutex };
        can_print.can_print = true;
        can_print.condition_variable.notify_all();
    }
    sp.print(true);

    //Make the master wait until there is a solution for him.
    pool.consume(this->master_input, dual_separation_checker);

    /// Stop printing.
    {
        std::unique_lock l{ can_print.mutex };
        can_print.can_print = false;
    }
}

template<template<typename> class ConvolutionArray>
double
column_generation_run<ConvolutionArray>::master_update_and_run(
  master_problem& mp)
{
    for (auto& variable : master_input) {
        mp.add_variable(variable.tof, reference_signal, std::nullopt);
        stats.add_statistic_for_next_master(variable.stats);
    }
    master_input.clear();

    stats.slave_time.push_back(swe.elapsed(stop_watch::SET_TO_ZERO));

    mp.solve_reduced_problem(this->c.elements,
                             this->c.get_roi_start(),
                             measurement,
                             reference_signal,
                             {},
                             dual,
                             master_obj);

    double master_time = swe.elapsed(stop_watch::SET_TO_ZERO);
    stats.add_statistic_for_master(dual.stats);
    stats.print_last_iteration(
      master_statistics_output, slave_statistics_output, time_output);

    /// needs to be executed after print_last_iteration.
    stats.master_time.push_back(master_time);
    return master_obj;
}

#endif //COLUMN_GENERATION_RUN_H
