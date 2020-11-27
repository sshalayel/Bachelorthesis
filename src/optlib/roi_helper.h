#ifndef ROI_HELPER_H
#define ROI_HELPER_H

#include "coordinates.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <optional>
#include <vector>

/// Helper struct for the ROI : filters out saft solutions that does not respects ROI-start and end.
struct roi
{
    /// ROI-start.
    unsigned start;
    /// ROI-end.
    unsigned end;
    /// Removes all tofs from in, and moves those that respects the ROI into out.
    void filter_tofs_to(std::vector<time_of_flight>& in,
                        std::vector<time_of_flight>& out);
    using values = std::optional<std::reference_wrapper<std::vector<double>>>;
    /// Removes all tofs from in, and moves those that respects the ROI into out.
    void filter_tofs_to(std::vector<time_of_flight>& in,
                        values in_values,
                        std::vector<time_of_flight>& out,
                        values out_values);
};
#endif
