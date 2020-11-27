#ifndef GRB_MULTIPLE_SLAVES_ASYNC_H
#define GRB_MULTIPLE_SLAVES_ASYNC_H

#include "cut_helper.h"
#include "grb_slave.h"
#include "grb_slave_async.h"
#include "slave_output_settings.h"
#include <chrono>
#include <mutex>
#include <string>

/// Uses multiple async slaves to find new columns.
class grb_multiple_slave_async
  : public slave_problem
  , slave_problem_async
{
  public:
    /// @brief The lambda for the async slaves.
    ///
    /// This lambda has a name to be able to put the async_slaves<lambda> in a vector as a field in this class.
    class constraint_pool_adder
    {
      public:
        constraint_pool& pool;
        unsigned& slave_id;
        constraint_pool_adder(constraint_pool& pool, unsigned& slave_id);

        void operator()(column&& c);
    };

    ///Helper struct to get readable gurobi output
    struct locked_ostream
    {
      public:
        std::stringstream output;
        std::mutex lock;
    };

    using slave_type = grb_slave_async<constraint_pool_adder>;

    std::ostream& output;
    bool verbose;

    grb_multiple_slave_async(double element_pitch_in_tacts,
                             double slave_threshold,
                             double slavestop,
                             unsigned offset,
                             std::optional<double> horizontal_roi_start,
                             std::optional<double> horizontal_roi_end,
                             std::ostream& output,
                             bool verbose,
                             unsigned max_parallel_slaves,
                             constraint_pool& pool,
                             slave_output_settings& can_print,
                             slave_cut_options slave_cuts,
                             std::string prefix_for_output,
                             slave_callback_options options);

    ~grb_multiple_slave_async() override;

    /// Number of async slaves to use.
    unsigned max_parallel_slaves;
    /// Slave that is next on worklist.
    std::list<slave_type>::iterator current_slave;
    /// ID of slave that is next on worklist.
    std::list<unsigned>::iterator current_slave_id;
    /// Used to filter non-optimal slave solutions out. Should be > 0.
    double threshold;

    constraint_pool& pool;

    /// Contains the async slaves.
    std::list<slave_type> slave_pool;
    std::list<unsigned> slave_ids;
    std::list<locked_ostream> slave_output;

    unsigned next_slave_id;
    /// Modifies the iterators to get the next slave.
    void next_slave();

    void run(proxy_arr<double>& f, columns& columns) override;
    void add_solution_start_hints(size size,
                                  std::vector<time_of_flight>& starts) override;
    void cancel() override;
    void run_async(proxy_arr<double>& f) override;
    bool ready() override;

    /// Prints the output of the slaves, is done in a separate thread.
    void print(bool force) override;
    /// Prints the output of the slaves, is done in a separate thread.
    std::thread printer;
    /// Represents when the slave is allowed to print, set from outside.
    slave_output_settings& can_print;

    /// Used to end the thread in the destructor.
    bool alive();
    /// Used to end the thread in the destructor.
    void stop_thread();
    /// Use alive() to get.
    bool _alive = true;
    /// Used to end the thread in the destructor.
    std::mutex alive_mutex;
};

#endif
