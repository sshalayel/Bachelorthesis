#ifndef GRB_COLUMN_GENERATION_H
#define GRB_COLUMN_GENERATION_H

#include "column_generation.h"
#include "column_generation_run.h"
#include "config.h"
#include "coordinates.h"
#include "fftw_arr.h"
#include "fftw_convolution.h"
#include "grb_master.h"
#include "grb_multiple_slave_async.h"
#include "printer.h"
#include "slave_problem.h"
#include "writer.h"
#include <fstream>
#include <iomanip>
#include <optional>
#include <random>
#include <vector>

/// Column Generation using fftw and gurobi.
class grb_cg : public column_generation
{
  public:
    grb_cg(config c, arr<>& measurement, arr<>& reference_signal);
    virtual ~grb_cg() override;
    void run(warm_start warm_start,
             std::vector<time_of_flight>& reflectors_out,
             std::vector<double>& amplitude_out) override;

  protected:
    GRBEnv* e;
};

/// Column Generation using fftw and gurobi and multiples slaves.
class grb_cg_multi_slaves : public column_generation
{
  public:
    grb_cg_multi_slaves(config c,
                        arr<>& measurement,
                        arr<>& reference_signal,
                        unsigned slave_count,
                        slave_cut_options slave_cuts,
                        slave_callback_options options);

    virtual ~grb_cg_multi_slaves() override;

    GRBEnv e;
};

/// @brief A less precise columngeneration.
///
/// It compresses the reference and the measurement such that one discrete steps has ~length of one wavelength.
/// Actually, being sampleprecise may be too precise because the resolution is defined by the wavelength.
class grb_cg_wavelength_precision : public column_generation
{
  public:
    grb_cg_wavelength_precision(config c,
                                arr<>& measurement,
                                arr<>& reference_signal);
    virtual ~grb_cg_wavelength_precision() override;

    unsigned tacts_pro_wavelength;

    void run(warm_start warm,
             std::vector<time_of_flight>& reflectors_out,
             std::vector<double>& amplitude_out) override;

    void resize(const arr<>& a, arr<>& out);

  private:
    GRBEnv* e;
};
#endif
