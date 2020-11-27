#ifndef SFT_H
#define SFT_H

#include "arr.h"
#include <cmath>

// does the dft followed by inverse dft
// the fftw implementation needs a state (,,plan'') so heres an abstract class
/// Convolution abstract class that computes convolutions.
class convolution
{
  protected:
    virtual void run(arr<>& f,
                     arr<>& g,
                     arr<>& out) = 0; ///< Computes the convolution.

  public:
    void convolve(arr<>& f, arr<>& g, arr<>& out)
    {
        //assert(((dynamic_cast<) || (f.dim1 == g.dim1 && f.dim2 == g.dim2)) &&
        //"f, g should have same count of senders and receivers");
        assert(((f.dim1 == out.dim1 && f.dim2 == out.dim2) ||
                (g.dim1 == out.dim1 && g.dim2 == out.dim2)) &&
               "Incompatible Outputdimensions!");
        assert(f.dim3 + g.dim3 - 1 == out.dim3 &&
               "Incompatible Outputdimensions!");
        run(f, g, out);
    }
    void operator()(arr<>& f, arr<>& g, arr<>& out)
    {
        this->convolve(f, g, out);
    };
    virtual ~convolution(){};
};

/// Naive convolution in quadratic runtime.
class slow_convolution : public convolution
{
  protected:
    virtual void run(arr<>& f, arr<>& g, arr<>& out) override;

  public:
    slow_convolution();
    virtual ~slow_convolution() override;
};

#endif // SFT_H
