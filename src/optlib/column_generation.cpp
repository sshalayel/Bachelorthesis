#include "column_generation.h"

bool
warm_start::integrity_check()
{
    if (for_master_values) {
        return for_master.size() == for_master_values->get().size();
    }
    return true;
}

column_generation::column_generation(config c,
                                     arr<>& measurement,
                                     arr<>& reference_signal)
  : c(c)
  , measurement(measurement)
  , reference_signal(reference_signal)
  , output(c.output.empty() ? std::cout : *new ofstream_with_dirs(c.output))
{
    if (!c.verbose) {
        print = new muted_printer();
        write = new muted_writer<double>();
    } else {
        print = new simple_printer(output, -1);
        write = new binary_writer<double>();
    }
}

column_generation::~column_generation()
{
    delete print;
    delete write;
    if (!c.output.empty()) {
        delete &output;
    }
}

void
column_generation::dump_warm_start_solutions() const
{
    dump(std::nullopt);
}

void
column_generation::dump(std::optional<unsigned> iteration) const
{
    if (!c.output.empty()) {
        std::string image = c.output;
        image += '_';
        image += iteration ? std::to_string(*iteration) : "warm";
        image += ".cgdump";

        dump(image,
             master->name2amplitude,
             container_input_iterator{ master->name2tof });
    }
}

void
column_generation::dump(const std::string& s,
                        std::vector<double>& values,
                        input_iterator<time_of_flight>&& tofs) const
{
    ofstream_with_dirs os{ s };
    assert_read(os, "Cannot open file!");
    dump(os, values, std::forward<input_iterator<time_of_flight>>(tofs));
}

void
column_generation::dump(std::ostream& os,
                        std::vector<double>& values,
                        input_iterator<time_of_flight>&& tofs) const
{
    //assert(values.size() == tofs.size());

    for (double& value : values) {
        value = std::fmax(value, 0.0);
    }

    c.save(os, { tofs }, { values });
}

void
column_generation::run(warm_start warm_start,
                       std::vector<time_of_flight>& reflectors_out,
                       std::vector<double>& amplitude_out)
{
    bool optimal = false;
    assert(warm_start.integrity_check() &&
           "No values given for all warm_start_solutions");

    stop_watch total_time;

    if (warm_start.for_master.size() > 0 && warm_start.for_master_values) {
        dump(c.output + "_input.cgdump",
             warm_start.for_master_values->get(),
             container_input_iterator{ warm_start.for_master });
        print->print_log(
          "(CG) Warm master starts empty and then a second time with " +
          std::to_string(warm_start.for_master.size()) + " candidates");
    } else {
        print->print_log("(CG) Empty master start");
    }
    double master_obj =
      instance->initial_master_run(*master,
                                   warm_start.for_master,
                                   warm_start.for_master_values,
                                   warm_start.slow_warm_start);
    print->print_log("(CG) Master Objective : " + std::to_string(master_obj));

    print->print_log("(CG) Adding " +
                     std::to_string(warm_start.for_slave.size()) +
                     " start-hints to slave");
    slave->add_solution_start_hints(measurement, warm_start.for_slave);
    // not needed anymore
    warm_start.for_slave.clear();

    dump_warm_start_solutions();

    for (unsigned iteration = 0; iteration < c.max_columns; iteration++) {
        print->print_log("(CG) Running Slave, iteration " +
                         std::to_string(iteration + 1));
        instance->slave_run(*slave, *conv);

        double slave_obj =
          instance->columns_for_masters_update().front().stats.objective;
        for (const auto& column : instance->columns_for_masters_update()) {
            slave_obj = std::max(slave_obj, column.stats.objective);

            std::stringstream ss;
            ss << "(CG) Slave generates (obj:" << column.stats.objective << ")";
            if (column.tof.representant_x) {
                ss << " with x = " << *column.tof.representant_x;
            }

            print->print_log(ss.str(), column.tof);
        }

        optimal = slave_obj <= c.slave_threshold;
        if (optimal) {
            print->print_log("(CG) Optimal Solution found!");
            break;
        }

        print->print_log("(CG) Running Master, iteration " +
                         std::to_string(iteration + 1));
        master_obj = instance->master_update_and_run(*master);
        print->print_log("(CG) Master Objective : " +
                         std::to_string(master_obj));
        unsigned variables_cleaned_from_master = master->clean();
        if (variables_cleaned_from_master > 0) {
            std::stringstream ss;
            ss << "(MasterCleaning) Removed " << variables_cleaned_from_master
               << " variables from master that were below the "
                  "master_solution_threshold "
               << c.master_solution_threshold.value_or(0.0);
            print->print_log(ss.str());
        }
        optimal = master_obj <= c.master_threshold;
        if (optimal) {
            print->print_log("(CG) Master-solution is under Master-threshold " +
                             std::to_string(c.master_threshold));
            break;
        }

        dump(iteration);
    }

    std::stringstream results;
    results << "(CG) ended!\nFinal Master objective: " << master_obj
            << "\nColumns in Master: " << master->name2amplitude.size()
            << "\nNeeded time: " << total_time.elapsed() / 60.0 << " minutes";
    print->print_log(results.str());

    // write results
    for (auto& x : master->name2tof) {
        reflectors_out.push_back(std::move(x));
    }
    master->get_primal(amplitude_out);
}
