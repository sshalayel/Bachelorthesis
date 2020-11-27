#include "optlib/cgdump_analyser.h"
#include "optlib/column_generation.h"
#include "optlib/config.h"
#include "optlib/exception.h"
#include "optlib/grb_column_generation.h"
#include "optlib/reader.h"
#include "optlib/roi_helper.h"
#include "optlib/saft.h"
#include <functional>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <optional>

const struct option options[] = {
    { "help", no_argument, nullptr, 'h' },
    { "verbose", no_argument, nullptr, 'v' },
    { "measurement_file", required_argument, nullptr, 'f' },
    { "reference_file", required_argument, nullptr, 'g' },
    { "max_columns", required_argument, nullptr, 'm' },
    { "slave_threshold", required_argument, nullptr, 't' },
    { "master_threshold", required_argument, nullptr, '&' },
    { "x_position", required_argument, nullptr, 'x' },
    { "pitch", required_argument, nullptr, 'p' },
    { "sampling_rate", required_argument, nullptr, 'r' },
    { "wave_speed", required_argument, nullptr, 'c' },
    { "elements", required_argument, nullptr, 'e' },
    { "wave_length", required_argument, nullptr, 'l' },
    { "samples", required_argument, nullptr, 's' },
    { "reference_samples", required_argument, nullptr, 'a' },
    { "output", required_argument, nullptr, 'o' },
    { "slavestop", required_argument, nullptr, 'S' },
    { "offset", required_argument, nullptr, 'O' },
    { "csv", required_argument, nullptr, 'C' },
    { "warm_start", required_argument, nullptr, 'w' },
    { "slave_warm_start", required_argument, nullptr, '#' },
    { "saft", required_argument, nullptr, 'W' },
    { "no_warm_start_values", no_argument, nullptr, 'n' },
    { "roi_start", required_argument, nullptr, 'R' },
    { "roi_end", required_argument, nullptr, '?' },
    { "horizontal_roi_start", required_argument, nullptr, '%' },
    { "horizontal_roi_end", required_argument, nullptr, '^' },
    { "master_solver", required_argument, nullptr, '!' },
    { "saft_resolution", required_argument, nullptr, '@' },
    { "slow_warm_start", no_argument, nullptr, '$' },
    { "master_solution_threshold", required_argument, nullptr, '*' },
    { "slave_cuts", no_argument, nullptr, '(' },
    { "compare_slave_cuts", no_argument, nullptr, ')' },
    { "slaves", required_argument, nullptr, '_' },
    { "analyse_with_slave", required_argument, nullptr, '+' },
    { "analyse_xy_with_slave", required_argument, nullptr, '[' },
    { "no_randomisation", no_argument, nullptr, '-' },
    { "no_rounding_down", no_argument, nullptr, '=' },
    { "no_tangents", no_argument, nullptr, ']' },
    { 0, 0, 0, 0 },
};
const char* short_options = "hvf:g:m:t:x:p:r:c:e:l:s:a:o:S:o:C:w:W:nR:?:!:#:";

int
main(int argc, char** argv)
{
    config c;
    char current;

    bool c_loaded = false;
    bool slow_warm_start = false;
    std::optional<double> saft_threshold;
    bool no_warm_start_values = false;
    slave_cut_options slave_cuts = slave_cut_options::OFF;
    unsigned slaves = 4;
    std::optional<config> analyse_with_slave;
    std::optional<config> analyse_xy_with_slave;
    std::vector<time_of_flight> to_be_analysed;
    std::vector<time_of_flight> to_be_xy_analysed;

    std::vector<config> rewrite_with_slave_config;
    std::vector<std::string> rewrite_with_slave_config_name;
    std::vector<std::vector<time_of_flight>> rewrite_with_slave_tofs;
    std::vector<std::vector<double>> rewrite_with_slave_vals;

    master_problem::solver master_solver = master_problem::SIMPLEX;
    std::vector<time_of_flight> master_warm_start;
    std::vector<time_of_flight> filtered_master_warm_start;
    std::vector<double> master_warm_start_values;
    std::vector<double> filtered_master_warm_start_values;
    std::vector<time_of_flight> warm_start_for_slave;
    std::vector<time_of_flight> filtered_warm_start_for_slave;
    std::optional<unsigned> saft_resolution;
    slave_callback_options cb_options = static_cast<slave_callback_options>(
      slave_callback_options::LAZY_TANGENTS |
      slave_callback_options::RANDOMISE |
      slave_callback_options::ROUNDING_DOWN);

    unsigned count = 0;
    while ((current =
              getopt_long(argc, argv, short_options, options, nullptr)) != -1) {
        count++;
        switch (current) {
            case '[': {
                std::ifstream is(optarg);

                analyse_xy_with_slave = config();
                c_loaded =
                  analyse_xy_with_slave->load(is, to_be_xy_analysed, {});
                break;
            }
            case '+': {
                std::ifstream is(optarg);

                analyse_with_slave = config();
                c_loaded = analyse_with_slave->load(is, to_be_analysed, {});
                break;
            }
            case '-': {
                cb_options = static_cast<slave_callback_options>(
                  cb_options & ~slave_callback_options::RANDOMISE);
                break;
            }
            case '=': {
                cb_options = static_cast<slave_callback_options>(
                  cb_options & ~slave_callback_options::ROUNDING_DOWN);
                break;
            }
            case ']': {
                cb_options = static_cast<slave_callback_options>(
                  cb_options & ~slave_callback_options::LAZY_TANGENTS);
                break;
            }
            case 'S': {
                c.slavestop = std::stod(optarg);
                break;
            }
            case 'f': {
                c.measurement_file = optarg;
                break;
            }
            case 'o': {
                c.output = optarg;
                break;
            }
            case 'g': {
                c.reference_file = optarg;
                break;
            }
            case 'm': {
                c.max_columns = std::stoi(optarg);
                break;
            }
            case 't': {
                c.slave_threshold = std::stod(optarg);
                break;
            }
            case '*': {
                c.master_solution_threshold = std::stod(optarg);
                break;
            }
            case '(': {
                slave_cuts = slave_cut_options::GREEDY_CHOOSER;
                break;
            }
            case ')': {
                slave_cuts = slave_cut_options::COMPARE_BOTH_CHOOSER;
                break;
            }
            case '_': {
                slaves = std::stoul(optarg);
                break;
            }
            case '&': {
                c.master_threshold = std::stod(optarg);
                break;
            }
            case 'x': {
                c.x_position = std::stod(optarg);
                break;
            }
            case 'p': {
                c.pitch = std::stod(optarg);
                break;
            }
            case 'r': {
                c.sampling_rate = (unsigned)std::stod(optarg);
                break;
            }
            case 'c': {
                c.wave_speed = std::stoi(optarg);
                break;
            }
            case 'e': {
                c.elements = std::stoi(optarg);
                break;
            }
            case 'l': {
                c.wave_length = std::stod(optarg);
                break;
            }
            case 's': {
                c.samples = std::stoi(optarg);
                break;
            }
            case 'a': {
                c.reference_samples = std::stoi(optarg);
                break;
            }
            case 'v': {
                c.verbose = true;
                break;
            }
            case 'n': {
                no_warm_start_values = true;
                break;
            }
            case 'W': {
                saft_threshold = std::stod(optarg);
                break;
            }
            case '!': {
                master_solver = (master_problem::solver)std::stoi(optarg);
                assert_that(master_solver >= 0 && master_solver <= 2,
                            "Only 3 solvers available : Simplex(0), Dual "
                            "Simplex(1) and Barrier(2)");
                break;
            }
            case '@': {
                saft_resolution = std::stoul(optarg);
                break;
            }
            case '$': {
                slow_warm_start = true;
                break;
            }
            case '#':
            case 'w': {
                const bool for_master = current == 'w';
                std::ifstream is(optarg);
                config _dummy;
                config& dummy = c_loaded ? _dummy : c;
                bool success = dummy.load(
                  is,
                  for_master ? master_warm_start : warm_start_for_slave,
                  for_master
                    ? master_warm_start_values
                    : std::optional<
                        std::reference_wrapper<std::vector<double>>>{});
                if (c_loaded) {
                    if (dummy != c) {
                        std::cerr << "Warning: using warm_start_values created "
                                     "with a different config!"
                                  << std::endl;
                    }
                } else {
                    c_loaded = success;
                }
                break;
            }
            case 'C': {
                std::ifstream is(optarg);
                c_loaded =
                  c.load(is, master_warm_start, master_warm_start_values);
                break;
            }
            case 'O': {
                c.offset = std::stoul(optarg);
                break;
            }
            case 'R': {
                c.roi_start = std::stoul(optarg);
                assert_that(*c.roi_start >= c.offset,
                            "ROI-Start should be bigger then the offset " +
                              std::to_string(c.offset));
                break;
            }
            case '?': {
                c.roi_end = std::stoul(optarg);
                assert_that(
                  *c.roi_end < c.samples + c.offset,
                  "ROI-End should be before the end of the measurement " +
                    std::to_string(c.samples + c.offset));
                break;
            }
            case '%': {
                const double start = c.get_roi_end() * -1.0;
                c.horizontal_roi_start = std::stod(optarg);
                assert_that(*c.horizontal_roi_start >= start,
                            "Horizontal ROI-Start should be before the end of "
                            "the measurement " +
                              std::to_string(start));
                break;
            }
            case '^': {
                const double end = c.get_roi_end();
                c.horizontal_roi_end = std::stod(optarg);
                assert_that(*c.horizontal_roi_end <= end,
                            "Horizontal ROI-End should be after the start of "
                            "the measurement " +
                              std::to_string(end));
                break;
            }
            case 'h':
            default: {
                std::cout << "Possible options:\n";
                unsigned x = 2; //number of args without options.
                const struct option* current = options;
                while (current < options + x) {
                    std::cout << "--" << current->name << "\n";
                    current++;
                }
                while (current->name != 0) {
                    std::cout << "--" << current->name << " value\n";
                    current++;
                }
                return 0;
            }
        }
    }

    const int parameters = 13;
    if (count < parameters && !c_loaded) {
        std::cout << "Warning : some Parameters (" << count << "/" << parameters
                  << ") are not set, using standard values for "
                     "missing arguments...\n";
    }

    c.consistency_check();

    fftw_arr<> _measurement(c.elements, c.elements, c.samples);
    arr_1d<fftw_arr, double> reference(c.reference_samples);

    fftw_arr<> measurement(0, 0, 0);

    reader::read(c.measurement_file, _measurement);
    reader::read(c.reference_file, reference);

    _measurement.sub_to(measurement,
                        0,
                        c.elements,
                        0,
                        c.elements,
                        c.get_roi_start() - c.offset,
                        c.get_roi_length());

    double max, min;
    reference.maxmin(max, min);
    double limit = (max - min) / 100.0;
    arr_1d<fftw_arr, double> trimmed_reference(0, nullptr, false);
    reference.trim(-limit, limit, trimmed_reference);

    const double scale = 100.0;
    assert_that(measurement.scale_to(scale) &&
                  trimmed_reference.scale_to(scale),
                "Reference and/or measurement consists only of zeroes in [" +
                  std::to_string(c.get_roi_start()) + ", " +
                  std::to_string(c.get_roi_end()) +
                  "]!!! Recheck "
                  "your ROI and/or your input files.");
    if (!saft_threshold && saft_resolution) {
        std::cerr << "--saft_resolution specified without --saft : no "
                     "saft-warm-start will be generated"
                  << std::endl;
    }

    if (saft_threshold && saft_threshold < 1) {
        const double resolution = saft_resolution.value_or(
          measurement.dim3 / c.meters_to_tacts(c.wave_length / 2));
        saft(resolution, 2 * resolution, c)
          .populate(
            measurement, *saft_threshold, warm_start_for_slave, std::nullopt);
    }

    roi{ c.get_roi_start(), c.get_roi_end() }.filter_tofs_to(
      warm_start_for_slave, filtered_warm_start_for_slave);

    /// add master_solver
    c.master_solver = master_solver;

    if (!rewrite_with_slave_tofs.empty()) {
        assert_that(rewrite_with_slave_tofs.size() ==
                      rewrite_with_slave_config.size(),
                    "Should have the same size");
        for (unsigned i = 0; i < rewrite_with_slave_tofs.size(); i++) {
            config& conf = rewrite_with_slave_config[i];
            std::vector<time_of_flight>& tofs = rewrite_with_slave_tofs[i];
            std::vector<double>& vals = rewrite_with_slave_vals[i];
            cgdump_analyser aws(conf);
            std::string infeasibility_file = c.output + ".infeasible_";
            unsigned count = 0;
            for (time_of_flight& tof : tofs) {
                std::string current_infeasibility_file =
                  infeasibility_file + std::to_string(count);
                if (!aws.rewrite(tof, current_infeasibility_file)) {
                    std::cerr << "could not write_back a tof, see file "
                              << current_infeasibility_file << std::endl;
                }
                count++;
            }
            std::ofstream os(rewrite_with_slave_config_name[i] +
                             ".corrected.cgdump");
            auto it = container_input_iterator{ tofs };
            conf.save(os, it, vals);
        }
        return 0;
    }

    auto analyse = [](std::unique_ptr<cgdump_analyser>& aws,
                      std::vector<time_of_flight>& tofs) {
        //a) iterate over tofs : for every tof, put in slave, run slave, dump variables
        for (time_of_flight& tof : tofs) {
            if (aws->analyse(tof)) {
                std::cout << "Slave is feasible for current tof." << std::endl;
                std::cout << "x/y-coordinate in tacts:  "
                          << aws->representant_x / 2.0 << " / "
                          << aws->representant_y / 2.0 << " or "
                          << aws->representant_y_alt / 2.0 << std::endl;
                std::cout << "x/y-coordinate in meters: "
                          << aws->c.tacts_to_meter(aws->representant_x / 2.0)
                          << " / "
                          << aws->c.tacts_to_meter(aws->representant_y / 2.0)
                          << " or "
                          << aws->c.tacts_to_meter(aws->representant_y_alt /
                                                   2.0)
                          << std::endl;
                //std::cout << "Binaries:\n" << std::endl;
                //aws.binaries.dump(std::cout);
                std::cout << "Diameters:\n" << std::endl;
                aws->diameters.dump(std::cout);
                std::cout << "Quadratics:\n" << std::endl;
                aws->quadratic.dump(std::cout);
            } else {
                std::cout << "Slave is not feasible for current tof."
                          << std::endl;
            }
        }
        return 0;
    };

    if (analyse_with_slave) {
        auto aws =
          std::make_unique<cgdump_analyser>(analyse_with_slave.value());
        analyse(aws, to_be_analysed);
    }
    if (analyse_xy_with_slave) {
        std::unique_ptr<cgdump_analyser> aws =
          std::make_unique<cgdump_analyser_from_xy>(
            analyse_xy_with_slave.value());
        analyse(aws, to_be_xy_analysed);
    }

    if (analyse_with_slave || analyse_xy_with_slave) {
        return 0;
    }

    //grb_cg cg{ c, measurement, trimmed_reference };
    //grb_cg_wavelength_precision cg(c);
    //grb_cg_multi_slaves cg{
    //    c, measurement, trimmed_reference, slaves, slave_cuts
    //};

    std::unique_ptr<column_generation> cg;
    cg = std::make_unique<grb_cg_multi_slaves>(
      c, measurement, trimmed_reference, slaves, slave_cuts, cb_options);

    roi{ c.get_roi_start(), c.get_roi_end() }.filter_tofs_to(
      master_warm_start,
      master_warm_start_values,
      filtered_master_warm_start,
      filtered_master_warm_start_values);

    warm_start start_values{
        filtered_master_warm_start,
        no_warm_start_values ? std::nullopt
                             : std::optional{ std::reference_wrapper{
                                 filtered_master_warm_start_values } },
        filtered_warm_start_for_slave,
        slow_warm_start,
    };

    std::vector<time_of_flight> reflectors;
    std::vector<double> amplitude;

    cg->run(start_values, reflectors, amplitude);

    return 0;
}