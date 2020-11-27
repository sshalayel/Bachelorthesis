#ifndef CONSTRAINT_POOL_H
#define CONSTRAINT_POOL_H

#include "coordinates.h"
#include <condition_variable>
#include <list>
#include <mutex>

/// Removes entries from a list that matches a predicate.
template<typename Predicate, typename List>
struct list_predicate_checker
{
    Predicate p;
    List& l;
    unsigned i;
    list_predicate_checker(List& l, Predicate p)
      : p(p)
      , l(l)
      , i(0)
    {}

    bool operator()() { return check(); }

    /// Executes the check on all newly added list-items. Assumes that the list was not modificated except for push_back() calls.
    bool check()
    {
        bool found = false;

        //check data to see if predicate is happy
        for (auto current_column = --l.end(); i < l.size();
             ++i, --current_column) {
            if (p(*current_column)) {
                current_column = l.erase(current_column);
                i--;
                found = true;
            }
        }
        return found;
    }
};

/// Used for old-slaves statistics. Contains a columns and the id of the slave that generated it.
struct column_with_origin
{
    column c;
    unsigned slave_id;
};

/// Passes the slave results to the master for concurrent slave/master accesses.
struct constraint_pool
{
  protected:
    /// Contains the results + objective from all the slaves.
    std::list<column_with_origin> pool;

    /// Exclusive access to add or to consume solutions
    std::mutex pool_mutex;

    /// Notify when there is a solution, needed for the start or when master is too fast for the slaves.
    std::condition_variable pool_not_empty;

    unsigned slave_id = 0;
    unsigned total_consumed = 0;
    unsigned actual_unconsumed = 0;
    unsigned consumed_from_old_slave = 0;

  public:
    constraint_pool();

    constraint_pool(constraint_pool&) = delete;

    /// Set the current slave_id (only needed for statistic purposes).
    void set_current_slave_id(unsigned slave_id);

    /// Set the current slave_id (only needed for statistic purposes).
    void get_statistics(unsigned& total_consumed,
                        unsigned& consumed_from_old_slave,
                        unsigned& actual_unconsumed);

    /// Move one time-of-flight to the pool, does all the locking.
    void add(column&& c, unsigned slave_id);

    /// Moves all time of flights of the pool into out that satisfies the given predicate. Blocks if blocking is true.
    template<typename Predicate>
    bool consume(columns& out, Predicate p, bool blocking);

    /// Moves all time of flights of the pool into out that satisfies the given predicate. Blocks if none is found.
    template<typename Predicate>
    void consume(columns& out, Predicate p);

    /// Moves all time of flights of the pool into out that satisfies the given predicate. Returns false if none is found.
    template<typename Predicate>
    bool consume_non_blocking(columns& out, Predicate p);
};

template<typename Predicate>
void
constraint_pool::consume(columns& columns, Predicate p)
{
    consume(columns, p, true);
}

template<typename Predicate>
bool
constraint_pool::consume(columns& columns, Predicate p, bool blocking)
{
    std::unique_lock l{ pool_mutex };

    list_predicate_checker checker(pool, [&](column_with_origin& c) {
        bool ret = p(c) || c.c.optimality != column::NON_OPTIMAL;
        if (ret) {
            columns.push_back(std::move(c.c));

            // statistics
            total_consumed++;
            if (c.slave_id < slave_id) {
                consumed_from_old_slave++;
            }
            actual_unconsumed--;

            columns.back().stats.used_old_slave_solutions =
              consumed_from_old_slave;
            columns.back().stats.total_old_slave_solutions = total_consumed;
            columns.back().stats.solutions_in_pool = actual_unconsumed;
            columns.back().stats.actual_slave_id = c.slave_id;
        }
        return ret;
    });

    if (blocking) {
        pool_not_empty.wait(l, checker);
        return true;
    } else {
        return checker.check();
    }
}

template<typename Predicate>
bool
constraint_pool::consume_non_blocking(columns& columns, Predicate p)
{
    return consume(columns, p, false);
}

#endif
