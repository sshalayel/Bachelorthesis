#include "linear_interpolation.h"

double
interpolation::linear(double floor_value, double ceil_value, double mix)
{
    mix -= std::floor(mix);

    //interpolate
    double a = (1.0 - mix) * floor_value;
    double b = mix * ceil_value;

    return a + b;
}
