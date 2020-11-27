#include "../optlib/reader.h"
#include <cxxtest/TestSuite.h>
#include <sstream>

class reader_test : public CxxTest::TestSuite
{
  public:
    const double delta = 0.0001;
    void test_read_civa()
    {
        std::string data =
          "Center frequency;2;\nSampling freq.;60;\nSamples;512;\nReference "
          "amplitude;100;\n;\nTime(us);Amplitude(dB);Amplitude(val);\n0;0E0;-"
          "1E2;\n0.0167;-1.9041E-1;-9.7832E1;\n0.0333;-7.7908E-1;-9.1421E1;\n0."
          "05;-1.8254E0;-8.1046E1;\n0.0667;-3.4583E0;-6.7156E1;\n0.0833;-5."
          "9593E0;-5.0354E1;\n0.1;NaN;0E0;\n0.1167;-1.9155E1;-1."
          "1022E1;\n";
        std::stringstream ss(data);
        civa_txt_reader r;
        unsigned length = 8;
        arr<> result(1, 1, length);
        TS_ASSERT(r.run(ss, result));
        double values[] = { -1e2,      -9.7832E1, -9.1421E1, -8.1046E1,
                            -6.7156E1, -5.0354E1, 0.0, -1.1022E1 };

        for (unsigned i = 0; i < length; i++) {
            TS_ASSERT_DELTA(result(0, 0, i), values[i], 0.0001);
        }
    }

    void test_read1234()
    {
        double data[] = { 1, 2, 3, 4 };
        std::stringstream ss(std::string((char*)data, 4 * sizeof(double)));

        binary_reader<double> br;
        arr<> a(1, 1, 4);
        TS_ASSERT(br.run(ss, a));
        for (unsigned i = 0; i < 4; i++) {
            TS_ASSERT_DELTA(a(0, 0, i), data[i], this->delta);
        }
    }
};