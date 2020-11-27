#include "../optlib/arr.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <numeric>

class arr_test : public CxxTest::TestSuite
{
  private:
    unsigned f(unsigned i, unsigned j, unsigned k, unsigned length)
    {
        return k + length * j + length * length * k;
    }

    unsigned f2(unsigned i, unsigned j, unsigned k, unsigned length)
    {
        return i * length * length + length * j + k;
    }

  public:
    void test_sub_to()
    {
        const unsigned elements = 10;
        const unsigned measurement_length = 7;

        const unsigned sub_elements = elements / 4;
        const unsigned sub_measurement_length = elements / 4;

        const unsigned sub_start_1 = 3, sub_start_2 = 4, sub_start_3 = 5;

        arr<> huge(elements, elements, measurement_length);
        std::iota(huge.begin(), huge.end(), 0);

        arr<> small(sub_elements, sub_elements, sub_measurement_length);
        huge.sub_to(small, sub_start_1, sub_start_2, sub_start_3);

        small.for_ijkv(
          [&](unsigned i, unsigned j, unsigned k, double& current) {
              TS_ASSERT_EQUALS(
                current,
                huge.at(i + sub_start_1, j + sub_start_2, k + sub_start_3));
          });
    }

    void test_symmetry()
    {
        const unsigned length = 10;
        unsigned i = 0;
        symmetric_arr<arr<>> s(length, length, length);
        std::generate(s.data, s.data + s.size(), [&i]() { return i++; });

        TS_ASSERT_EQUALS(s.data, &s.at(0, 0, 0));
        TS_ASSERT_EQUALS(s.data + s.size() - 1,
                         &s.at(length - 1, length - 1, length - 1));
        TS_ASSERT_EQUALS(length - 1,
                         &s.at(length - 1, length - 1, length - 1) -
                           &s.at(length - 1, length - 1, 0));

        s.at(0, 0, 0) = 42;
        TS_ASSERT_EQUALS(s.data[0], 42);
        s.at(length - 1, length - 1, length - 1) = 43;
        TS_ASSERT_EQUALS(s.data[s.size() - 1], 43);

        for (unsigned i = 0; i < length; i++) {
            for (unsigned j = 0; j < length; j++) {
                for (unsigned k = 0; k < length; k++) {
                    TS_ASSERT_EQUALS(s.at(i, j, k), s.at(j, i, k));
                }
            }
        }
    }

    void test_scale()
    {
        unsigned length = 5;
        double data[] = { 1, 2, 3, 4, 5 };

        arr_1d<> a(length);
        std::copy(data, data + (sizeof(data) / sizeof(double)), a.data);

        a.scale_to(1.0);

        for (unsigned i = 0; i < length; i++) {
            TS_ASSERT_EQUALS(a.at(0, 0, i), data[i] / 5.0);
        }
    }

    void test_scale_2()
    {
        unsigned length = 5;
        double data[] = { 1, -2, 3, 4, -5 };

        arr_1d<> a(length);
        std::copy(data, data + (sizeof(data) / sizeof(double)), a.data);

        a.scale_to(1.0);

        for (unsigned i = 0; i < length; i++) {
            TS_ASSERT_EQUALS(a.at(0, 0, i), data[i] / 5.0);
        }
    }

    void test_scale_3()
    {
        unsigned length = 5;
        double data[] = { -1, 2, -7, 4, 3 };

        arr_1d<> a(length);
        std::copy(data, data + (sizeof(data) / sizeof(double)), a.data);

        a.scale_to(1.0);

        for (unsigned i = 0; i < length; i++) {
            TS_ASSERT_EQUALS(a.at(0, 0, i), data[i] / 7.0);
        }
    }

    void test_normalize()
    {
        unsigned length = 5;
        double data[] = { 1, 2, 3, 4, 5 };

        arr_1d<> a(length);
        std::copy(data, data + (sizeof(data) / sizeof(double)), a.data);

        a.normalize_to(1.0);

        for (unsigned i = 0; i < length; i++) {
            TS_ASSERT_EQUALS(a.at(0, 0, i), (data[i] - 1) / 4.0);
        }
    }
    void test_invert()
    {
        unsigned length = 5;
        double data[] = { 1, 2, 3, 4, 5 };

        arr_1d<> a(length, data);
        arr_1d<> b(length);

        a.invert(b);

        for (unsigned i = 0; i < length; i++) {
            TS_ASSERT_EQUALS(b.at(0, 0, i), data[length - 1 - i]);
        }
    }

    void test_array()
    {
        int a = 7, b = 11, c = 13;

        arr<> arr(a, b, c);
        auto f = [](int i, int j, int k) -> double {
            return i * 10000 + j * 100 + k * 71;
        };
        // write in order
        for (int i = 0; i < a; i++) {
            for (int j = 0; j < b; j++) {
                for (int k = 0; k < c; k++) {
                    arr(i, j, k) = f(i, j, k);
                }
            }
        }
        // read in order
        for (int i = 0; i < a; i++) {
            for (int j = 0; j < b; j++) {
                for (int k = 0; k < c; k++) {
                    TSM_ASSERT_EQUALS(std::to_string(i) + ", " +
                                        std::to_string(j) + ", " +
                                        std::to_string(k) + " : ",
                                      arr(i, j, k),
                                      f(i, j, k));
                }
            }
        }
        // write in reverse
        for (int i = 0; i < a; i++) {
            for (int j = 0; j < b; j++) {
                for (int k = 0; k < c; k++) {
                    arr(a - i - 1, b - j - 1, c - k - 1) =
                      f(a - i - 1, b - j - 1, c - k - 1);
                }
            }
        }
        // read in order
        for (int i = 0; i < a; i++) {
            for (int j = 0; j < b; j++) {
                for (int k = 0; k < c; k++) {
                    TSM_ASSERT_EQUALS("(reverse) " + std::to_string(i) + ", " +
                                        std::to_string(j) + ", " +
                                        std::to_string(k) + " : ",
                                      arr(i, j, k),
                                      f(i, j, k));
                }
            }
        }
    }
};
