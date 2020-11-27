#include "convolution.h"

slow_convolution::slow_convolution() {}

void
slow_convolution::run(arr<>& f, arr<>& g, arr<>& ret)
{
    for (unsigned i = 0; i < f.dim1; i++) {
        for (unsigned j = 0; j < f.dim2; j++) {
            for (unsigned k = 0; k < ret.dim3; k++) {
                double curr = 0.0;
                for (unsigned s = 0; s < f.dim3; s++) {
                    if (k - s < 0 || k - s >= g.dim3) {
                        continue;
                    }
                    curr += f.at(i, j, s) * g.at(i, j, k - s);
                }
                ret.at(i, j, k) = curr;
            }
        }
    }
}

slow_convolution::~slow_convolution() {}
