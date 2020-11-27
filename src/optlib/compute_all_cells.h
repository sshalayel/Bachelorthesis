#ifndef COMPUTE_ALL_CELLS_H
#define COMPUTE_ALL_CELLS_H
#include "coordinates.h"
#include <cassert>
#include <functional>
#include <optional>

/// Union-Find-Datastructure.
template<typename T>
class UnionFindElement
{
  public:
    /// Embedded data.
    std::optional<T> data;

    /// Is empty for the representant.
    std::optional<std::reference_wrapper<UnionFindElement>> parent;

    /// Creates a set that is it own representant.
    UnionFindElement(T element_and_parent)
      : data(element_and_parent)
    {}

    UnionFindElement(UnionFindElement&) = delete;

    /// Creates a set that is it own representant.
    UnionFindElement() {}

    ~UnionFindElement() {}

    /// Get representant for this element.
    std::reference_wrapper<UnionFindElement> find()
    {
        if (!parent) {
            return *this;
        } else {
            //find representant
            std::reference_wrapper<UnionFindElement> representant = *this;
            while (auto next_parent = representant.get().parent) {
                representant = *next_parent;
            }

            std::reference_wrapper<UnionFindElement> current = *this;

            //set representant as parent
            while (auto next_parent = current.get().parent) {
                current.get().parent = representant;
                current = *next_parent;
            }

            assert_that(!representant.get().parent, "Incorrect representant!");

            return representant;
        }
    }

    ///Take the representant of e.
    void unite(UnionFindElement& e)
    {
        std::reference_wrapper<UnionFindElement> representant_1 = find();
        std::reference_wrapper<UnionFindElement> representant_2 = e.find();

        assert(!representant_1.get().parent && !representant_1.get().parent &&
               "Incorrect representants!");

        if (&representant_1.get() != &representant_2.get()) {
            representant_1.get().parent = representant_2;
        }
    }
};

struct intensity_and_time_of_flight
{
    intensity_and_time_of_flight(double intensity, time_of_flight&& tof);
    intensity_and_time_of_flight(intensity_and_time_of_flight&&);
    intensity_and_time_of_flight& operator=(intensity_and_time_of_flight&&);
    double intensity;
    time_of_flight tof;
};

struct compute_all_cells
{

    /// Transforms one pixel into one ToF.
    std::function<void(unsigned, unsigned, time_of_flight&)> pixel_to_tof;

    ///Size of image
    unsigned width;

    ///Size of image
    unsigned height;

    ///Number of elements
    unsigned elements;

    /// When to stop drawing?
    unsigned roi_end;

    //contains the colors
    arr_2d<proxy_arr, UnionFindElement<intensity_and_time_of_flight>> cells;

    compute_all_cells(
      std::function<void(unsigned, unsigned, time_of_flight&)> pixel_to_tof,
      unsigned width,
      unsigned height,
      unsigned elements,
      unsigned roi_end);

    void compute_all(arr_2d<arr, double>& intensities, bool with_diagonal);

    intensity_and_time_of_flight& fill_cell(unsigned i, unsigned j);
};

#endif
