#ifndef FFTW_ARR_H
#define FFTW_ARR_H

#include "arr.h"
#include <fftw3.h>

/// Array subclass using fftw's malloc und free functions.
template<typename T = double>
class fftw_arr : public arr<T>
{
  public:
    fftw_arr(unsigned dim1, unsigned dim2, unsigned dim3);
    fftw_arr(unsigned dim1,
             unsigned dim2,
             unsigned dim3,
             T* data,
             bool take_ownership);
    virtual void realloca(unsigned dim1, unsigned dim2, unsigned dim3) override;
    virtual void realloca(unsigned dim1,
                          unsigned dim2,
                          unsigned dim3,
                          T* data,
                          bool take_ownership = false) override;
    fftw_arr(fftw_arr&& a);
    virtual ~fftw_arr() override;
};

template<typename T>
fftw_arr<T>::fftw_arr(fftw_arr&& a)
  : arr<T>(std::move(a))
{}

template<typename T>
fftw_arr<T>::fftw_arr(unsigned dim1, unsigned dim2, unsigned dim3)
  : arr<T>(dim1, dim2, dim3, (T*)fftw_malloc(dim1 * dim2 * dim3 * sizeof(T)))
{}

template<typename T>
fftw_arr<T>::fftw_arr(unsigned dim1,
                      unsigned dim2,
                      unsigned dim3,
                      T* data,
                      bool take_ownership)
  : arr<T>(0, 0, 0, nullptr, false)
{
    assert(data == nullptr && "fftw arrays cannot take T* as data as they need "
                              "specially allocated data");
}

template<typename T>
void
fftw_arr<T>::realloca(unsigned dim1, unsigned dim2, unsigned dim3)
{
    //need to realloca?
    if (this->owner && dim1 == this->dim1 && dim2 == this->dim2 &&
        dim3 == this->dim3) {
        return;
    }
    realloca(
      dim1, dim2, dim3, (T*)fftw_malloc(dim1 * dim2 * dim3 * sizeof(T)), true);
}
template<typename T>
void
fftw_arr<T>::realloca(unsigned dim1,
                      unsigned dim2,
                      unsigned dim3,
                      T* data,
                      bool take_ownership)
{
    //need to realloca?
    if (this->owner) {
        fftw_free(this->data);
    }
    this->owner = take_ownership;
    this->dim1 = dim1;
    this->dim2 = dim2;
    this->dim3 = dim3;
    this->data = data;
}

template<typename T>
fftw_arr<T>::~fftw_arr()
{
    this->owner = false;
    fftw_free(this->data);
}
#endif //FFTW_ARR_H
