#include "../optlib/config.h"
#include <cxxtest/TestSuite.h>
#include <sstream>

class config_test : public CxxTest::TestSuite
{
  public:
    void test_parser()
    {
        std::stringstream ss(
          "#measurement_file;reference_file;max_columns;x_position;pitch;"
          "sampling_rate;wave_speed;elements;wave_length;slave_threshold;"
          "samples;reference_samples;verbose;output;\n"
          "#another comment ü\n"
          "./data/simulated/23.05.19/array_small_specimen_1_SDH.bin;./data/"
          "simulated/23.05.19/"
          "array_small_specimen_1_SDH_refsignal.csv;10;0;1.2e-3;20e6;6350;16;0."
          "00127;0.1;161;64;true;1_sdh.log;0;1234");
        config c;
        c.load(ss, std::nullopt, std::nullopt);

        TS_ASSERT_EQUALS(
          c.measurement_file,
          "./data/simulated/23.05.19/array_small_specimen_1_SDH.bin");
        TS_ASSERT_EQUALS(
          c.reference_file,
          "./data/simulated/23.05.19/array_small_specimen_1_SDH_refsignal.csv");
        TS_ASSERT_EQUALS(c.max_columns, 10);
        TS_ASSERT_EQUALS(c.x_position, 0);
        TS_ASSERT_EQUALS(c.pitch, 1.2e-3);
        TS_ASSERT_EQUALS(c.sampling_rate, 20e6);
        TS_ASSERT_EQUALS(c.wave_speed, 6350);
        TS_ASSERT_EQUALS(c.elements, 16);
        TS_ASSERT_EQUALS(c.wave_length, 0.00127);
        TS_ASSERT_EQUALS(c.samples, 161);
        TS_ASSERT_EQUALS(c.reference_samples, 64);
        TS_ASSERT_EQUALS(c.verbose, true);
        TS_ASSERT_EQUALS(c.output, "1_sdh.log");
        TS_ASSERT_EQUALS(c.offset, 1234);
    }
};
