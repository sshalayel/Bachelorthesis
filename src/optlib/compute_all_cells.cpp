#include "compute_all_cells.h"

intensity_and_time_of_flight::intensity_and_time_of_flight(double intensity,
                                                           time_of_flight&& tof)
  : intensity(intensity)
  , tof(std::move(tof))
{}

intensity_and_time_of_flight::intensity_and_time_of_flight(
  intensity_and_time_of_flight&& iatof)
  : intensity_and_time_of_flight(iatof.intensity, std::move(iatof.tof))
{}

intensity_and_time_of_flight&
intensity_and_time_of_flight::operator=(intensity_and_time_of_flight&& iatof)
{
    intensity = iatof.intensity;
    tof = std::move(iatof).tof;
    return *this;
}

intensity_and_time_of_flight&
compute_all_cells::fill_cell(unsigned i, unsigned j)
{
    auto& data = cells(i, j).find().get().data;
    if (data) {
    } else {
        data = intensity_and_time_of_flight{
            0.0,
            time_of_flight(elements, elements, 0),
        };
        pixel_to_tof(i, j, data->tof);
    }
    assert_that(cells(i, j).find().get().data, "Writing data failed!");
    return *data;
}

compute_all_cells::compute_all_cells(
  std::function<void(unsigned, unsigned, time_of_flight&)> pixel_to_tof,
  unsigned width,
  unsigned height,
  unsigned elements,
  unsigned roi_end)
  : pixel_to_tof(pixel_to_tof)
  , width(width)
  , height(height)
  , elements(elements)
  , roi_end(roi_end)
  , cells(width, height)
{}

void
compute_all_cells::compute_all(arr_2d<arr, double>& intensities,
                               bool with_diagonal)
{
    intensities.realloca(width, height);
    double intensity = 0;

    time_of_flight tof(elements, elements, {});

    // TODO instead of saving all tofs, maybe just save those needed and reuse them?

    //merge cells together
    for (unsigned i = 0; i < width; i++) {
        for (unsigned j = 0; j < height; j++) {
            intensity_and_time_of_flight& current = fill_cell(i, j);

            //check left, lower and left-lower neighbour
            for (int ni = -1; ni < 2; ni++) {
                for (int nj = -1; nj < 2; nj++) {

                    if (ni == 0 && nj == 0) {
                        continue;
                    }

                    if (0 <= i + ni && i + ni < width && 0 <= j + nj &&
                        j + nj < height) {

                        intensity_and_time_of_flight& representant =
                          fill_cell(i + ni, j + nj);

                        if (with_diagonal) {
                            if (std::equal(representant.tof.begin(),
                                           representant.tof.end(),
                                           current.tof.begin())) {
                                cells(i, j).unite(cells(i + ni, j + nj));
                            }
                        } else {
                            if (std::equal(representant.tof.diagonal_begin(),
                                           representant.tof.diagonal_end(),
                                           current.tof.diagonal_begin())) {
                                cells(i, j).unite(cells(i + ni, j + nj));
                            }
                        }
                    }
                }
            }
        }
    }

    std::cerr << "Cells created!" << std::endl;

    //color cells
    for (unsigned i = 0; i < width; i++) {
        for (unsigned j = 0; j < height; j++) {
            auto& current_representant = cells.at(0, i, j).find().get();
            assert_that(current_representant.data,
                        "Representant should have data!");
            double& current_intensity = current_representant.data->intensity;

            if (current_intensity == 0) {
                intensity += (1.0 + std::sqrt(5.0)) / 2.0;
                if (intensity > 10.0) {
                    intensity -= 10.0;
                }
                //intensity++;

                current_intensity = intensity;
            }

            time_of_flight& current = current_representant.data->tof;

            bool drawable = true;
            for (unsigned x = 0; x < current.senders; x++) {
                if (current(x, x) > roi_end) {
                    drawable = false;
                    break;
                }
            }

            if (drawable) {
                intensities.at(i, j) = current_intensity;
            } else {
                intensities.at(i, j) = 0.0;
            }
        }
    }
}
