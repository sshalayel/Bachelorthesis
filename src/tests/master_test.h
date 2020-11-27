#include <cxxtest/TestSuite.h>
#include <numeric>

#include "../optlib/coordinates.h"
#include "../optlib/grb_master.h"
#include "../optlib/reader.h"
#include "gurobi_c++.h"
#include <optional>

class master_test : public CxxTest::TestSuite
{
  public:
    void test_master_small_data()
    {
        GRBEnv e;
        grb_master m(&e, false, std::cout, master_problem::SIMPLEX, 0.0);

        const unsigned elements = 2;
        const unsigned measurement_length = 5;
        const unsigned ref_length = 2;
        arr<> measurement(elements, elements, measurement_length);
        arr_1d<arr, double> reference_signal(ref_length);

        ///controversial feature
        const unsigned offset = 2;

        reference_signal.for_ijk(
          [&](unsigned i, unsigned j, unsigned k) { return k + 1; });

        std::iota(measurement.begin(), measurement.end(), 0);

        arr<> _dual(measurement.dim1, measurement.dim2, measurement.dim3);
        dual_solution dual{ _dual, { 0, 0, 0 } };
        double obj;

        m.solve_reduced_problem(
          elements, offset, measurement, reference_signal, {}, dual, obj);

        time_of_flight add_me(elements, elements, {});
        std::fill(add_me.begin(), add_me.end(), offset);

        const char* before_add = "/tmp/before_add.lp";
        const char* after_add = "/tmp/after_add.lp";

        m.model->write(before_add);

        m.add_variable(add_me, reference_signal, std::nullopt);

        m.model->update();
        m.model->write(after_add);

        // checks if both files only differ in the right lines (where the variable was added)
        {
            std::ifstream before_str(before_add);
            std::ifstream after_str(after_add);

            std::string current_a, current_b;
            while (std::getline(before_str, current_b) &&
                   std::getline(after_str, current_a)) {
                if (current_a == current_b) {
                    continue;
                }

                unsigned start_a = current_a.find("R");
                unsigned start_b = current_b.find("R");
                unsigned end_a = current_a.find(":", start_a);
                unsigned end_b = current_b.find(":", start_b);

                unsigned constr_a =
                  std::stoul(current_a.substr(start_a + 1, end_a));
                unsigned constr_b =
                  std::stoul(current_b.substr(start_b + 1, end_b));

                TS_ASSERT_EQUALS(constr_a, constr_b);
                if (constr_a % 5 > 1) {
                    TS_ASSERT_EQUALS(current_a, current_b);
                } else {
                    TS_ASSERT_DIFFERS(current_a, current_b);
                }
            }
        }

        arr<> _after_add_dual(
          measurement.dim1, measurement.dim2, measurement.dim3);
        dual_solution after_add_dual{ _after_add_dual, { 0, 0, 0 } };
        double after_add_obj;

        m.solve_reduced_problem(elements,
                                offset,
                                measurement,
                                reference_signal,
                                {},
                                after_add_dual,
                                after_add_obj);

        TS_ASSERT(!std::equal(dual.values.begin(),
                              dual.values.end(),
                              after_add_dual.values.begin()));
    }

    void test_simple_master_with_offset()
    {
        GRBEnv e;
        grb_master m(&e, false, std::cout, master_problem::SIMPLEX, 0.0);
        binary_reader<double> br;
        civa_txt_reader cr;

        unsigned elements = 4;
        unsigned measurement_start = 2700;
        unsigned measurement_length = 30;
        unsigned offset = 500;

        arr<> _measurement(16, 16, 3518);
        arr<> measurement(4, 4, measurement_length);
        TS_ASSERT(
          br.run("./data/simulated/9.04.19/model_Dvoie.bin", _measurement));
        _measurement.sub_to(measurement, 0, 0, measurement_start);
        arr_1d<> _reference_signal(512);
        arr_1d<> reference_signal(0, nullptr, false);
        TS_ASSERT(cr.run("./data/simulated/9.04.19/reference_signal.csv",
                         _reference_signal));

        _reference_signal.trim(-10, 10, reference_signal);

        arr<> _dual(measurement.dim1, measurement.dim2, measurement.dim3);
        dual_solution dual{ _dual, { 0, 0, 0 } };
        double obj;
        m.solve_reduced_problem(
          elements, offset, measurement, reference_signal, {}, dual, obj);

        std::vector<double> data_before;
        arr<> slack(elements, elements, measurement_length);
        arr<> neg_slack(elements, elements, measurement_length);
        m.get_primal(data_before, slack, neg_slack);

        // The dual consists of ones and -ones, because there are only the slack variables in the primal.
        for (unsigned i = 0; i < dual.values.dim1; i++) {
            for (unsigned j = 0; j < dual.values.dim2; j++) {
                for (unsigned k = 0; k < dual.values.dim3; k++) {
                    TS_ASSERT_EQUALS(std::abs(dual.values(i, j, k)), 1);
                }
            }
        }

        time_of_flight test(elements, elements, {});
        for (unsigned i = 0; i < elements; i++) {
            for (unsigned j = 0; j < elements; j++) {
                test.at(i, j) = offset + 10 + i + j;
            }
        }

        m.add_variable(test, reference_signal, std::nullopt);

        arr<> _dual2(measurement.dim1, measurement.dim2, measurement.dim3);
        dual_solution dual2{ _dual2, { 0, 0, 0 } };
        double obj2;
        m.solve_reduced_problem(
          elements, offset, measurement, reference_signal, {}, dual2, obj2);

        std::vector<double> data;
        m.get_primal(data, slack, neg_slack);
    }
};