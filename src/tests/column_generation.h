
#include "../optlib/grb_column_generation.h"
#include "../optlib/reader.h"
#include <cxxtest/TestSuite.h>

class column_generation_test : public CxxTest::TestSuite
{
  private:
    void helper(double d[],
                unsigned samples,
                unsigned elements,
                std::vector<time_of_flight>& coords,
                std::vector<double>& results,
                bool verbose,
                double pitch_in_tacts = 7.559)
    {
        fftw_arr<> measurement(elements, elements, samples);
        std::copy(d, d + samples * elements * elements, measurement.data);
        arr_1d<fftw_arr> reference_signal(2);
        reference_signal(0, 0, 0) = 1;
        reference_signal(0, 0, 1) = -1;
        helper(measurement,
               reference_signal,
               samples,
               elements,
               coords,
               results,
               verbose,
               pitch_in_tacts);
    }

    void helper(arr<>& measurement,
                arr<>& reference_signal,
                unsigned samples,
                unsigned elements,
                std::vector<time_of_flight>& coords,
                std::vector<double>& results,
                bool verbose,
                double pitch_in_tacts = 7.559)
    {
        config c;
        c.x_position = 0.0;
        c.sampling_rate = 20e6;
        c.wave_speed = 6350;
        c.pitch = pitch_in_tacts * c.wave_speed / c.sampling_rate;
        c.elements = elements;
        c.wave_length = 6350.0 / 5e6;
        c.samples = samples;
        c.reference_samples = 512;
        c.verbose = verbose;

        grb_cg cg(c, measurement, reference_signal);

        //reference_signal.trim(-10, 10);
        std::vector<time_of_flight> _dummy;
        warm_start w{
            _dummy,
            std::nullopt,
            _dummy,
        };
        cg.run(w, coords, results);
    }

  public:
    //void test_simple_reversed_cg()
    //{
    //    std::vector<time_of_flight> coords;
    //    std::vector<double> results;

    //    unsigned samples = 5;
    //    double d[] = { 0, 0, -2, 2, 0 };
    //    helper(d, samples, 1, coords, results);

    //    //TS_ASSERT_EQUALS(coords.size(), 1);
    //    //TS_ASSERT_EQUALS(results.size(), 1);
    //    for (unsigned i = 0; i < results.size(); i++) {
    //        if (std::abs(results[i]) > 0.001) {
    //            TS_ASSERT_EQUALS(coords[0].dim1, 1);
    //            TS_ASSERT_EQUALS(coords[0].dim2, 1);
    //            TS_ASSERT_EQUALS(coords[0].dim3, 1);
    //            TS_ASSERT_EQUALS(coords[0].at(0, 0), 2);
    //        }
    //    }
    //}
    void test_simple_cg2()
    {
        std::vector<time_of_flight> coords;
        std::vector<double> results;

        unsigned samples = 5;
        double d[] = { 0, 2, -2, 0,  0, 0, 0, 2,  -2, 0,
                       0, 0, 2,  -2, 0, 0, 2, -2, 0,  0 };
        double pitch_in_tacts = 2;
        helper(d, samples, 2, coords, results, false, pitch_in_tacts);
        //std::cout << std::endl;
    }

    void test_simple_cg()
    {
        std::vector<time_of_flight> coords;
        std::vector<double> results;

        unsigned samples = 5;
        double d[] = { 0, 0, 2, -2, 0 };
        helper(d, samples, 1, coords, results, false);

        for (unsigned i = 0; i < results.size(); i++) {
            if (std::abs(results[i]) > 0.001) {
                TS_ASSERT_EQUALS(coords[i].dim1, 1);
                TS_ASSERT_EQUALS(coords[i].dim2, 1);
                TS_ASSERT_EQUALS(coords[i].dim3, 1);
                TS_ASSERT_EQUALS(coords[i].at(0, 0), 2);
            }
        }
    }

    void test_double_cg()
    {
        std::vector<time_of_flight> coords;
        std::vector<double> results;

        unsigned samples = 5;
        double d[] = { 0, 2, 0, -2, 0 };
        helper(d, samples, 1, coords, results, false);

        for (unsigned i = 0; i < coords.size(); i++) {
            if (std::abs(results[i]) > 0.001) {
                TS_ASSERT_EQUALS(coords[i].senders, 1);
                TS_ASSERT_EQUALS(coords[i].receivers, 1);
                TS_ASSERT(coords[i].at(0, 0) == 1 || coords[i].at(0, 0) == 2);
            }
        }
    }

    //void test_maxima()
    //{
    //    grb_cg cg(0.0,     // x_pos
    //              1.2e-3,  //pitch in m
    //              20e6,    //samplingrate
    //              6350,    // speed of sound in alu
    //              4,       //velements
    //              4,       //elements
    //              0.00127, //wavelength = speed/freq
    //              124,
    //              64,
    //              true); //verbose
    //    time_of_flight tof(4, 4);
    //    for (unsigned i = 0; i < tof.senders; i++) {
    //        for (unsigned j = 0; j < tof.receivers; j++) {
    //            tof.at(i, j) = 22;
    //        }
    //    }
    //    std::vector<time_of_flight> tofvec;
    //    tofvec.push_back(std::move(tof));
    //    std::vector<double> amp;
    //    amp.push_back(1.0);

    //    cg.generate_maxima_plot_script(std::cout, tofvec, amp);

    //    // Piping does not work yet :(
    //    //std::stringstream ss;
    //    //cg.generate_maxima_plot_script(ss, tofvec, amp);
    //    //FILE* pipe = popen("maxima", "w");
    //    //fputs(ss.str().c_str(), pipe);
    //    //fclose(pipe);
    //}
};
