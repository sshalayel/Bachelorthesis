
#include "../optlib/coordinates.h"
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <numeric>

class coord_test : public CxxTest::TestSuite
{
  protected:
    void feasibility_helper(std::vector<unsigned>& data,
                            std::string message = "")
    {
        const unsigned elements = data.size();

        TSM_ASSERT(message,
                   coordinate_tools::tof_feasible_in_range(
                     data.begin(), data.end(), elements, 3.78));
    }

  public:
    void test_bounds()
    {
        bounds simple = bounds(1) * bounds(2) + bounds(3) / bounds(4);
        TS_ASSERT_EQUALS(simple.lower, 1.0 * 2.0 + 3.0 / 4.0);
        TS_ASSERT_EQUALS(simple.upper, 1.0 * 2.0 + 3.0 / 4.0);

        simple = bounds(4, 8);
        simple = simple.intersect(bounds(5, 7));
        TS_ASSERT_EQUALS(simple.lower, 5.0);
        TS_ASSERT_EQUALS(simple.upper, 7.0);
        TS_ASSERT(simple.is_inside(simple));
        TS_ASSERT(simple.is_inside(bounds(-10, 10)));

        simple = bounds(3, 4) * bounds(5, 6);
        TS_ASSERT_EQUALS(simple.lower, 15.0);
        TS_ASSERT_EQUALS(simple.upper, 24.0);

        simple = bounds(3, -4) * bounds(5, 6);
        TS_ASSERT_EQUALS(simple.lower, -24.0);
        TS_ASSERT_EQUALS(simple.upper, 18.0);

        TS_ASSERT_THROWS_ANYTHING(bounds(3, -4) / bounds(5, 0));

        simple = bounds(3, -4) + bounds(5, 6);
        TS_ASSERT_EQUALS(simple.lower, 1.0);
        TS_ASSERT_EQUALS(simple.upper, 9.0);

        simple = bounds(3, -4) - bounds(5, 6);
        TS_ASSERT_EQUALS(simple.lower, -10.0);
        TS_ASSERT_EQUALS(simple.upper, -2.0);

        TS_ASSERT_THROWS_ANYTHING(bounds(3, -4).intersect(bounds(4, 6)));

        simple = bounds(3, -4).square();
        TS_ASSERT_EQUALS(simple.lower, 9.0);
        TS_ASSERT_EQUALS(simple.upper, 16.0);

        simple = bounds(16, -25).root();
        TS_ASSERT_EQUALS(simple.lower, 0.0);
        TS_ASSERT_EQUALS(simple.upper, 4.0);

        TS_ASSERT_THROWS_ANYTHING(bounds(-9, -25).root());

        simple = 13 * bounds(16, -25);
        TS_ASSERT_EQUALS(simple.lower, 13 * -25);
        TS_ASSERT_EQUALS(simple.upper, 13 * 16);
    }

    void test_separability2()
    {
        const unsigned offset = 1;
        const unsigned elements = 2;
        time_of_flight tof(elements, elements, {});
        std::iota(tof.begin(), tof.end(), 1);

        arr_1d<arr, double> reference(4);
        std::iota(reference.begin(), reference.end(), 42);

        arr_1d<arr, double> measurement(8);
        std::iota(measurement.begin(), measurement.end(), 66);

        TS_ASSERT_DELTA(
          tof.dot_product_with_dual(reference, measurement, 0), 48740, 0.1);
        TS_ASSERT_DELTA(
          tof.dot_product_with_dual(reference, measurement, offset),
          48044,
          0.1);
    }

    void test_separability()
    {
        const unsigned elements = 4;
        const unsigned measurement_length = 5;
        unsigned data[elements * elements];
        std::iota(data, data + elements * elements, 0);
        time_of_flight tof(4, 4, data, false, {});

        arr_1d<arr, double> reference(2);
        reference.at(0) = 0;
        reference.at(1) = 1;

        arr<> measurement(elements, elements, measurement_length);
        std::iota(measurement.begin(), measurement.end(), 0);

        TS_ASSERT_EQUALS(40,
                         tof.dot_product_with_dual(reference, measurement, 0));

        measurement.realloca(elements, elements, 2 * measurement_length);
        std::iota(measurement.begin(), measurement.end(), 0);

        TS_ASSERT_EQUALS(405,
                         tof.dot_product_with_dual(reference, measurement, 0));
    }

    void test_tikz()
    {
        time_of_flight tof(4, 4, {});
        std::iota(tof.diagonal_begin(), tof.diagonal_end(), 42);
        std::ofstream os("tex/test_output.tex");
        tikz_tof_printer(os, 0.5, 0.5, 1.2e-3, 3e-4, 4)
          .add_tof(tof, 200, 80, 200, true);
        std::ofstream os2("tex/test_output_without.tex");
        tikz_tof_printer(os2, 0.5, 0.5, 1.2e-3, 3e-4, 4)
          .add_tof(tof, 200, 80, 200, false);
    }

    void test_tof_feasibility()
    {
        std::vector<std::vector<unsigned>> datas = {
            //diagonal taken from one cgdump
            {
              621,
              620,
              620,
              620,
              619,
              619,
              619,
              619,
              619,
              619,
              619,
              620,
              620,
              620,
              621,
              622,
            },

            // Taken from one cgdump.
            {
              241,
              240,
              238,
              237,
            },

            //Plotted with geogebra, should be ok
            {
              303,
              301,
              300,
              300,
              299,
              299,
              298,
              297,
              297,
              297,
              297,
            },
        };

        unsigned data_idx = 0;
        for (auto& data : datas) {
            feasibility_helper(data, std::to_string(data_idx));
        }
    }

    void test_strange_tof()
    {
        //TODO: add strange tof here
        // //diagonal taken from one cgdump that looks very suspicious...
        // std::vector<unsigned> data = {
        //     241, 239, 238, 236, 236, 235, 234, 234,
        //     234, 234, 235, 235, 236, 238, 239, 241,
        // };
        // feasibility_helper(
        //   data, "The slave constraints are allowing infeasible tofs.");
    }

    void test_reference()
    {
        int a = 5;
        int b = 7;
        time_of_flight tof(a, b, {});
        TS_ASSERT_EQUALS(tof.senders, tof.dim2);
        TS_ASSERT_EQUALS(tof.receivers, tof.dim3);
        tof.dim2 = 2 * a;
        tof.dim3 = 2 * b;
        TS_ASSERT_EQUALS(tof.senders, tof.dim2);
        TS_ASSERT_EQUALS(tof.receivers, tof.dim3);
    }
    void test_cartesian_string()
    {
        std::string test = "var0_1_";
        cartesian<double> c(test);
        TS_ASSERT_EQUALS(c.x, 0);
        TS_ASSERT_EQUALS(c.y, 1);
        TS_ASSERT_EQUALS((std::string)c, test);
    }

    void test_copy()
    {
        unsigned size = 4;

        time_of_flight tof(size, size, {});
        for (unsigned i = 0; i < tof.senders; i++) {
            for (unsigned j = 0; j < tof.receivers; j++) {
                tof.at(i, j) = i + j * i;
            }
        }
        time_of_flight tof2(tof.senders, tof.receivers, {});
        tof.copy_to(tof2);
        for (unsigned i = 0; i < tof.senders; i++) {
            for (unsigned j = 0; j < tof.receivers; j++) {
                TS_ASSERT_EQUALS(tof.at(i, j), tof2.at(i, j));
            }
        }
    }
};
