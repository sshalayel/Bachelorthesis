#ifndef ARR_H
#define ARR_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>

/// A wrapper for some T* providing 3D-indexing.
template<typename T>
class proxy_arr
{
  protected:
    /// Needed for deallocation in destructor.
    bool owner;
    /// Indexing provided by subclass implementation.
    virtual unsigned index(unsigned i, unsigned j, unsigned k) const;
    /// Checks if indexes are in bounds.
    virtual bool check_bounds(unsigned i, unsigned j, unsigned k) const;

  public:
    /// Size of the array for each dimension.
    unsigned dim1;
    /// Size of the array for each dimension.
    unsigned dim2;
    /// Size of the array for each dimension.
    unsigned dim3;
    /// The internal array.
    T* data;

    /// The internal array. Can be used as iteratorstart.
    const T* begin() const { return data; };
    /// The internal array. Can be used as iteratorend.
    const T* end() const { return data + size(); };
    /// The internal array. Can be used as iteratorstart.
    T* begin() { return data; };
    /// The internal array. Can be used as iteratorend.
    T* end() { return data + size(); };
    /// Number of entries in the current window (dim1 * dim2 * dim3)
    virtual unsigned size() const;
    /// Avoid implicit copying, use the explicit copy_to() method to copy.
    proxy_arr(const proxy_arr&) = delete;
    /// Needed by std::vector<> for reallocating, transfer ownership of underlaying array.
    proxy_arr(proxy_arr&&);
    /// Needed by std::vector<> for reallocating, transfer ownership of underlaying array.
    proxy_arr& operator=(proxy_arr&&);
    /// Allocates some memory.
    proxy_arr(unsigned dim1, unsigned dim2, unsigned dim3);
    /// Wrap around some existing data.
    proxy_arr(unsigned dim1,
              unsigned dim2,
              unsigned dim3,
              T* data,
              bool take_ownership = false);
    /// Deletes internal buffer if dimensions are not equal and reallocate new buffer.
    virtual void realloca(unsigned dim1, unsigned dim2, unsigned dim3);
    /// Deletes internal buffer if dimensions are not equal and reallocate new buffer.
    virtual void realloca(unsigned dim1,
                          unsigned dim2,
                          unsigned dim3,
                          T* data,
                          bool take_ownership = false);
    virtual ~proxy_arr();
    /// True if both arr have same dimensions.
    bool same_dim(const proxy_arr& arr) const;

    /// Works like ,,std::substr''.
    void sub_to(proxy_arr& dest,
                unsigned i_start,
                unsigned j_start,
                unsigned k_start);

    /// Works like ,,std::substr''.
    void sub_to(proxy_arr& dest,
                unsigned i_start,
                unsigned i_length,
                unsigned j_start,
                unsigned j_length,
                unsigned k_start,
                unsigned k_length);

    /// Iterates over all entries.
    template<typename Func>
    void for_each(Func f)
    {
        std::for_each(this->begin(), this->end(), f);
    }

    /// Iterates over all dimensions.
    template<typename Func>
    void for_ijk(Func f)
    {
        for (unsigned i = 0; i < this->dim1; i++) {
            for (unsigned j = 0; j < this->dim2; j++) {
                for (unsigned k = 0; k < this->dim3; k++) {
                    this->at(i, j, k) = f(i, j, k);
                }
            }
        }
    }

    /// Iterates over all dimensions.
    template<typename Func>
    void for_ijkv(Func f) const
    {
        for (unsigned i = 0; i < this->dim1; i++) {
            for (unsigned j = 0; j < this->dim2; j++) {
                for (unsigned k = 0; k < this->dim3; k++) {
                    f(i, j, k, at(i, j, k));
                }
            }
        }
    }

    /// Iterates over all dimensions.
    template<typename Func>
    void for_ijkv(Func f)
    {
        for (unsigned i = 0; i < this->dim1; i++) {
            for (unsigned j = 0; j < this->dim2; j++) {
                for (unsigned k = 0; k < this->dim3; k++) {
                    f(i, j, k, at(i, j, k));
                }
            }
        }
    }

    /// Checks if all values of this array lie in lower and upper.
    bool in_bounds(T lower, T upper)
    {
        for (unsigned i = 0; i < this->dim1; i++) {
            for (unsigned j = 0; j < this->dim2; j++) {
                for (unsigned k = 0; k < this->dim3; k++) {
                    const T& current = at(i, j, k);
                    if (!(lower <= current && current < upper)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    /// Const 3D-indexing.
    T const& operator()(unsigned i, unsigned j, unsigned k) const
    {
        return this->at(i, j, k);
    }
    ///Nonconst 3D-indexing.
    T& operator()(unsigned i, unsigned j, unsigned k)
    {
        return this->at(i, j, k);
    }

    /// Const 3D-indexing.
    T const& at(unsigned i, unsigned j, unsigned k) const
    {
        return this->data[this->index(i, j, k)];
    }
    /// Nonconst 3D-indexing.
    T& at(unsigned i, unsigned j, unsigned k)
    {
        return this->data[this->index(i, j, k)];
    }

    ///// Const 3D-indexing.
    //T const& operator()(size pos) const
    //{
    //    return this->at(pos.dim1, pos.dim2, pos.dim3);
    //}
    /////Nonconst 3D-indexing.
    //T& operator()(size pos) { return this->at(pos.dim1, pos.dim2, pos.dim3); }

    ///// Const 3D-indexing.
    //T const& at(size pos) const
    //{
    //    return this->at(pos.dim1, pos.dim2, pos.dim3);
    //}
    ///// Nonconst 3D-indexing.
    //T& at(size pos) { return this->at(pos.dim1, pos.dim2, pos.dim3); }
};

/// Contains the size of the problem.
struct size
{
    unsigned dim1;
    unsigned dim2;
    unsigned dim3;
    size() = default;
    template<typename T>
    size(const proxy_arr<T>& a)
      : dim1(a.dim1)
      , dim2(a.dim2)
      , dim3(a.dim3)
    {}
    size(unsigned dim1, unsigned dim2, unsigned dim3)
      : dim1(dim1)
      , dim2(dim2)
      , dim3(dim3)
    {}
    size(unsigned dim2, unsigned dim3)
      : dim1(1)
      , dim2(dim2)
      , dim3(dim3)
    {}
    size(unsigned dim3)
      : dim1(1)
      , dim2(1)
      , dim3(dim3)
    {}
};

/// Symmetric Array that only works with data on one half.
template<typename ARR, int unsymmetric_index = 2>
class symmetric_arr : public ARR
{
  public:
    using ARR::ARR;
    virtual unsigned index(unsigned i, unsigned j, unsigned k) const override;
    unsigned size() const override;
    static unsigned size(struct size s);

    //gauss : n(n+1)/2
    static unsigned gauss(unsigned n);
};

template<typename ARR, int unsymmetric_index>
unsigned
symmetric_arr<ARR, unsymmetric_index>::size(struct size s)
{
    if (unsymmetric_index == 0) {
        assert(s.dim2 == s.dim3);
        return gauss(s.dim2) * s.dim1;
    } else if (unsymmetric_index == 2) {
        assert(s.dim1 == s.dim2);
        return gauss(s.dim1) * s.dim3;
    }
}

template<typename ARR, int unsymmetric_index>
unsigned
symmetric_arr<ARR, unsymmetric_index>::gauss(unsigned n)
{
    return n * (n + 1) / 2;
}

template<typename ARR, int unsymmetric_index>
unsigned
symmetric_arr<ARR, unsymmetric_index>::size() const
{
    return size(*this);
}

template<typename ARR, int unsymmetric_index>
unsigned
symmetric_arr<ARR, unsymmetric_index>::index(unsigned i,
                                             unsigned j,
                                             unsigned k) const
{
    unsigned dim1 = this->dim1;
    unsigned dim2 = this->dim2;
    unsigned dim3 = this->dim3;

    if (unsymmetric_index == 0) {
        std::swap(i, j); // i,j,k => j,i,k
        std::swap(dim1, dim2);

        std::swap(j, k); // j,i,k => k,i,j
        std::swap(dim2, dim3);
    } else {
        assert(unsymmetric_index == 2);
    }

    assert(dim1 == dim2);

    if (i > j) {
        std::swap(i, j);
    }

    return (j - i + gauss(dim1) - gauss(dim1 - i)) * dim3 + k;
}

/// Some proxy_arr but exclusively for numbers. Can print, add, substract, ...
template<typename T = double>
class arr : public proxy_arr<T>
{
  public:
    /// Can be used to mirror the data in some dimension. [1,2,3] -> [3,2,1]
    void invert(arr& out,
                bool dim1 = false,
                bool dim2 = false,
                bool dim3 = true);
    /// Allocate new Array.
    arr(unsigned dim1, unsigned dim2, unsigned dim3);
    virtual ~arr() override;
    /// Move values/ownership of arr to this(for vectors).
    arr(arr&& arr);
    arr& operator=(arr&& arr);
    arr(unsigned dim1,
        unsigned dim2,
        unsigned dim3,
        T* data,
        bool take_ownership = false);
    /// Performs a copy to an array, T has to be implicitly convertible to U.
    template<typename U>
    void copy_to(arr<U>& out) const;

    /// Printing used for debugging (a.dump(std::cout)).
    virtual void dump(std::ostream& os) const;

    /// Finds the max/min values of array.
    void maxmin(T& max, T& min);
    /// Remove all small values that lie between min and min + (max-min) * percentage, set them to min + (max-min) * percentage
    void lower_threshold_to(double percentage);
    /// Remove all small values whose absolute value lie between min and min + (max-min) * percentage, set them to 0
    void threshold_to(double percentage);
    /// Remove all small values that lie under cutoff.
    void lower_cutoff(T cutoff);
    /// Normalize this array to be in [0, n] using : (array_ijk - min) * n / (max - min).
    bool normalize_to(T n);
    /// Normalize this array to be in [0, n] using : log(1 + (array_ijk - min) * n / (max - min)) * n / log(n)
    bool normalize_to_with_log(T n);
    /// Scale this array to be in [-n, n] using : array_ijk * n/|max| or array_ijk * n/|min| (depending on which has a greater absolute value), returns false for arrays consisting entirely of 0s.
    bool scale_to(T s);
    /// Scalar addition. Returns this. Needed when playing with offsets.
    arr& add(T add);
    /// Dotwise in_place addition. Returns this.
    arr& add(const arr& arr);
    /// Dotwise in_place substraction. Returns this.
    arr& sub(const arr& arr);

    /// Needed for ordered sets.
    bool operator<(const arr& arr) const
    {
        return std::lexicographical_compare(
          this->begin(), this->end(), arr.begin(), arr.end());
    }
};

template<typename T>
bool
operator<(const std::reference_wrapper<T> a, const std::reference_wrapper<T> b)
{
    return a.get() < b.get();
}

template<typename T>
std::ostream&
operator<<(std::ostream& ostr, const arr<T>& arr)
{
    arr.dump(ostr);
    return ostr;
}

/// A three dimensional T array-wrapper for one dimensional matrix (wrapper[i,j,k] = internal_array[k]).
template<template<typename> class ARR = arr, typename T = double>
class arr_1d : public ARR<T>
{
  public:
    /// Wrap around existing data.
    arr_1d(unsigned dim, T* data, bool take_ownership = false);
    /// Allocate own memory.
    arr_1d(unsigned dim);
    virtual ~arr_1d() override;

    arr_1d(arr_1d&& other);
    arr_1d& operator=(arr_1d&& other);

    /// Remove leading and trailing data that is between lower/upper bound.
    void trim(T lower_bound, T upper_bound, arr_1d& out);

    using ARR<T>::operator();
    using ARR<T>::at;
    /// Const indexing.
    T const operator()(unsigned i) const { return this->at(i); }
    /// Const indexing.
    T const at(unsigned i) const { return this->data[this->index(0, 0, i)]; }
    /// Nonconst indexing.
    T& at(unsigned i) { return this->data[this->index(0, 0, i)]; }
    /// Nonconst indexing.
    T& operator()(unsigned i) { return this->at(i); }

    using ARR<T>::realloca;
    void realloca(unsigned dim3, T* data, bool take_ownership)
    {
        ARR<T>::realloca(1, 1, dim3, data, take_ownership);
    }

  protected:
    virtual unsigned index(unsigned i, unsigned j, unsigned k) const override;
};

/// A three dimensional T array-wrapper for two dimensional matrix (wrapper[i,j,k] = internal_array[j,k]).
template<template<typename> class ARR = arr, typename T = double>
class arr_2d : public ARR<T>
{
  public:
    struct diagonal_iterator;

    arr_2d(arr_2d&& temp)
      : ARR<T>(std::move(temp))
    {}

    arr_2d& operator=(arr_2d&& temp)
    {
        ARR<T>::operator=(std::move(temp));
        return *this;
    }

    arr_2d(unsigned senders, unsigned receivers)
      : ARR<T>(1, senders, receivers)
    {}

    /// Wrap around existing data.
    arr_2d(unsigned senders, unsigned receivers, T* data, bool owner = false)
      : ARR<T>(1, senders, receivers, data, owner)
    {}

    using ARR<T>::realloca;
    void realloca(unsigned dim2, unsigned dim3)
    {
        ARR<T>::realloca(1, dim2, dim3);
    }

    void realloca(unsigned dim2,
                  unsigned dim3,
                  T* data,
                  bool take_ownership = false)
    {
        ARR<T>::realloca(1, dim2, dim3, data, take_ownership);
    }

    virtual ~arr_2d() override{};

    using ARR<T>::operator();

    using ARR<T>::at;

    /// Const indexing.
    T const operator()(unsigned i, unsigned j) const { return this->at(i, j); }
    /// Const indexing.
    T const at(unsigned i, unsigned j) const
    {
        return this->data[this->index(0, i, j)];
    }
    /// Nonconst indexing.
    T& at(unsigned i, unsigned j) { return this->data[this->index(0, i, j)]; }
    /// Nonconst indexing.
    T& operator()(unsigned i, unsigned j) { return this->at(i, j); }

    virtual unsigned index(unsigned i, unsigned j, unsigned k) const override
    {
        return ARR<T>::index(0, j, k);
    }

    diagonal_iterator diagonal_begin() { return { *this, 0 }; }
    diagonal_iterator diagonal_end()
    {
        return { *this, std::min(this->dim2, this->dim3) };
    }

    /// Iterates over all diagonal entries.
    struct diagonal_iterator
    {
      public:
        /// Needed by stl-functions.
        using difference_type = unsigned;
        /// Needed by stl-functions.
        using iterator_category = std::bidirectional_iterator_tag;
        /// Needed by stl-functions.
        using value_type = T;
        /// Needed by stl-functions.
        using pointer = T*;
        /// Needed by stl-functions.
        using reference = T&;

        std::reference_wrapper<arr_2d> self;
        unsigned position;

        diagonal_iterator(arr_2d& self, unsigned position)
          : self(self)
          , position(position)
        {}

        diagonal_iterator(diagonal_iterator& di)
          : self(di.self)
          , position(di.position)
        {}

        diagonal_iterator& operator++()
        {
            assert(position < std::min(self.get().dim2, self.get().dim3));
            position++;
            return *this;
        }

        diagonal_iterator& operator--()
        {
            assert(position > 0);
            position--;
            return *this;
        }

        T& operator*() { return self(position, position); }

        bool operator==(diagonal_iterator it)
        {
            return position == it.position;
        }

        bool operator!=(diagonal_iterator it)
        {
            return position != it.position;
        }
    };
};

/// A three dimensional T array-wrapper for two dimensional matrix (wrapper[i,j,k] = internal_array[j,k]).
template<template<typename> class ARR = arr, typename T = double>
class symmetric_arr_2d : public symmetric_arr<arr_2d<ARR, T>, 0>
{
  public:
    using symmetric_arr<arr_2d<ARR, T>, 0>::symmetric_arr;
    symmetric_arr_2d(symmetric_arr_2d&& a)
      : symmetric_arr<arr_2d<ARR, T>, 0>::symmetric_arr(std::move(a))
    {}
    symmetric_arr_2d& operator=(symmetric_arr_2d&& a)
    {
        symmetric_arr<arr_2d<ARR, T>, 0>::operator=(std::move(a));
        return *this;
    }
    virtual ~symmetric_arr_2d() override{};
};

template<typename T>
proxy_arr<T>::proxy_arr(proxy_arr&& temp)
{
    *this = std::move(temp);
}

template<typename T>
proxy_arr<T>&
proxy_arr<T>::operator=(proxy_arr&& temp)
{
    dim1 = temp.dim1;
    dim2 = temp.dim2;
    dim3 = temp.dim3;
    data = temp.data;
    owner = temp.owner;

    //change ownership from temp to this
    temp.dim1 = 0;
    temp.dim2 = 0;
    temp.dim3 = 0;
    temp.data = nullptr;
    temp.owner = false;
    return *this;
}

template<typename T>
bool
arr<T>::scale_to(T norm)
{
    T max, min;
    maxmin(max, min);

    T abs_max = max >= 0 ? max : -max;
    T abs_min = min >= 0 ? min : -min;
    T div = (abs_max >= abs_min ? abs_max : abs_min);

    if (div == 0.0) {
        return false;
    }

    for (unsigned i = 0; i < this->dim1; i++) {
        for (unsigned j = 0; j < this->dim2; j++) {
            for (unsigned k = 0; k < this->dim3; k++) {
                this->at(i, j, k) = this->at(i, j, k) * norm / div;
            }
        }
    }
    return true;
}

template<typename T>
void
arr<T>::lower_cutoff(T cutoff)
{
    this->for_each([=](T& value) { value = std::max(value, cutoff); });
}

template<typename T>
void
arr<T>::threshold_to(double percentage)
{
    assert(percentage < 1 && percentage >= 0);

    T max, min;
    maxmin(max, min);
    const T bound = std::max(std::abs(max), std::abs(min));
    const T thresh = bound * percentage;
    this->for_each([=](T& value) {
        if (std::abs(value) < thresh) {
            value = 0;
        }
    });
}

template<typename T>
void
arr<T>::lower_threshold_to(double percentage)
{
    assert(percentage < 1 && percentage >= 0);

    T max, min;
    maxmin(max, min);
    const T lower_bound = min + (max - min) * percentage;
    this->for_each([=](T& value) { value = std::max(value, lower_bound); });
}

template<typename T>
bool
arr<T>::normalize_to(T norm)
{
    T max, min;
    maxmin(max, min);

    if (this->dim1 == this->dim2 && this->dim2 == this->dim3 &&
        this->dim3 == 1) {
        this->at(0, 0, 0) = norm;
        return true;
    }

    if (max == min) {
        for (unsigned i = 0; i < this->dim1; i++) {
            for (unsigned j = 0; j < this->dim2; j++) {
                for (unsigned k = 0; k < this->dim3; k++) {
                    this->at(i, j, k) = norm;
                }
            }
        }
        return false;
    } else {
        for (unsigned i = 0; i < this->dim1; i++) {
            for (unsigned j = 0; j < this->dim2; j++) {
                for (unsigned k = 0; k < this->dim3; k++) {
                    this->at(i, j, k) = (this->at(i, j, k) - min) * norm;
                    this->at(i, j, k) /= max - min;
                }
            }
        }
        return true;
    }
}

template<typename T>
bool
arr<T>::normalize_to_with_log(T norm)
{
    T max, min;
    maxmin(max, min);
    if (min < 0) {
        return false;
    }
    this->for_each(
      [=](T& x) { x = std::log2(1 + x) * norm / std::log2(1 + max); });
    return true;
}

template<typename T>
void
arr<T>::maxmin(T& max, T& min)
{
    if (this->size() == 0) {
        max = 0;
        min = 0;
        return;
    }

    max = this->at(0, 0, 0);
    min = this->at(0, 0, 0);

    for (unsigned i = 0; i < this->dim1; i++) {
        for (unsigned j = 0; j < this->dim2; j++) {
            for (unsigned k = 0; k < this->dim3; k++) {
                T current = this->at(i, j, k);
                if (max < current) {
                    max = current;
                }
                if (min > current) {
                    min = current;
                }
            }
        }
    }
}

template<typename T>
arr<T>&
arr<T>::add(T add_me)
{
    this->for_each([&](T& v) { v += add_me; });
    return *this;
}

template<typename T>
arr<T>&
arr<T>::add(const arr& arr)
{
    assert(this->same_dim(arr) &&
           "Dotwise Operations only works on arrays with same dimensions!");
    for (unsigned i = 0; i < this->dim1; i++) {
        for (unsigned j = 0; j < this->dim2; j++) {
            for (unsigned k = 0; k < this->dim3; k++) {
                this->at(i, j, k) += arr.at(i, j, k);
            }
        }
    }
    return *this;
}

template<typename T>
arr<T>&
arr<T>::sub(const arr& arr)
{
    assert(this->same_dim(arr) &&
           "Dotwise Operations only works on arrays with same dimensions!");
    for (unsigned i = 0; i < this->dim1; i++) {
        for (unsigned j = 0; j < this->dim2; j++) {
            for (unsigned k = 0; k < this->dim3; k++) {
                this->at(i, j, k) -= arr.at(i, j, k);
            }
        }
    }
    return *this;
}

template<typename T>
proxy_arr<T>::proxy_arr(unsigned dim1,
                        unsigned dim2,
                        unsigned dim3,
                        T* data,
                        bool take_ownership)
  : owner(take_ownership)
  , dim1(dim1)
  , dim2(dim2)
  , dim3(dim3)
  , data(data)
{}

template<typename T>
void
proxy_arr<T>::realloca(unsigned dim1,
                       unsigned dim2,
                       unsigned dim3,
                       T* data,
                       bool take_ownership)
{
    if (owner) {
        delete[] this->data;
    }
    owner = take_ownership;
    this->dim1 = dim1;
    this->dim2 = dim2;
    this->dim3 = dim3;
    this->data = data;
}

template<typename T>
void
proxy_arr<T>::realloca(unsigned dim1, unsigned dim2, unsigned dim3)
{
    //need to realloca?
    if (this->owner && dim1 == this->dim1 && dim2 == this->dim2 &&
        dim3 == this->dim3) {
        return;
    }
    realloca(dim1, dim2, dim3, new T[dim1 * dim2 * dim3], true);
}

template<typename T>
proxy_arr<T>::proxy_arr(unsigned dim1, unsigned dim2, unsigned dim3)
  : proxy_arr<T>(dim1,
                 dim2,
                 dim3,
                 dim1 * dim2 * dim3 == 0 ? nullptr : new T[dim1 * dim2 * dim3],
                 true)
{}

template<typename T>
arr<T>::arr(unsigned dim1, unsigned dim2, unsigned dim3)
  : proxy_arr<T>(dim1, dim2, dim3)
{}

template<typename T>
bool
proxy_arr<T>::same_dim(const proxy_arr& arr) const
{
    return dim1 == arr.dim1 && dim2 == arr.dim2 && dim3 == arr.dim3;
}

template<typename T>
unsigned
proxy_arr<T>::size() const
{
    return dim1 * dim2 * dim3;
}

template<template<typename> class ARR, typename T>
arr_1d<ARR, T>::arr_1d(arr_1d&& other)
  : ARR<T>(std::move(other))
{}

template<template<typename> class ARR, typename T>
arr_1d<ARR, T>&
arr_1d<ARR, T>::operator=(arr_1d&& other)
{
    ARR<T>::operator=(std::move(other));
    return *this;
}

template<template<typename> class ARR, typename T>
void
arr_1d<ARR, T>::trim(T lower_bound, T upper_bound, arr_1d& out)
{
    unsigned start = 0, end = this->dim3;
    for (unsigned k = 0; k < this->dim3; k++) {
        if ((*this)(0, 0, k) > upper_bound || this->at(0, 0, k) < lower_bound) {
            start = k;
            break;
        }
    }

    for (unsigned k = (*this).dim3 - 1; k > 0; k--) {
        if ((*this)(0, 0, k) > upper_bound || (*this)(0, 0, k) < lower_bound) {
            end = k;
            break;
        }
    }

    out.realloca(1, 1, end - start);
    std::copy(&at(start), &at(end), &out.at(0));
}

template<typename T>
void
arr<T>::dump(std::ostream& os) const
{
    for (unsigned i = 0; i < this->dim1; i++) {
        for (unsigned j = 0; j < this->dim2; j++) {
            os << "( " << i << ", " << j << " ) [";
            for (unsigned k = 0; k < this->dim3; k++) {
                os << this->at(i, j, k) << ", ";
            }
            os << "]\n";
        }
    }
    os << "\n";
}

template<typename T>
proxy_arr<T>::~proxy_arr()
{
    if (owner && this->data) {
        delete[] this->data;
    }
}

template<typename T>
arr<T>::arr(arr&& arr)
  : proxy_arr<T>(std::move(arr))
{}

template<typename T>
arr<T>&
arr<T>::operator=(arr&& arr)
{
    proxy_arr<T>::operator=(std::move(arr));
    return *this;
}

template<typename T>
arr<T>::~arr()
{
    // superclass proxy_arr frees the internal array.
}

template<typename T>
arr<T>::arr(unsigned dim1,
            unsigned dim2,
            unsigned dim3,
            T* data,
            bool take_ownership)
  : proxy_arr<T>(dim1, dim2, dim3, data, take_ownership)
{}

template<typename T>
void
arr<T>::invert(arr& out, bool inv1, bool inv2, bool inv3)
{
    out.realloca(this->dim1, this->dim2, this->dim3);

    out.for_ijk([=](unsigned i, unsigned j, unsigned k) {
        unsigned x = inv1 ? this->dim1 - 1 - i : i;
        unsigned y = inv2 ? this->dim2 - 1 - j : j;
        unsigned z = inv3 ? this->dim3 - 1 - k : k;
        return this->at(x, y, z);
    });
}

template<typename T>
bool
proxy_arr<T>::check_bounds(unsigned i, unsigned j, unsigned k) const
{
    return i < this->dim1 && j < this->dim2 && k < this->dim3;
}

template<typename T>
unsigned
proxy_arr<T>::index(unsigned i, unsigned j, unsigned k) const
{
    assert(check_bounds(i, j, k) && "out of bounds!");

    return k + dim3 * (j + dim2 * i);
}

template<template<class> class ARR, typename T>
unsigned
arr_1d<ARR, T>::index(unsigned i, unsigned j, unsigned k) const
{
    assert(k < this->dim3);
    return k;
}

template<template<class> class ARR, typename T>
arr_1d<ARR, T>::arr_1d(unsigned dim, T* data, bool take_ownership)
  : ARR<T>(1, 1, dim, data, take_ownership)
{}

template<template<class> class ARR, typename T>
arr_1d<ARR, T>::arr_1d(unsigned dim)
  : ARR<T>(1, 1, dim)
{}

template<template<class> class ARR, typename T>
arr_1d<ARR, T>::~arr_1d()
{}

template<typename T>
void
proxy_arr<T>::sub_to(proxy_arr& dest,
                     unsigned i_start,
                     unsigned j_start,
                     unsigned k_start)
{
    sub_to(dest, i_start, dest.dim1, j_start, dest.dim2, k_start, dest.dim3);
}

template<typename T>
void
proxy_arr<T>::sub_to(proxy_arr& dest,
                     unsigned i_start,
                     unsigned i_length,
                     unsigned j_start,
                     unsigned j_length,
                     unsigned k_start,
                     unsigned k_length)
{
    dest.realloca(i_length, j_length, k_length);
    for (unsigned i = 0; i < i_length; i++) {
        for (unsigned j = 0; j < j_length; j++) {
            for (unsigned k = 0; k < k_length; k++) {
                dest.at(i, j, k) = at(i + i_start, j + j_start, k + k_start);
            }
        }
    }
}

template<typename T>
template<typename U>
void
arr<T>::copy_to(arr<U>& out) const
{
    out.realloca(this->dim1, this->dim2, this->dim3);
    out.for_ijk(std::reference_wrapper(*this));
    //out.for_ijk([&](unsigned i, unsigned j, unsigned k) {
    //    return (U)at(i, j, k);
    //});
}

#endif // ARR_H
