#include "../optlib/coordinates.h"
#include "../optlib/randomisation.h"
#include "../optlib/slave_problem.h"
#include <cxxtest/TestSuite.h>
#include <optional>

class randomisation_test : public CxxTest::TestSuite
{
    slave_problem::distance_mapping get_mapping(unsigned offset)
    {
        return slave_problem::distance_mapping{
            [=](unsigned i) { return i + offset; },
            [=](unsigned i) {
                return i >= offset ? std::optional<unsigned>(i - offset)
                                   : std::optional<unsigned>();
            }
        };
    }

  public:
    void test_simple()
    {
        const unsigned elements = 3;
        const unsigned samples = 4;
        const unsigned offset = 100;

        symmetric_arr<proxy_arr<double>> binaries(elements, elements, samples);
        binaries.for_each([](double& d) { d = 0.25; });

        symmetric_arr<proxy_arr<double>> objective_for_binaries(
          elements, elements, samples);
        objective_for_binaries.for_ijk([](unsigned i, unsigned j, unsigned k) {
            return ((double)i + k) - j;
        });

        slave_problem::distance_mapping mapping = get_mapping(offset);
        double element_pitch_in_tacts = 1;
        randomisation r(
          binaries, objective_for_binaries, mapping, element_pitch_in_tacts);

        time_of_flight tof(elements, elements, {});
        r.randomise(tof);
    }
};
