#include "cgdump_analyser.h"

cgdump_analyser::cgdump_analyser(config c)
  : c(c)
  , binaries(c.elements, c.elements, c.samples)
  , diameters(c.elements, c.elements)
  , quadratic(c.elements)
  , measurement(c.elements, c.elements, c.samples)
{
    measurement.for_each([](double& d) { d = 0.0; });
}

bool
cgdump_analyser::rewrite(time_of_flight& tof, std::string infeasibility_file)
{
    return analyse_and_rewrite(tof, infeasibility_file, true);
}

bool
cgdump_analyser::analyse(time_of_flight& tof, std::string infeasibility_file)
{
    return analyse_and_rewrite(tof, infeasibility_file, false);
}

void
cgdump_analyser::hardcode(time_of_flight& tof, grb_slave& slave)
{
    for (unsigned i = 0; i < tof.senders; i++) {
        for (unsigned j = 0; j < tof.senders; j++) {
            slave.model.addConstr(
              slave.vars_proxy(i, j, tof(i, j) - c.offset) == 1);
        }
    }
}

bool
cgdump_analyser::analyse_and_rewrite(time_of_flight& tof,
                                     std::string infeasibility_file,
                                     bool write_back)
{
    try {
        grb_slave slave{
            &e,           c.element_pitch_in_tacts(),
            std::nullopt, c.offset,
            std::nullopt, std::nullopt,
            std::cout,    grb_no_op_callback(),
            true,
        };

        slave.prepare_for_solving(measurement);
        this->hardcode(tof, slave);

        slave.model.optimize();

        /// test for infeasibility (or feasibility).
        if (slave.assert_feasibility(slave.model)) {
            /// Get the value of the variables.
            representant_x = slave.representant_x.get(GRB_DoubleAttr_X);
            binaries.for_ijk([&](unsigned i, unsigned j, unsigned k) {
                return slave.vars_proxy(i, j, k).get(GRB_DoubleAttr_X);
            });
            diameters.for_ijk([&](unsigned i, unsigned j, unsigned k) {
                return slave.diameter_vars_proxy(i, j, k).get(GRB_DoubleAttr_X);
            });

            quadratic.for_ijk([&](unsigned i, unsigned j, unsigned k) {
                return slave.squared_vars_proxy(i, j, k).get(GRB_DoubleAttr_X);
            });

            representant_y = std::sqrt(std::pow(diameters(0, 0), 2) -
                                       std::pow(representant_x, 2));
            representant_y_alt =
              std::sqrt(quadratic(0) - std::pow(representant_x, 2));

            if (write_back) {
                slave.fill_tof(std::ref(binaries),
                               std::ref(diameters),
                               std::ref(quadratic),
                               representant_x,
                               tof,
                               false);
            }
            return true;
        } else {
            slave.model.write(infeasibility_file + ".lp");
            slave.model.write(infeasibility_file + ".ilp");
            return false;
        }
    } catch (GRBException& e) {
        std::cerr << "Error " << e.getErrorCode() << ": " << e.getMessage()
                  << std::endl;
        return false;
    }
}

cgdump_analyser_from_xy::cgdump_analyser_from_xy(config c)
  : cgdump_analyser(c)
{}

void
cgdump_analyser_from_xy::hardcode(time_of_flight& tof, grb_slave& slave)
{
    assert_that(tof.extension, "This cgdump analyser needs an extended ToF!");

    slave.model.addConstr(slave.representant_x == *tof.representant_x);
    slave.model.addConstr(slave.squared_vars_proxy(0) ==
                          std::pow(*tof.representant_x, 2) +
                            std::pow(tof.extension->y, 2));
}
