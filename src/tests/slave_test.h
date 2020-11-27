#include <cxxtest/TestSuite.h>
#include <gurobi_c++.h>

#include "../optlib/coordinates.h"
#include "../optlib/fftw_convolution.h"
#include "../optlib/grb_slave.h"
#include "../optlib/linear_expression.h"
#include "../optlib/reader.h"
#include "../optlib/simplexoid.h"
#include "../optlib/slave_constraints_generator.h"
#include <numeric>
#include <variant>

struct check_linear_expression
{
    time_of_flight& tof;
    double x;
    std::function<unsigned(unsigned)> to_distance;
    bool operator()(linear_expression_constraint e);
    double operator()(linear_expression e);
    double operator()(binary_linear_expression e);
    double operator()(variable e);
    double operator()(binary_sum bs);
    double operator()(constant e);
};

bool
check_linear_expression::operator()(linear_expression_constraint e)
{
    switch (e.op) {
        case linear_expression_constraint::EQUAL:
            return operator()(e.lhs) == operator()(e.rhs);
        case linear_expression_constraint::GREATER_EQUAL:
            return operator()(e.lhs) >= operator()(e.rhs);
        case linear_expression_constraint::LESS_EQUAL:
            return operator()(e.lhs) <= operator()(e.rhs);
        default:
            assert(false);
    }
}

double
check_linear_expression::operator()(linear_expression e)
{
    return e->coefficient * std::visit(*this, e->v);
}

double
check_linear_expression::operator()(binary_sum bs)
{
    switch (bs.t) {
        case binary_sum::SUM:
            return 1.0;
        case binary_sum::DISTANCE:
            return tof.at(bs.i, bs.j);
        case binary_sum::SQUARED_DISTANCE:
            return std::pow(tof.at(bs.i, bs.j), 2);
        default:
            assert(false);
    }
}

double
check_linear_expression::operator()(binary_linear_expression e)
{
    switch (e.op) {
        case binary_linear_expression::ADD:
            return operator()(e.lhs) + operator()(e.rhs);
        case binary_linear_expression::SUB:
            return operator()(e.lhs) - operator()(e.rhs);
        default:
            assert(false);
    }
}
double
check_linear_expression::operator()(variable e)
{
    switch (e.t) {
        case variable::QUADRATIC:
            return std::pow(tof(e.i, e.i), 2);
        case variable::DIAMETER:
            return tof(e.i, e.j);
        case variable::BINARY:
            return tof(e.i, e.j) == to_distance(e.k) ? 1.0 : 0.0;
        case variable::X_REPRESENTANT:
            return x;
        default:
            assert(false);
    }
}

double
check_linear_expression::operator()(constant e)
{
    return 1.0;
}

class slave_test : public CxxTest::TestSuite
{
  private:
    double cosinus_gamma(double a, double b, double c)
    {
        double r = (a * a + b * b - c * c) / (2.0 * a * b);
        return r;
    }
    double length_of_c(double a, double b, double cosinus_gamma)
    {
        return std::sqrt(a * a + b * b - 2.0 * a * b * cosinus_gamma);
    }

    double helper(arr<>& signal,
                  arr<>& ref,
                  columns& results,
                  double& obj,
                  bool verbose = false,
                  unsigned offset = 0,
                  bool dump = false)
    {
        GRBEnv e;
        grb_slave s(&e,
                    3,
                    std::nullopt,
                    offset,
                    std::nullopt,
                    std::nullopt,
                    std::cout,
                    verbose ? new grb_to_file(std::cout) : grb_no_op_callback(),
                    verbose);

        fourier_convolution fc;
        arr<> _convoluted(signal.dim1, signal.dim2, signal.dim3 + ref.dim3 - 1);
        arr_1d<fftw_arr, double> inverted_ref(0, nullptr, false);
        ref.invert(inverted_ref);
        fc.convolve(signal, inverted_ref, _convoluted);

        arr<> convoluted(signal.dim1, signal.dim2, signal.dim3);
        _convoluted.sub_to(convoluted, 0, 0, ref.dim3 - 1);

        if (verbose) {
            std::cout << "Slave_test : " << convoluted << std::endl;
        }

        s.run(convoluted, results);
        if (dump) {
            s.model.write("empty.lp");
        }
        for (auto& r : results) {
            if (verbose) {
                std::cout << r.tof << std::endl;
            }
            TS_ASSERT_DELTA(r.tof.dot_product_with_dual(ref, signal, offset),
                            r.stats.objective,
                            0.1);
        }

        return s.element_pitch_in_tacts;
    }

    void helper_shifter(arr<>& reference,
                        arr<unsigned>& shifts_per_sender_receiver_pair,
                        arr<>& out,
                        bool verbose = false)
    {
        for (unsigned i = 0; i < out.dim1; i++) {
            for (unsigned j = 0; j < out.dim2; j++) {
                unsigned shift = shifts_per_sender_receiver_pair(0, i, j);
                assert(shift < out.dim3 && "uncorrect shift!");

                // before the shifted reference
                for (unsigned k = 0; k < shift; k++) {
                    out(i, j, k) = 0;
                }
                if (reference.dim3 - shift < out.dim3) {
                    // the shifted reference
                    for (unsigned k = shift; k < reference.dim3 + shift; k++) {
                        out(i, j, k) = reference(0, 0, k - shift);
                    }
                    // behind the shifted reference
                    for (unsigned k = shift + reference.dim3; k < out.dim3;
                         k++) {
                        out(i, j, k) = 0.0;
                    }
                } else {
                    // the shifted reference until results is full
                    for (unsigned k = shift; k < out.dim3; k++) {
                        out(i, j, k) = reference(0, 0, k - shift);
                    }
                }
            }
        }
        if (verbose) {
            //            out.dump(std::cout);
        }
    }

    void feasible(time_of_flight& tof,
                  std::function<unsigned(unsigned)> to_distance,
                  bool should_be_feasible,
                  std::vector<linear_expression_constraint>& constraints,
                  double pitch)
    {
        if (!tof.representant_x) {
            TS_SKIP(
              "Skipping feasibility test as tof has no x-representant-value");
        }
        simplexoid s{ 2.0 * pitch, 1.0 };
        double x, y;
        s.tof = &tof;
        s.representant_to_coord({ 0, true, tof.senders - 1, true }, x, y);

        check_linear_expression check{ tof, x, to_distance };
        std::stringstream ss;
        dump_linear_expression dump{
            ss, std::optional{ dump_values{ tof, to_distance } }
        };
        for (linear_expression_constraint& c : constraints) {
            ss.str({});
            ss << std::endl << "x=" << x << " in ";
            dump(c);
            TSM_ASSERT(ss.str().c_str(), check(c));
        }
    }

  public:
    void test_constraints_on_tofs()
    {
        //generate constraints
        std::vector<linear_expression_constraint> constraints;
        const unsigned elements = 16;
        const unsigned offset = 280;
        size f = { elements, elements, 300 };
        const double pitch = 3.78;
        auto&& to_distance = [](unsigned i) { return i + offset; };
        auto&& to_index = [](unsigned i) {
            return i >= offset ? std::optional(i - offset) : std::nullopt;
        };
        slave_constraint_generator scg{
            f,
            { to_distance, to_index },
            pitch,
        };
        scg.all([&](linear_expression_constraint lec) {
            constraints.push_back(std::move(lec));
        });

        //generate evil tofs
        unsigned evil[] = {
            303, 302, 301, 301, 300, 300, 299, 299, 299, 298, 298, 297, 297,
            297, 296, 296, 302, 301, 300, 300, 299, 299, 299, 298, 298, 297,
            297, 296, 296, 296, 295, 295, 301, 300, 300, 299, 299, 298, 298,
            298, 297, 296, 296, 296, 295, 296, 295, 295, 301, 300, 299, 299,
            298, 298, 298, 297, 297, 296, 296, 295, 295, 295, 295, 294, 300,
            299, 299, 298, 298, 297, 297, 297, 296, 296, 296, 295, 294, 294,
            294, 294, 300, 299, 298, 298, 297, 297, 297, 296, 296, 295, 295,
            294, 294, 294, 293, 293, 299, 299, 298, 298, 297, 297, 296, 295,
            295, 294, 294, 294, 293, 293, 293, 293, 299, 298, 298, 297, 297,
            296, 295, 295, 294, 294, 294, 294, 293, 293, 293, 293, 299, 298,
            297, 297, 296, 296, 295, 294, 294, 294, 293, 293, 292, 292, 292,
            292, 298, 297, 296, 296, 296, 295, 294, 294, 294, 293, 293, 292,
            292, 292, 292, 291, 298, 297, 296, 296, 296, 295, 294, 294, 293,
            293, 293, 293, 292, 292, 292, 291, 297, 296, 296, 295, 295, 294,
            294, 294, 293, 292, 293, 292, 291, 292, 291, 291, 297, 296, 295,
            295, 294, 294, 293, 293, 292, 292, 292, 291, 291, 291, 291, 291,
            297, 296, 296, 295, 294, 294, 293, 293, 292, 292, 292, 292, 291,
            291, 291, 290, 296, 295, 295, 295, 294, 293, 293, 293, 292, 292,
            292, 291, 291, 291, 290, 290, 296, 295, 295, 294, 294, 293, 293,
            293, 292, 291, 291, 291, 291, 290, 290, 290
        };
        arr_2d _evil(elements, elements, evil);
        time_of_flight from_evil(elements, elements, {});
        //copy from normal arr to symmetric arr
        from_evil.for_ijk(std::ref(_evil));

        feasible(from_evil, to_distance, false, constraints, pitch);
    }

    void test_constraint_pool_simple()
    {
        const unsigned elements = 16;
        double obj = 123;
        time_of_flight tof(elements, elements, {});
        time_of_flight tof_ref(elements, elements, {});
        std::iota(tof.begin(), tof.end(), 42);
        tof.copy_to(tof_ref);

        constraint_pool p;

        const int my_slave_id = 42;
        p.add({ std::move(tof), { obj, 0, 0, 0, 0, 0 } }, my_slave_id);

        columns c_res;

        p.consume(c_res, [](column_with_origin& c) { return true; });

        time_of_flight& res = c_res[0].tof;

        TS_ASSERT_EQUALS(c_res.size(), 1);
        TS_ASSERT_EQUALS(res.size(), tof_ref.size());

        TS_ASSERT_SAME_DATA(tof_ref.data, res.data, tof_ref.size());
    }

    //void test_strange_signs()
    //{
    //    double d[] = { 0, 2, -2, 0,  0, 0, 0,  -1, 1, 0,
    //                   0, 0, 4,  -4, 0, 0, -3, 3,  0 , 0};
    //    const unsigned elements = 2;
    //    const unsigned samples = 5;
    //    fftw_arr<> signal(elements, elements, samples);
    //    std::copy(d, d + elements * elements * samples, signal.data);
    //    for (unsigned i = 0; i < signal.size(); i++) {
    //        std::cerr << signal.data[i] << ", ";
    //        std::cerr.flush();
    //    }

    //    fftw_arr_1d<> ref(2);
    //    ref(0, 0, 0) = 1.0;
    //    ref(0, 0, 0) = -1.0;

    //    std::vector<time_of_flight> results(elements, elements);
    //    double obj;

    //    std::cout << "STRANGE_SIGNS" << std::endl;
    //    helper(signal, ref, results, obj, true);
    //    results.dump(std::cout);
    //    std::cout << "STRANGE_SIGNS" << std::endl;
    //}

    void test_shifted_reference()
    {
        civa_txt_reader r;
        unsigned length = 512;
        unsigned offset = 500;
        unsigned shifted_reference_length = 2 * length;
        arr_1d<fftw_arr> reference_signal(length);
        TS_ASSERT(r.run("./data/simulated_references/"
                        "gauss_2Mhz_1percent_0degree_signal.csv",
                        reference_signal));

        // unsigned shift = 135;
        unsigned shift = 270;
        arr_1d<fftw_arr> shifted_reference(shifted_reference_length);
        std::fill(shifted_reference.begin(), shifted_reference.end(), 0.0);
        for (unsigned i = shift;
             i < std::min(shifted_reference_length, length + shift);
             i++) {
            shifted_reference(0, 0, i) = reference_signal(0, 0, i - shift);
        }

        columns results;
        double obj;
        helper(
          shifted_reference, reference_signal, results, obj, false, 0, false);

        TS_ASSERT_EQUALS(results.size(), 1);
        TS_ASSERT_EQUALS(results[0].tof.size(), 1);
        TS_ASSERT_DELTA(results[0].tof.at(0, 0), shift, 0.001);

        results.clear();
        helper(shifted_reference,
               reference_signal,
               results,
               obj,
               false,
               offset,
               false);

        TS_ASSERT_EQUALS(results.size(), 1);
        TS_ASSERT_EQUALS(results[0].tof.size(), 1);
        TS_ASSERT_DELTA(results[0].tof.at(0, 0), shift + offset, 0.001);
    }

    void test_shifted_reference2()
    {
        civa_txt_reader r;
        unsigned length = 512;
        unsigned offset = 500;
        unsigned shifted_reference_length = 2 * length;
        arr_1d<fftw_arr> reference_signal(length);
        TS_ASSERT(r.run("./data/simulated_references/"
                        "gauss_2Mhz_1percent_0degree_signal.csv",
                        reference_signal));

        // unsigned shift = 135;
        unsigned shift = 270.3;
        arr_1d<fftw_arr> shifted_reference(shifted_reference_length);
        std::fill(shifted_reference.begin(), shifted_reference.end(), 0.0);
        for (unsigned i = shift;
             i < std::min(shifted_reference_length, length + shift);
             i++) {
            shifted_reference(0, 0, i) = reference_signal(0, 0, i - shift);
        }

        columns results;
        double obj;
        helper(
          shifted_reference, reference_signal, results, obj, false, 0, false);

        TS_ASSERT_EQUALS(results.size(), 1);
        TS_ASSERT_EQUALS(results[0].tof.size(), 1);
        TS_ASSERT_DELTA(results[0].tof.at(0, 0), shift, 0.001);

        results.clear();
        helper(shifted_reference,
               reference_signal,
               results,
               obj,
               false,
               offset,
               false);

        TS_ASSERT_EQUALS(results.size(), 1);
        TS_ASSERT_EQUALS(results[0].tof.size(), 1);
        TS_ASSERT_DELTA(results[0].tof.at(0, 0), shift + offset, 0.001);
    }

    void test_shifted_reference_multiple_sender_receiver_and_check_cosinerule()
    {
        civa_txt_reader r;
        unsigned reference_length = 512;
        arr_1d<fftw_arr> _reference_signal_1d(reference_length);
        arr_1d<fftw_arr> reference_signal_1d(0, nullptr, false);
        TS_ASSERT(r.run("./data/simulated_references/"
                        "gauss_2Mhz_50percent_0degree_signal.csv",
                        _reference_signal_1d));

        _reference_signal_1d.trim(-10, 10, reference_signal_1d);

        unsigned length = 3 * reference_signal_1d.dim3;

        // TODO: this test lasts too long
        unsigned elements = 2;
        arr<unsigned> shifts(1, elements, elements);
        unsigned shift_values[] = { 20, 33, 46, 29, 32, 22, 31, 93,
                                    78, 52, 89, 45, 0,  28, 49, 60 };

        // Generate a symmetric shift matrix
        unsigned current_shift_value_idx = 0;
        for (unsigned i = 0; i < elements; i++) {
            for (unsigned j = 0; j <= i; j++) {
                shifts(0, i, j) = shift_values[current_shift_value_idx];
                shifts(0, j, i) = shift_values[current_shift_value_idx++];
            }
        }
        //shifts.dump(std::cout);

        fftw_arr<> shifted_signal(elements, elements, length);
        helper_shifter(reference_signal_1d, shifts, shifted_signal);
        columns results_v;
        double obj;
        double pitch = helper(
          shifted_signal, reference_signal_1d, results_v, obj, false, 0, false);

        time_of_flight& results = results_v[0].tof;

        //std::cerr << results << std::endl;

        //check for cosinerule
        for (unsigned i = 0; i < results.senders; i++) {
            for (unsigned j = 0; j < results.senders; j++) {
                if (i != j) {
                    for (unsigned k = 0; k < results.senders; k++) {
                        if (k != j && k != i) {
                            double cos_angle_ij =
                              cosinus_gamma(results.at(i, i) / 2.0,
                                            std::fabs((double)i - j) * pitch,
                                            results.at(j, j) / 2.0);
                            TSM_ASSERT_LESS_THAN_EQUALS(
                              "Invalid triangle " + std::to_string(i) + "(" +
                                std::to_string(results.at(i, i)) + "), " +
                                std::to_string(j) + "(" +
                                std::to_string(results.at(j, j)) + ") and (" +
                                std::to_string(((double)i - j) * pitch) + ")!",
                              -1.0,
                              cos_angle_ij);
                            TSM_ASSERT_LESS_THAN_EQUALS(
                              "Invalid triangle " + std::to_string(i) + "(" +
                                std::to_string(results.at(i, i)) + "), " +
                                std::to_string(j) + "(" +
                                std::to_string(results.at(j, j)) + ") and (" +
                                std::to_string(((double)i - j) * pitch) + ")!",

                              cos_angle_ij,
                              1.0);
                            double theoretical_length_kk =
                              length_of_c(results.at(i, i) / 2.0,
                                          std::fabs((double)i - k) * pitch,
                                          cos_angle_ij);
                            TSM_ASSERT_LESS_THAN_EQUALS(
                              "Cosinerule not satisfied for " +
                                std::to_string(i) + ", " + std::to_string(j) +
                                ", " + std::to_string(k) +
                                ", angle=" + std::to_string(cos_angle_ij) + "!",
                              results.at(k, k) / 2.0,
                              theoretical_length_kk);
                            TSM_ASSERT_LESS_THAN_EQUALS(
                              "Cosinerule not satisfied for " +
                                std::to_string(i) + ", " + std::to_string(j) +
                                ", " + std::to_string(k) +
                                ", angle=" + std::to_string(cos_angle_ij) + "!",
                              theoretical_length_kk,
                              results.at(k, k) / 2.0 + 1.0);
                        }
                    }
                }
            }
        }
    }
};
