#ifndef LINEAR_INTERPOLATION_H
#define LINEAR_INTERPOLATION_H

#include <cmath>

/// Helper class for linear interpolation (needed in visu and saft).
struct interpolation
{
    /// Interpolates 2 values based on mix-std::floor(mix).
    static double linear(double floor_value, double ceil_value, double mix);
};

#endif
