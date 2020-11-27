#include "saft.h"

double
saft::length(double a, double b)
{
    return std::sqrt(a * a + b * b);
}

void
saft::compute(const arr<>& measurement, arr_2d<arr, double>& populate_me)
{
    populate_me.realloca(width, height);
    std::fill(populate_me.begin(), populate_me.end(), 0.0);

    const double roi_start = c.get_roi_start();
    const double roi_length = c.get_roi_length();

    for (unsigned i = 0; i < width; i++) {
        for (unsigned j = 0; j < height; j++) {
            double current = 0;
            for (unsigned s = 0; s < measurement.dim1; s++) {
                //for (unsigned r = 0; r < measurement.dim1; r++) {
                const unsigned r = s;

                double x, y;
                c.pixel_to_tact_coords(width, height, i, j, x, y);
                const double distance_to_s = c.distance_in_tacts(x, y, s);
                const double distance_to_r = c.distance_in_tacts(x, y, r);

                const double distance = distance_to_s + distance_to_r;

                double floor = std::floor(distance) - roi_start;
                double ceil = std::ceil(distance) - roi_start;

                if (ceil < roi_length && floor >= 0) {
                    // Ankathete auf Hypothenuse
                    const double cos_s = y / distance_to_s;
                    const double cos_r = y / distance_to_r;
                    current +=
                      cos_s * cos_r *
                      interpolation::linear(measurement.at(s, r, floor),
                                            measurement.at(s, r, ceil),
                                            distance);
                }
                //}
            }
            populate_me.at(i, j) = std::abs(current);
        }
    }
    populate_me.normalize_to(1.0);
}

saft::saft(unsigned width, unsigned height, config c)
  : width(width)
  , height(height)
  , c(c)
{}

void
saft::populate(const arr<>& measurement,
               double threshold,
               std::vector<time_of_flight>& populate_me,
               optional_value_vector values)
{
    arr_2d<arr, double> pixels(0, 0, nullptr);
    compute(measurement, pixels);

    populate_from(pixels, threshold, populate_me, values);
}

void
saft::populate_from(const arr_2d<arr, double>& saft_image,
                    double threshold,
                    std::vector<time_of_flight>& populate_me,
                    optional_value_vector values)
{
    std::list<time_of_flight> unique_tofs;
    std::set<std::reference_wrapper<time_of_flight>> already_added;
    saft_image.for_ijkv([&](unsigned, unsigned i, unsigned j, double v) {
        if (v > threshold) {
            double x, y;
            c.pixel_to_tact_coords(
              saft_image.dim2, saft_image.dim3, i, j, x, y);
            assert(std::abs(x) <= c.get_roi_end() && y >= 0 &&
                   y <= c.get_roi_end() && "Coordinate is not in image!");

            time_of_flight t(c.elements, c.elements, x);
            c.tact_coords_to_tof(x, y, t);

            unique_tofs.push_back(std::move(t));
            if (already_added.insert(unique_tofs.back()).second) {
                if (values) {
                    values->get().push_back(v);
                }
            } else {
                unique_tofs.pop_back();
            }
        }
    });

    populate_me.reserve(unique_tofs.size());
    for (time_of_flight& tof : unique_tofs) {
        populate_me.push_back(std::move(tof));
    }
}
