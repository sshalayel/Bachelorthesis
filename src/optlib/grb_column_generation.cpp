#include "grb_column_generation.h"

grb_cg::grb_cg(config c, arr<>& measurement, arr<>& reference_signal)
  : column_generation(c, measurement, reference_signal)
{
    e = new GRBEnv();
    master = new grb_master(e,
                            c.verbose,
                            output,
                            (master_problem::solver)c.master_solver,
                            c.master_solution_threshold.value_or(0.0));
    conv = new fourier_convolution();
    slave = new grb_slave(e,
                          c.pitch * c.sampling_rate / c.wave_speed,
                          c.slavestop,
                          c.get_roi_start(),
                          c.horizontal_roi_start,
                          c.horizontal_roi_end,
                          output,
                          new grb_to_file(output),
                          c.verbose);
    instance =
      new column_generation_run<fftw_arr>(measurement, reference_signal, c);
}

void
grb_cg::run(warm_start warm_start,
            std::vector<time_of_flight>& reflectors_out,
            std::vector<double>& amplitude_out)
{
    try {
        column_generation::run(warm_start, reflectors_out, amplitude_out);
    } catch (GRBException& e) {
        std::cout << "Error :" << e.getErrorCode() << "\n";
        std::cout << e.getMessage() << std::endl;
        abort();
    }
}

grb_cg::~grb_cg()
{
    delete master;
    delete slave;
    delete e;
    delete conv;
}

grb_cg_multi_slaves::grb_cg_multi_slaves(config c,
                                         arr<>& measurement,
                                         arr<>& reference_signal,
                                         unsigned slave_count,
                                         slave_cut_options slave_cuts,
                                         slave_callback_options options)
  : column_generation(c, measurement, reference_signal)
{
    //master = new slow_grb_master(e, c.verbose, output, c.master_solution_threshold);
    master = new grb_master(&e,
                            c.verbose,
                            output,
                            (master_problem::solver)c.master_solver,
                            c.master_solution_threshold.value_or(0.0));
    conv = new fourier_convolution();
    // _instance needed to get the constraint pool before casting to interface
    auto _instance = new column_generation_run_async<fftw_arr>(
      measurement, reference_signal, c);
    slave = new grb_multiple_slave_async{
        c.pitch * c.sampling_rate / c.wave_speed,
        c.slave_threshold,
        c.slavestop,
        c.get_roi_start(),
        c.horizontal_roi_start,
        c.horizontal_roi_end,
        output,
        c.verbose,
        slave_count,
        _instance->pool,
        _instance->can_print,
        slave_cuts,
        c.output,
        options,
    };
    instance = _instance;
}

grb_cg_multi_slaves::~grb_cg_multi_slaves() {}

grb_cg_wavelength_precision::grb_cg_wavelength_precision(
  config c,
  arr<>& measurement,
  arr<>& reference_signal)
  : column_generation(c,
                      *new arr<>(0, 0, 0, nullptr),
                      *new arr<>(0, 0, 0, nullptr))
  , tacts_pro_wavelength(c.sampling_rate * c.wave_length / c.wave_speed)
{
    e = new GRBEnv();
    master = new grb_master(e,
                            c.verbose,
                            output,
                            (master_problem::solver)c.master_solver,
                            c.master_solution_threshold.value_or(0.0));
    conv = new fourier_convolution();
    slave = new grb_slave(e,
                          c.pitch / c.wave_length,
                          c.slavestop,
                          c.get_roi_start(),
                          c.horizontal_roi_start,
                          c.horizontal_roi_end,
                          output,
                          new grb_to_file(output),
                          c.verbose);
    if (!c.verbose) {
        print = new muted_printer();
        write = new muted_writer<double>();
    } else {
        print = new simple_printer(output);
        write = new binary_writer<double>();
    }

    resize(measurement, this->measurement);
    resize(reference_signal, this->reference_signal);
}

grb_cg_wavelength_precision::~grb_cg_wavelength_precision()
{
    delete master;
    delete slave;
    delete e;
    delete conv;
    delete print;
    if (!c.output.empty()) {
        delete &output;
    }
}

void
grb_cg_wavelength_precision::resize(const arr<>& in, arr<>& out)
{

    out.realloca(in.dim1,
                 in.dim2,
                 (unsigned)std::ceil((double)in.dim3 / tacts_pro_wavelength));

    out.for_ijk([&](unsigned i, unsigned j, unsigned k) {
        double max = in.at(i, j, tacts_pro_wavelength * k);
        for (unsigned a = tacts_pro_wavelength * k + 1;
             a < std::min(tacts_pro_wavelength * (k + 1), in.dim3);
             a++) {
            max = std::max(max, in.at(i, j, a));
        }
        return max;
    });
}

void
grb_cg_wavelength_precision::run(warm_start warm,
                                 std::vector<time_of_flight>& reflectors_out,
                                 std::vector<double>& amplitude_out)
{
    column_generation::run(warm, reflectors_out, amplitude_out);

    for (time_of_flight& tof : reflectors_out) {
        std::for_each(tof.begin(), tof.end(), [=](unsigned& n) {
            n *= tacts_pro_wavelength;
        });
    }
}
