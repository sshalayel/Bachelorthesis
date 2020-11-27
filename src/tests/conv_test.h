#include "../optlib/fftw_convolution.h"
#include <cxxtest/TestSuite.h>
#include <iostream>

class SimpleTest : public CxxTest::TestSuite
{
  private:
    void conv_helper(arr<>& a, arr<>& b, int results[])
    {
        slow_convolution sc;
        fourier_convolution fc;
        arr<> slow_convulated(a.dim1, a.dim2, a.dim3 + b.dim3 - 1);
        sc(a, b, slow_convulated);
        arr<> fourier_convulated(a.dim1, a.dim2, a.dim3 + b.dim3 - 1);
        fc(a, b, fourier_convulated);
        // allow rounding error up to delta
        const double delta = 1e-3;
        for (unsigned i = 0; i < a.dim3 + b.dim3 - 1; i++) {
            TS_ASSERT_DELTA(slow_convulated(0, 0, i), results[i], delta);
            TS_ASSERT_DELTA(fourier_convulated(0, 0, i), results[i], delta);
        }
    }

  public:
    /// computes the convolution of {1,2,3} and {4,5,6}
    void test_mini_conv()
    {
        const unsigned array_elements = 1, sample_length = 3;
        fftw_arr<> a(array_elements, array_elements, sample_length);
        fftw_arr<> b(array_elements, array_elements, sample_length);
        for (unsigned i = 0; i < array_elements; i++) {
            for (unsigned j = 0; j < array_elements; j++) {
                for (unsigned k = 0; k < sample_length; k++) {
                    a(i, j, k) = k + 1;
                    b(i, j, k) = k + sample_length + 1;
                }
            }
        }
        int results[] = { 4, 13, 28, 27, 18 };
        conv_helper(a, b, results);
    }
    /// computes the convolution of {1,2,3} and {4,5,6}
    void test_mini_inverted_conv()
    {
        const unsigned array_elements = 1, sample_length = 3;
        fftw_arr<> a(array_elements, array_elements, sample_length);
        fftw_arr<> b(array_elements, array_elements, sample_length);

        for (unsigned i = 0; i < array_elements; i++) {
            for (unsigned j = 0; j < array_elements; j++) {
                for (unsigned k = 0; k < sample_length; k++) {
                    a(i, j, k) = sample_length - k;
                    b(i, j, k) = k + sample_length + 1;
                }
            }
        }
        int results[] = { 12, 23, 32, 17, 6 };

        slow_convolution sc;
        fourier_convolution fc;
        arr<> slow_convulated(a.dim1, a.dim2, a.dim3 + b.dim3 - 1);
        sc(a, b, slow_convulated);
        arr<> fourier_convulated(a.dim1, a.dim2, a.dim3 + b.dim3 - 1);
        fc(a, b, fourier_convulated);
        // allow rounding error up to delta
        const double delta = 1e-3;
        for (unsigned i = 0; i < a.dim3 + b.dim3 - 1; i++) {
            TS_ASSERT_DELTA(slow_convulated(0, 0, i), results[i], delta);
            TS_ASSERT_DELTA(fourier_convulated(0, 0, i), results[i], delta);
        }

        arr<> dot_product(a.dim1, a.dim2, a.dim3);

        fourier_convulated.sub_to(
          dot_product, a.dim1 - 1, a.dim2 - 1, a.dim3 - 1);

        TS_ASSERT_DELTA(dot_product(0, 0, 0), 32, delta);
        TS_ASSERT_DELTA(dot_product(0, 0, 1), 17, delta);
        TS_ASSERT_DELTA(dot_product(0, 0, 2), 6, delta);
    }

    void test_mini_conv_fftw_arr()
    {
        const unsigned array_elements = 1, sample_length = 3;
        fftw_arr<> a(array_elements, array_elements, sample_length);
        fftw_arr<> b(array_elements, array_elements, sample_length);
        for (unsigned i = 0; i < array_elements; i++) {
            for (unsigned j = 0; j < array_elements; j++) {
                for (unsigned k = 0; k < sample_length; k++) {
                    a(i, j, k) = k + 1;
                    b(i, j, k) = k + sample_length + 1;
                }
            }
        }
        int results[] = { 4, 13, 28, 27, 18 };
        conv_helper(a, b, results);
    }

    /// computes the convolution of {1,2,3,4} and {5,6,7}
    void test_mini_conv2()
    {
        unsigned array_elements = 1, sample_length_a = 4, sample_length_b = 3;
        fftw_arr<> a(array_elements, array_elements, sample_length_a);
        fftw_arr<> b(array_elements, array_elements, sample_length_b);
        for (unsigned i = 0; i < array_elements; i++) {
            for (unsigned j = 0; j < array_elements; j++) {
                for (unsigned k = 0; k < sample_length_a; k++) {
                    a(i, j, k) = k + 1;
                }
                for (unsigned k = 0; k < sample_length_b; k++) {
                    b(i, j, k) = k + sample_length_a + 1;
                }
            }
        }

        int results[] = { 5, 16, 34, 52, 45, 28 };
        conv_helper(a, b, results);
    }

    void test_slow_conv()
    {
        bool print = false;

        unsigned array_elements = 1, sample_length = 50;
        slow_convolution sc;
        fourier_convolution fc;
        // t1 has a peak at 250
        // t2 has a peak at 500
        const unsigned peak1 = 5, peak2 = 37, peak3 = 10;
        const double large1 = 3.14, large2 = 12.0, large3 = 1.0;
        fftw_arr<> t1(array_elements, array_elements, sample_length);
        fftw_arr<> t2(array_elements, array_elements, sample_length);
        fftw_arr<> f(array_elements, array_elements, sample_length);
        for (unsigned i = 0; i < array_elements; i++) {
            for (unsigned j = 0; j < array_elements; j++) {
                for (unsigned k = 0; k < sample_length; k++) {
                    t1(i, j, k) = std::max(
                      0.0,
                      10.0 -
                        large1 * std::pow(std::abs((int)peak1 - (int)k), 2));
                    t2(i, j, k) = std::max(
                      0.0,
                      10.0 -
                        large2 * std::pow(std::abs((int)peak2 - (int)k), 2));
                    f(i, j, k) = std::max(
                      0.0,
                      10.0 -
                        large3 * std::pow(std::abs((int)peak3 - (int)k), 2));
                }
            }
        }

        arr<> arr1(t1.dim1, t1.dim2, t1.dim3 + f.dim3 - 1);
        arr<> arr2(t2.dim1, t2.dim2, t2.dim3 + f.dim3 - 1);
        sc(t1, f, arr1);
        sc(t2, f, arr2);

        arr<> farr1(t1.dim1, t1.dim2, t1.dim3 + f.dim3 - 1);
        arr<> farr2(t2.dim1, t2.dim2, t2.dim3 + f.dim3 - 1);
        fc(t1, f, farr1);
        fc(t2, f, farr2);

        double max1 = 0, max2 = 0;
        unsigned maxi1 = 0, maxi2 = 0;
        double fmax1 = 0, fmax2 = 0;
        unsigned fmaxi1 = 0, fmaxi2 = 0;
        for (unsigned i = 0; i < array_elements; i++) {
            for (unsigned j = 0; j < array_elements; j++) {
                for (unsigned k = 0; k < 2 * sample_length - 1; k++) {
                    if (fmax1 < farr1(i, j, k)) {
                        fmax1 = farr1(i, j, k);
                        fmaxi1 = k;
                    }
                    if (fmax2 < farr2(i, j, k)) {
                        fmax2 = farr2(i, j, k);
                        fmaxi2 = k;
                    }
                    if (max1 < arr1(i, j, k)) {
                        max1 = arr1(i, j, k);
                        maxi1 = k;
                    }
                    if (max2 < arr2(i, j, k)) {
                        max2 = arr2(i, j, k);
                        maxi2 = k;
                    }
                    if (print) {
                        std::cout << i << ", ";
                        std::cout << j << ", ";
                        std::cout << k << ": ";
                        std::cout << arr1(i, j, k) << ", " << arr2(i, j, k)
                                  << " vs " << farr1(i, j, k) << ", "
                                  << farr2(i, j, k) << std::endl;
                    }
                }
            }
        }
        TS_ASSERT_EQUALS(maxi1, (peak1 + peak3));
        TS_ASSERT_EQUALS(maxi2, (peak2 + peak3));
        TS_ASSERT_EQUALS(fmaxi1, (peak1 + peak3));
        TS_ASSERT_EQUALS(fmaxi2, (peak2 + peak3));
    }
};
