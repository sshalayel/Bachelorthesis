#include "../optlib/arr.h"
#include "../optlib/saft.h"
#include "../optlib/visualizer.h"
#include <cxxtest/TestSuite.h>

class saft_test : public CxxTest::TestSuite
{
  public:
    void test_simple_saft()
    {
        double data[] = {
            1, 1, 1, 1, 0, 0, 0, 0, 3, 3, 3, 3, 4, 4, 4, 4,
        };
        arr<> measurement{ 2, 2, 4, data };
        config c;

        arr_2d<arr, double> image{ 0, 0, nullptr };
        saft{ 10, 10, c }.compute(measurement, image);

        //TODO: check output
        //image.dump(std::cout);
    }

    void test_saft_to_tof()
    {
        const unsigned width = 3000;
        const unsigned height = 1500;
        const double threshold = 0.4;
        const bool verbose = false;
        std::stringstream ss;
        ss
          << "./data/simulated/one_reflector_multiple_heights/10_fmc.bin;./"
             "data/simulated/one_reflector_multiple_heights/"
             "reference_signal.csv;500;44e-3;1.2e-3;20e6;6350;16;0.00127;5;620;"
             "64;true;one_reflector_multiple_heights/10_fmc.log;10000;36;\n";

        visualizer v{ 1, false };
        v.load(ss, 0.0);
        config& c = v.c;

        arr<> measurement(c.elements, c.elements, c.samples);
        reader::read(c.measurement_file, measurement);

        v.compute_saft(width, height, measurement);

        arr_2d<arr, double> saft_image(0, 0, nullptr);
        v.intensities.copy_to(saft_image);

        v.tofs.clear();
        v.vals.clear();

        // Test this method: the tofs returned by this method should exist in the original saft_image! (but only approximatively, they could lie in a neighbouring pixel if the tof lies on the boundary of both (A).)
        saft{ width, height, c }.populate_from(
          saft_image, threshold, v.tofs, v.vals);

        v.vals_index.resize(v.vals.size());
        std::iota(v.vals_index.begin(), v.vals_index.end(), 0);

        unsigned problematic_reflectors = v.compute(width, height, verbose);
        // Let 1/5 of the reflectors be non-drawable.
        TSM_ASSERT_LESS_THAN(
          std::to_string(v.tofs.size() - problematic_reflectors) +
            " could be drawn from " + std::to_string(v.tofs.size()),
          problematic_reflectors,
          v.tofs.size() / 5.0);

        // The rest needs to be fixed or removed : see (A) above.
        // unsigned incorrect_pixels = 0;
        // unsigned correct_pixels = 0;
        //// Assert the generated image from saft-candidates only contains saft-candidates.
        //v.intensities.for_ijkv(
        //  [&](unsigned i, unsigned j, unsigned k, double v) {
        //      if (v > 0.0) {
        //          if (saft_image.at(i, j, k) >= threshold) {
        //              correct_pixels++;
        //          } else {
        //              incorrect_pixels++;
        //          }
        //      }
        //  });

        //TS_ASSERT_LESS_THAN(incorrect_pixels, 2 * problematic_reflectors);
        //TS_ASSERT_LESS_THAN(incorrect_pixels, correct_pixels);
    }
};
