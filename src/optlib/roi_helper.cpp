#include "roi_helper.h"

void
roi::filter_tofs_to(std::vector<time_of_flight>& in,
                    std::vector<time_of_flight>& out)
{
    filter_tofs_to(in, values{}, out, values{});
}

void
roi::filter_tofs_to(
  std::vector<time_of_flight>& in,
  std::optional<std::reference_wrapper<std::vector<double>>> in_values,
  std::vector<time_of_flight>& out,
  std::optional<std::reference_wrapper<std::vector<double>>> out_values)
{
    out.reserve(in.size());
    if (out_values) {
        assert(in_values && in_values->get().size() == in.size());
        out_values->get().reserve(in.size());
    }
    for (unsigned i = 0; i < in.size(); i++) {
        time_of_flight& tof = in[i];

        if (tof.in_bounds(start, end)) {
            out.push_back(std::move(tof));
            if (out_values) {
                out_values->get().push_back(in_values->get()[i]);
            }
        }
    }
    in.clear();
    if (in_values) {
        in_values->get().clear();
    }
}
