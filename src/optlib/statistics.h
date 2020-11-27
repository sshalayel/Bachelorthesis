#ifndef STATISTICS_H
#define STATISTICS_H

#include "csv_tools.h"
#include <chrono>
#include <vector>

/// Contains information about which constraints where not respected for some add_cuts()-run.
struct cut_statistics
{
    unsigned non_diagonal = 0;
    unsigned cosine_rule = 0;
    unsigned pythagoras = 0;
    unsigned tangent = 0;
    unsigned total() { return pythagoras + non_diagonal + cosine_rule; }
};

/// Contains data that can be plotted in python about the run and the slaves behaviour.
struct slave_statistics
{
    double objective;
    double elapsed_run_time;
    double best_objective;
    double best_objective_bound;
    double explored_node_count;
    int feasible_solutions_count;

    ///Filled by the pool.
    unsigned used_old_slave_solutions = 0;
    unsigned total_old_slave_solutions = 0;
    unsigned solutions_in_pool = 0;
    unsigned actual_slave_id = 0;
};

/// Same as slave_statistics but for the master.
struct master_statistics
{
    double objective;
    double elapsed_run_time;
    double explored_node_count;
};

/// Collection of master_statistics and slave_statistics. Dumps the information to disk.
struct statistics
{
    /// Slave runs per Iteration.
    std::vector<std::vector<slave_statistics>> slave_runs;
    /// Master runs per Iteration.
    std::vector<master_statistics> master_runs;
    /// Master runtime per Iteration.
    std::vector<double> master_time;
    /// (Total) Slave runtime per Iteration.
    std::vector<double> slave_time;

    /// Add slave_statistic for next master-run.
    void add_statistic_for_next_master(slave_statistics s)
    {
        current_master_slave_runs.push_back(s);
    }

    /// Add master_run, marks the beginning of a new iteration.
    void add_statistic_for_master(master_statistics m)
    {
        master_runs.push_back(m);
        slave_runs.push_back(std::move(current_master_slave_runs));
    }

    /// Prints the last iteration to disk.
    void print_last_iteration(std::ostream& master_stream,
                              std::ostream& slave_stream,
                              std::ostream& time_stream)
    {
        if (master_runs.size() == 1) {
            master_stream << "#objective;elapsed_run_time;explored_node_count;"
                          << std::endl;
            time_stream << "#master_time;slave_time" << std::endl;
        }
        master_statistics& m = master_runs.back();
        insert_with_semicolon(master_stream, m.objective);
        insert_with_semicolon(master_stream, m.elapsed_run_time);
        insert_with_semicolon(master_stream, m.explored_node_count);
        master_stream << std::endl;

        if (slave_runs.size() == 1) {
            slave_stream << "#iteration;objective;elapsed_run_time;best_"
                            "objective;best_objective_"
                            "bound;explored_node_count;feasible_solutions_"
                            "count;used_old_slave_solutions;total_old_"
                            "slave_solutions;solutions_in_pool"
                         << std::endl;
        }

        std::vector<slave_statistics>& ss = slave_runs.back();
        const int current_master = slave_runs.size() - 1;
        for (slave_statistics& s : ss) {
            insert_with_semicolon(slave_stream, current_master);

            insert_with_semicolon(slave_stream, s.objective);
            insert_with_semicolon(slave_stream, s.elapsed_run_time);
            insert_with_semicolon(slave_stream, s.best_objective);
            insert_with_semicolon(slave_stream, s.best_objective_bound);
            insert_with_semicolon(slave_stream, s.explored_node_count);
            insert_with_semicolon(slave_stream, s.feasible_solutions_count);
            insert_with_semicolon(slave_stream, s.used_old_slave_solutions);
            insert_with_semicolon(slave_stream, s.total_old_slave_solutions);
            insert_with_semicolon(slave_stream, s.solutions_in_pool);
            insert_with_semicolon(slave_stream, s.actual_slave_id);
            slave_stream << std::endl;
        }

        insert_with_semicolon(time_stream, master_time.back());
        double current_slave_time = slave_time.empty() ? 0 : slave_time.back();
        insert_with_semicolon(time_stream, current_slave_time);

        time_stream << std::endl;
    }

  protected:
    std::vector<slave_statistics> current_master_slave_runs;
};

#endif
