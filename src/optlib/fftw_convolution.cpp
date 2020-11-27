#include "fftw_convolution.h"

fourier_convolution::fourier_convolution()
  : length(0)
{}

void
fourier_convolution::set_length(unsigned length)
{
    assert(this->length == 0);
    this->length = length;
    unsigned half_length = std::ceil(((double)length) / 2.0) + 1;
    in1 = fftw_alloc_real(length);
    in2 = fftw_alloc_real(length);
    mid1 = fftw_alloc_complex(half_length);
    mid2 = fftw_alloc_complex(half_length);
    double* out = fftw_alloc_real(length);
    r2c = fftw_plan_dft_r2c_1d(length, in1, mid1, FFTW_MEASURE);
    c2r = fftw_plan_dft_c2r_1d(length, mid1, out, FFTW_MEASURE);
    fftw_free(out); // not needed anymore
}
fourier_convolution::~fourier_convolution()
{
    if (length != 0) {
        fftw_free(in1);
        fftw_free(in2);
        fftw_free(mid1);
        fftw_free(mid2);
    }
}
void
fourier_convolution::run(arr<>& dual_solution, arr<>& f, arr<>& conv)
{
    if (length != conv.dim3) {
        set_length(conv.dim3);
    }

    assert((dynamic_cast<fftw_arr<>*>(&dual_solution) ||
            dynamic_cast<arr_1d<fftw_arr, double>*>(&dual_solution)) &&
           "Must be an fftw-array!");
    assert((dynamic_cast<fftw_arr<>*>(&f) ||
            dynamic_cast<arr_1d<fftw_arr, double>*>(&f)) &&
           "Must be an fftw-array!");

    for (unsigned i = 0; i < dual_solution.dim1; i++) {
        for (unsigned j = 0; j < dual_solution.dim2; j++) {
            for (unsigned k = 0; k < dual_solution.dim3; k++) {
                in1[k] = dual_solution.at(i, j, k);
            }
            // add 0 padding to the right
            for (unsigned k = dual_solution.dim3; k < length; k++) {
                in1[k] = 0.0;
            }
            for (unsigned k = 0; k < f.dim3; k++) {
                in2[k] = f.at(i, j, k);
            }
            // add 0 padding to the right
            for (unsigned k = f.dim3; k < length; k++) {
                in2[k] = 0.0;
            }

            // convolution thm says : F(conv(x,y)) = F(x)
            // <dotwise-multiplication> F(y)
            // => conv(x,y) = F^{-1} (F(x) <dotwise multiplication> F(y))
            fftw_execute_dft_r2c(r2c, in1, mid1);
            fftw_execute_dft_r2c(r2c, in2, mid2);

            double half = std::floor(((double)length) / 2.0) + 1.0;
            // because of symmetry, the mid array is only half filled
            for (unsigned k = 0; k < half; k++) {
                // complex dotwise multiplication
                const double r =
                  mid1[k][0] * mid2[k][0] - mid1[k][1] * mid2[k][1];
                const double c =
                  mid1[k][0] * mid2[k][1] + mid1[k][1] * mid2[k][0];
                mid1[k][0] = r;
                mid1[k][1] = c;
            }

            fftw_execute_dft_c2r(c2r, mid1, &conv(i, j, 0));

            // normalize
            for (unsigned k = 0; k < length; k++) {
                conv(i, j, k) /= length;
            }
        }
    }
}
