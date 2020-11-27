#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "compute_all_cells.h"
#include "config.h"
#include "coordinates.h"
#include "csv_tools.h"
#include "linear_interpolation.h"
#include "saft.h"
#include "simplexoid.h"
#include "stop_watch.h"
#include <algorithm>
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <variant>

/// Maps intensities (in [0,1]) to colors according to colors.
struct color_palette
{
  public:
    /// The color.
    using color = unsigned char;
    /// A chain of colors.
    using colors = std::vector<color>;
    color_palette();
    color_palette(colors&& reds, colors&& greens, colors&& blues);

    /// The amount of red for this intensity.
    color red(double intensity) const;
    /// The amount of green for this intensity.
    color green(double intensity) const;
    /// The amount of blue for this intensity.
    color blue(double intensity) const;

    /// Used by green(), blue() and red() for interpolation.
    color helper(double intensity, const colors& colors) const;

    /// The actual colors in the palette.
    colors reds, greens, blues;
};

/// Handles all the visualisation stuff: loads some cgdump and transform it into some image.
class visualizer
{
    /// Ignores non-diagonal elements of tofs when enabled.
    bool only_diagonal = true;

  public:
    /// Small enum used to decide if visualiser should summarise ToFs together.
    enum summarise_option
    {
        SUMMARISE,
        DO_NOT_SUMMARISE
    };

    /// Small enum used to color all reflectors with random colors.
    enum color_option
    {
        RANDOM_COLORS,
        NORMAL_COLORS
    };
    /// @brief Overlapping margin is the distance between 2 tacts. Should be normally 1.
    ///
    /// The old_version iterates over all pixels and is slower then the new one.
    visualizer(double overlapping_margin, bool old_version = false);
    /// Load a cgdump stream.
    void load(std::istream& is,
              double threshold,
              summarise_option summarise = SUMMARISE,
              color_option color = NORMAL_COLORS);
    /// Load a cgdump file.
    void load(const std::string& filename,
              double threshold,
              summarise_option summarise,
              color_option color)
    {
        std::ifstream is(filename);
        load(is, threshold, summarise, color);
    }
    /// The actual transformation from time-of-flights to some image.
    unsigned compute(unsigned width, unsigned height, bool verbose = false);

    /// The actual transformation from time-of-flights to some image using x and y from the tofs extension.
    unsigned compute_from_centers(bool verbose = false);

    /// Computes the highest ratio of cos_corrections for all elements and draws them.
    void draw_max_cos_correction_deviance(unsigned width,
                                          unsigned height,
                                          std::string filename);

    /// Only draws the factors of cos-correction.
    void draw_cos_correction(unsigned width,
                             unsigned height,
                             unsigned up_to_element_e,
                             std::string filename);
    /// Helper function for cos-computation.
    double cos_correction_for_element(unsigned element, double x, double y);

    /// Color all cells with (most of the time) different colors for neighbouring cells.
    void compute_cells(unsigned width,
                       unsigned height,
                       bool with_diagonals = true);
    /// Computes Saft
    void compute_saft(unsigned width,
                      unsigned height,
                      const arr<>& measurement);

    config c;
    /// Contains the image after one compute-call.
    arr_2d<arr, double> intensities;

    /// Finds the representative pixels for all reflectors and put them in the queue for the Breadth-first-search.
    void populate_queue(
      std::variant<std::reference_wrapper<std::deque<pixel>>,
                   std::reference_wrapper<std::deque<tact_coordinate>>>
        populate_me);

    /// Setting to control the pixels with intensity 0.0.
    enum background_option
    {
        WHITE,
        NORMAL
    };

    /// Setting to enable/disable the colorscale.
    enum color_scale_option
    {
        ENABLED,
        DISABLED
    };

    /// Draw some image from the loaded cgdump.
    void draw(unsigned width,
              unsigned height,
              std::ostream& os,
              color_scale_option cso = ENABLED,
              background_option bo = NORMAL);

    /// Draw some image from the loaded cgdump. Needs to be called after compute().
    void draw(std::ostream& os,
              color_scale_option cso = ENABLED,
              background_option bo = NORMAL);

    /// Analyse the image from the loaded cgdump. Needs to be called after compute(). Stops after the top ,,max_pixels'' pixels.
    void analyze(std::ostream& os, unsigned max_pixels);

    using tof = std::vector<time_of_flight>;
    using val = std::vector<double>;

    /// The reflectors.
    tof tofs;
    /// The intensity from the reflectors as given by the reduced master.
    val vals;
    /// The index of the reflector in the cgdump.
    std::vector<unsigned> vals_index;
    /// True if a cgdump was loaded.
    bool loaded;
    /// True if intensities contains some image.
    bool computed;
    /// Enables the old and slow version that iterates over all pixels
    const bool old_version;
    /// The distance between 2 tacts. Normally 1, bigger if imprecisions are tolerated.
    const double overlapping_margin;

    const color_palette palette;

    /// Computes the cosine of the angle lying on the opposite side of c, in the triangle abc.
    static double cosinus_gamma(double a, double b, double c);
    /// Computes the length of the side lying on the opposite side of the cosine, in the triangle a bc.
    static double squared_length_of_c(double a, double b, double cosinus_gamma);

    /// Returns the coordinates of the middle of pixel (i,j) in m.
    void pixel_to_coords(unsigned i, unsigned j, double& x, double& y) const;

    /// True if coordinate (tacts_x,tacts_y) is inside all tof(t,t) and tof(t,t) + 1.
    bool reflector_in_tact_coords(const time_of_flight& tof,
                                  double tacts_x,
                                  double tacts_y) const;

    void draw_transducer(std::ostream& os,
                         unsigned image_width,
                         unsigned element_height) const;

    unsigned size_of_element_in_pixels() const;
};

/// The logic for the simplex-like algorithm that finds neighbour pixels that should be colored.
template<typename Func>
class BFS
{
  public:
    /// Does a breadth-first-search. Func filters invalid/uninteresting pixels out.
    BFS(std::deque<pixel>& startqueue, Func f, unsigned reflectors)
      : q(startqueue)
      , f(f)
    {
        colored_pixels.resize(reflectors);
        for (pixel& p : q) {
            //if (f(p)) {
            colored_pixels[p.reflector].insert(p);
            //}
        }
    }

    /// The used queue.
    std::deque<pixel>& q;
    /// Returns false for out-of-bounds pixels and pixels that should not be colored.
    Func f;
    /// colored_pixels[reflector] = all pixels colored by this reflector.
    std::vector<std::set<pixel>> colored_pixels;

    /// The actual search.
    void run()
    {
        while (!q.empty()) {
            const pixel current = q.front();
            pixel neighbour = current;
            q.pop_front();

            for (int a = -1; a <= 1; a++) {
                for (int b = -1; b <= 1; b++) {
                    if (a == 0 && b == 0) {
                        continue;
                    }
                    neighbour.x = current.x + a;
                    neighbour.y = current.y + b;

                    if (colored_pixels[neighbour.reflector].count(neighbour) ==
                          0 &&
                        f(neighbour)) {
                        colored_pixels[neighbour.reflector].insert(neighbour);
                        q.push_back(neighbour);
                    }
                }
            }
        }
    }
};

#endif // VISUALIZER_H
