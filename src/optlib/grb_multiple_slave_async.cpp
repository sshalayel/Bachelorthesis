#include "grb_multiple_slave_async.h"

grb_multiple_slave_async::constraint_pool_adder::constraint_pool_adder(
  constraint_pool& pool,
  unsigned& slave_id)
  : pool(pool)
  , slave_id(slave_id)
{}

void
grb_multiple_slave_async::constraint_pool_adder::operator()(column&& c)
{
    pool.add(std::move(c), slave_id);
}

grb_multiple_slave_async::grb_multiple_slave_async(
  double element_pitch_in_tacts,
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
  slave_callback_options options)

  : output(output)
  , verbose(verbose)
  , max_parallel_slaves(max_parallel_slaves)
  , threshold(slave_threshold)
  , pool(pool)
  , next_slave_id(1)
  , can_print(can_print)
{
    assert(threshold >= 0 && "Threshold should be >= 0");

    for (unsigned i = 0; i < max_parallel_slaves; i++) {
        slave_ids.push_back(0);
        slave_output.emplace_back();
        slave_pool.emplace_back(element_pitch_in_tacts,
                                slavestop,
                                offset,

                                horizontal_roi_start,
                                horizontal_roi_end,

                                slave_output.back().output,
                                slave_output.back().lock,
                                verbose,
                                constraint_pool_adder(pool, slave_ids.back()),
                                threshold,
                                slave_cuts,
                                prefix_for_output,
                                options);

        // set categories :for 4 we have one with 25%, one with 50% and 2 unbounded.
        if (i < std::log2(max_parallel_slaves)) {
            double max_allowed_gap = (double)(1 << i) / max_parallel_slaves;
            if (max_allowed_gap < 1.0) {
                slave_pool.back().max_allowed_gap = max_allowed_gap;
            }
        }
    }
    current_slave_id = slave_ids.begin();
    current_slave = slave_pool.begin();
    printer = std::thread([&]() {
        while (alive()) {
            print(false);
            std::this_thread::sleep_for(can_print.printing_interval);
        }
    });
}

bool
grb_multiple_slave_async::ready()
{
    return true;
}

grb_multiple_slave_async::~grb_multiple_slave_async()
{
    stop_thread();
    printer.join();
}

void
grb_multiple_slave_async::next_slave()
{
    if (++current_slave == slave_pool.end()) {
        current_slave = slave_pool.begin();
        current_slave_id = slave_ids.begin();
    } else {
        ++current_slave_id;
    }
}

void
grb_multiple_slave_async::cancel()
{
    for (auto& slave : slave_pool) {
        slave.cancel();
    }
}

void
grb_multiple_slave_async::run_async(proxy_arr<double>& f)
{
    while (!current_slave->ready()) {
        next_slave();
    }

    *current_slave_id = next_slave_id++;
    pool.set_current_slave_id(*current_slave_id);

    current_slave->run_async(f);
    next_slave();

    unsigned total_consumed, consumed_from_old_slave, actual_unconsumed;
    pool.get_statistics(
      total_consumed, consumed_from_old_slave, actual_unconsumed);

    if (verbose) {
        output << " ==== Current Pool-statistics :" << consumed_from_old_slave
               << "/" << total_consumed
               << " old-slave-constraints separated master and actual size :"
               << actual_unconsumed << " ====\n";
    }
}

bool
grb_multiple_slave_async::alive()
{
    std::unique_lock l{ alive_mutex };
    return _alive;
}

void
grb_multiple_slave_async::stop_thread()
{
    std::unique_lock l{ alive_mutex };
    _alive = false;
}

void
grb_multiple_slave_async::print(bool force)
{
    {
        std::unique_lock l{ can_print.mutex };
        if (!force) {
            can_print.condition_variable.wait_for(
              l, can_print.printing_interval, [&]() {
                  return can_print.can_print;
              });
        }

        unsigned count = 0;
        // no need to lock slave_output itself: is it only accessed here and in the constructor.
        for (locked_ostream& o : slave_output) {
            ++count;
            std::unique_lock l(o.lock);
            //output only if something was changed
            o.output.flush();

            std::string current_line;
            while (std::getline(o.output, current_line)) {
                output << " ====(Slave " << count << ") ==== " << current_line
                       << std::endl;
            }
            o.output.str({});
            o.output.clear();
        }
        output.flush();
    }
}

void
grb_multiple_slave_async::run(proxy_arr<double>& f, columns& columns)
{
    run_async(f);
}

void
grb_multiple_slave_async::add_solution_start_hints(
  size size,
  std::vector<time_of_flight>& starts)
{
    if (starts.empty()) {
        return;
    }
    const unsigned starts_per_slave =
      std::ceil(starts.size() / slave_pool.size());
    assert(starts_per_slave > 0);
    std::vector<time_of_flight> split;
    split.reserve(starts_per_slave);

    unsigned current = 0;

    for (slave_type& sp : slave_pool) {
        split.clear();
        const long unsigned start = current * starts_per_slave;
        // bool is true when end of starts is reached.
        const unsigned end = std::min(starts.size(), start + starts_per_slave);

        for (unsigned i = start; i < end; i++) {
            split.push_back(std::move(starts[i]));
        }

        sp.add_solution_start_hints(size, split);

        if (end >= starts.size()) {
            return;
        }
        current++;
    }
}
