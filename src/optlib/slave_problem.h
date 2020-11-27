#ifndef SLAVE_H
#define SLAVE_H

#include "arr.h"
#include "coordinates.h"
#include <functional>
#include <optional>
#include <vector>

/// Used to find new columns.
class slave_problem
{
  public:
    /// Contains all slave_variables. Used to pass them easily around.
    template<typename T>
    struct variables
    {
        /// Maps the indexs to variables.
        proxy_arr<T>& values;

        /// Maps the indexs to variables.
        arr_2d<proxy_arr, T>& diameter_values;

        /// Maps the indexs to variables.
        arr_1d<proxy_arr, T>& quadratic_values;

        /// The x-coordinate of the representant pixel.
        T& representant_x;
    };

    /// Represents the functions needed to compute distances from indexes and backwards, caused by the ROI.
    struct distance_mapping
    {
        std::function<unsigned(unsigned)> to_distance;
        std::function<std::optional<unsigned>(unsigned)> to_index;
    };

    /// References the relaxation values for cuts.
    struct relaxation_values
    {
        double representant_x;
        std::reference_wrapper<binary_t<double>> binary;
        std::optional<std::reference_wrapper<diameter_t<double>>> diameter =
          std::nullopt;
        std::optional<std::reference_wrapper<quadratic_t<double>>> squared =
          std::nullopt;
    };

    /// Run the solver and save the results in the vars vector.
    virtual void run(proxy_arr<double>& f, columns& vars) = 0;

    /// Adds solution-hints from e.g. saft-solutions.
    virtual void add_solution_start_hints(
      size size,
      std::vector<time_of_flight>& solution_hints) = 0;

    /// Ask the slave to print its logs.
    virtual void print(bool force){};

    virtual ~slave_problem(){};
};

/// Used to find new columns asynchronously.
class slave_problem_async
{
  public:
    virtual ~slave_problem_async(){};

    /// @brief Runs the solver and calls the solution_callback for every found solution.
    ///
    /// Cancels the current optimization if the slave is already running.
    virtual void run_async(proxy_arr<double>& f) = 0;

    /// Cancels the currently running optimisation, needed when CG ends but slaves are still running.
    virtual void cancel() = 0;

    /// @brief Checks if slave is ready.
    ///
    /// Slaves may have different categories: some run until a certain optimality before ending while others can be interrupted. Non-interruptable slaves (that are currently solving something and need more time) will return false.
    virtual bool ready() = 0;
};

#endif // SLAVE_H
