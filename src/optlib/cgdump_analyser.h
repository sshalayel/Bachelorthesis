#ifndef ANALYSE_WITH_SLAVE_H
#define ANALYSE_WITH_SLAVE_H

#include "config.h"
#include "coordinates.h"
#include "grb_slave.h"
#include "gurobi_c++.h"

/// Contains the logic for rerunning a slave on a cgdump to check for feasibility.
struct cgdump_analyser
{
    config c;

    double representant_y;
    double representant_y_alt;
    /// Value of x-coordinate.
    double representant_x;
    /// Values of the slave.
    arr<double> binaries;
    /// Values of the slave.
    arr_2d<arr, double> diameters;
    /// Values of the slave.
    arr_1d<arr, double> quadratic;

    ///Fake empty measurement as we do not care about the objective.
    arr<double> measurement;

    GRBEnv e;

    cgdump_analyser(config c);

    /// Fills binaries, diameters and quadratic on true, else the slave is infeasible. In this latter case, an ilp is written to infeasibility_file.
    bool analyse(time_of_flight& tof,
                 std::string infeasibility_file = "infeasible");

    /// Like analyse but writes the results back in tof.
    bool rewrite(time_of_flight& tof,
                 std::string infeasibility_file = "infeasible");

  protected:
    bool analyse_and_rewrite(time_of_flight& tof,
                             std::string infeasibility_file,
                             bool write_back);
    /// Hardcode the solutions into the slave.
    virtual void hardcode(time_of_flight& tof, grb_slave& slave);
};

struct cgdump_analyser_from_xy : cgdump_analyser
{
    cgdump_analyser_from_xy(config c);

  protected:
    void hardcode(time_of_flight& tof, grb_slave& slave) override;
};

#endif
