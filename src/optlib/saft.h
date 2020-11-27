#ifndef SAFT_H
#define SAFT_H

#include "arr.h"
#include "config.h"
#include "coordinates.h"
#include "linear_interpolation.h"
#include <functional>
#include <list>
#include <optional>
#include <set>
#include <utility>

/// @brief Can be used to compute saft.
///
/// It can also transform all saftpixels with a value over a threshold into time_of_flights for warmstarting.
struct saft
{
    /// The values of the saftpixels.
    using optional_value_vector =
      std::optional<std::reference_wrapper<std::vector<double>>>;
    saft(unsigned width, unsigned height, config c);
    /// The dimensions of the saft-image.
    unsigned width;
    /// The dimensions of the saft-image.
    unsigned height;
    /// The information needed to find the measurement-data.
    config c;

    /// Computes SAFT and converts all pixels with an saft-intensity > threshold into a tof (and puts it into populate_me, optionally with the saftpixelvalue into optional_value_vector).
    void populate(const arr<>& measurement,
                  double threshold,
                  std::vector<time_of_flight>& populate_me,
                  optional_value_vector);
    /// Converts all pixels with an saft-intensity > threshold into a tof (and puts it into populate_me, optionally with the saftpixelvalue into optional_value_vector).
    void populate_from(const arr_2d<arr, double>& saft_image,
                       double threshold,
                       std::vector<time_of_flight>& populate_me,
                       optional_value_vector);
    /// Computes SAFT from measurement and write it into image.
    void compute(const arr<>& measurement, arr_2d<arr, double>& image);

    /// Small helper to compute the 2-norm of the vector (a,b).
    static double length(double a, double b);
};

#endif
