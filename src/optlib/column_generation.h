#ifndef COLUMN_GENERATION_H
#define COLUMN_GENERATION_H

#include "column_generation_run.h"
#include "config.h"
#include "coordinates.h"
#include "iterator.h"
#include "master.h"
#include "printer.h"
#include "slave_problem.h"
#include "stop_watch.h"
#include "writer.h"
#include <functional>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>

/// Contains all values needed for warm_starting.
struct warm_start
{
    /// The solutions to be added directly to the master (from a previous run to be continued).
    std::vector<time_of_flight>& for_master;
    /// The values that the variables in warm-start had in the previous run.
    std::optional<std::reference_wrapper<std::vector<double>>>
      for_master_values;
    /// @brief The solutions that will be added to the slave, e.g. from SAFT.
    ///
    /// Instead of directly adding a lot of solutions to the master, they can be added via the slave, that then act as a filtering method and only takes the best one.
    std::vector<time_of_flight>& for_slave;
    bool slow_warm_start = false;

    bool integrity_check();
};

/// Class containing everything needed to perform Column Generation.
class column_generation
{
  protected:
    column_generation(config c, arr<>& measurement, arr<>& reference_signal);

    config c;
    arr<>& measurement;
    arr<>& reference_signal;

    /// The implementation to be used.
    master_problem* master;
    /// The implementation to be used.
    slave_problem* slave;
    /// The implementation to be used.
    printer* print;
    /// The implementation to be used.
    convolution* conv;
    /// The implementation to be used.
    writer<double>* write;

    column_generation_run_interface* instance;

    std::ostream& output;

  public:
    virtual ~column_generation();

    /// Executes CG on given data, and stops when maxcolumns or slave_objective < threshold.
    virtual void run(warm_start warm,
                     std::vector<time_of_flight>& reflectors_out,
                     std::vector<double>& amplitude_out);

    void dump_warm_start_solutions() const;
    void dump(std::optional<unsigned> iteration) const;
    void dump(std::ostream& os,
              std::vector<double>& values,
              input_iterator<time_of_flight>&& tofs) const;
    void dump(const std::string& s,
              std::vector<double>& values,
              input_iterator<time_of_flight>&& tofs) const;
};

#endif // COLUMN_GENERATION_H