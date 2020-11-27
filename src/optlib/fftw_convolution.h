#ifndef FFTW_CONVOLUTION
#define FFTW_CONVOLUTION

#include "convolution.h"
#include "fftw_arr.h"
#include <fftw3.h>

/// Fast convolution using fftw's fourier transform.
class fourier_convolution : public convolution
{
  private:
    double *in1, *in2;
    fftw_complex *mid1, *mid2;
    fftw_plan r2c, c2r;

  protected:
    virtual void run(arr<>& f, arr<>& g, arr<>& out) override;
    unsigned length;
    void set_length(unsigned length);

  public:
    fourier_convolution();
    fourier_convolution(const fourier_convolution&) = delete;
    virtual ~fourier_convolution() override;
};

#endif // FFTW_CONVOLUTION
