#include "visualizer.h"

visualizer::visualizer(double overlapping_margin, bool old_version)
  : intensities(0, 0, nullptr)
  , loaded(false)
  , computed(false)
  , old_version(old_version)
  , overlapping_margin(overlapping_margin)
{}

void
visualizer::analyze(std::ostream& os, unsigned max)
{
    assert(computed);
    arr<unsigned> idx(intensities.dim1, intensities.dim2, intensities.dim3);
    for (unsigned i = 0; i < idx.size(); i++) {
        idx.data[i] = i;
    }

    //reverse sort the indexes : big values at the front
    std::sort(idx.data, idx.data + idx.size(), [this](unsigned i, unsigned j) {
        return intensities.data[i] > intensities.data[j];
    });

    const unsigned height = intensities.dim3;

    for (unsigned c = 0; c < max; c++) {
        unsigned current = idx.data[c];
        const unsigned i = current / height;
        const unsigned j = current % height;
        const double intensity = intensities.data[current];
        if (intensity < 0.01) {
            return;
        }
        double x, y;
        pixel_to_coords(i, j, x, y);
        os << "\nPixel "
           << "(" << i << ", " << j << ") with intensity " << intensity
           << " lies at (" << x << ", " << y << ").";
    }
}

void
visualizer::pixel_to_coords(unsigned i, unsigned j, double& x, double& y) const
{
    assert(loaded && intensities.data);
    const unsigned width = intensities.dim2;
    const unsigned height = intensities.dim3;

    x = i;
    x -= width / 2;
    x = x / (double)width;
    x *= c.max_distance_x();

    y = (double)j / (double)height;
    y *= c.max_distance_y();
}

bool
between(double a, double b, double middle)
{
    return (a <= middle && b >= middle) || (b <= middle && a >= middle);
}

bool
visualizer::reflector_in_tact_coords(const time_of_flight& tof,
                                     double x,
                                     double y) const
{
    constexpr double epsylon = 0.05;
    const double overlap = overlapping_margin / 2.0;
    for (unsigned s = 0; s < c.elements; s++) {
        const double distance_s = c.distance_in_tacts(x, y, s);
        const unsigned limit = only_diagonal ? s + 1 : c.elements;
        for (unsigned r = s; r < limit; r++) {
            // The current pixel defined by the distance of its center point
            const double distance_r = c.distance_in_tacts(x, y, r);

            const double small_radius = tof.at(s, r) - overlap;
            const double big_radius = tof.at(s, r) + overlap;
            const double distance = distance_r + distance_s;

            if (!between(
                  small_radius - epsylon, big_radius + epsylon, distance)) {
                return false;
            }
        }
    }
    return true;
}

void
visualizer::draw(unsigned width,
                 unsigned height,
                 std::ostream& os,
                 color_scale_option cso,
                 background_option bo)
{
    stop_watch sw;
    unsigned problematic_reflectors = compute(width, height, true);
    double compute_seconds = sw.elapsed(stop_watch::SET_TO_ZERO);

    if (problematic_reflectors > 0) {
        //      std::cout << "Found " << problematic_reflectors
        //                << " undrawable reflectors (out of " << tofs.size() << ")"
        //                << std::endl;
    }

    draw(os, cso, bo);
    double draw_seconds = sw.elapsed();

    std::cout << "Used " << compute_seconds << " seconds for computing and "
              << draw_seconds << " for drawing\n";
}

unsigned
visualizer::size_of_element_in_pixels() const
{
    unsigned e1, e2, j;
    //output probe-position
    c.tact_coords_to_pixel(intensities.dim2, intensities.dim3, 0, 0, e1, j);
    c.tact_coords_to_pixel(
      intensities.dim2, intensities.dim3, c.element_pitch_in_tacts(), 0, e2, j);
    return e2 - e1;
}

void
visualizer::draw_transducer(std::ostream& os,
                            unsigned image_width,
                            unsigned element_height) const
{
    //output probe-position
    std::vector<int> element_positions;
    for (unsigned e = 0; e < c.elements; e++) {
        const double x = c.element_pitch_in_tacts() * e;
        unsigned i, j;
        c.tact_coords_to_pixel(intensities.dim2, intensities.dim3, x, 0, i, j);
        element_positions.push_back(i);
    }
    unsigned element_widths = element_positions[1] - element_positions[0];
    if (element_widths == 0) {
        std::cerr << "Cannot draw probe-elements, resolution is too low!"
                  << std::endl;
        return;
    }
    element_widths /= 2;

    for (unsigned i = 0; i < element_height; i++) {
        for (unsigned j = 0; j < image_width; j++) {
            if (j < element_positions.front() - element_widths ||
                j > element_positions.back() + element_widths) {
                os << (unsigned char)0xFF;
                os << (unsigned char)0xFF;
                os << (unsigned char)0xFF;
            } else {
                bool ok = false;
                for (unsigned e = 0; e < element_positions.size(); e++) {
                    const unsigned current_e = element_positions.size() - 1 - e;
                    if (element_positions[current_e] - element_widths <= j) {
                        os << palette.red((double)current_e / c.elements);
                        os << palette.green((double)current_e / c.elements);
                        os << palette.blue((double)current_e / c.elements);
                        ok = true;
                        break;
                    }
                }
                if (!ok) {
                    os << (unsigned char)0xFF;
                    os << (unsigned char)0xFF;
                    os << (unsigned char)0xFF;
                }
            }
        }
    }
}

void
visualizer::draw(std::ostream& os, color_scale_option cso, background_option bo)
{
    assert(computed && "Please use compute or give me the Image size!");

    unsigned width = intensities.dim2;
    unsigned height = intensities.dim3;
    unsigned element_height = size_of_element_in_pixels();

    const unsigned color_reference_height = 20;

    //create Header
    os << "P6 " << std::to_string(width) << " "
       << std::to_string(element_height + height +
                         (cso == color_scale_option::ENABLED
                            ? 2 * color_reference_height
                            : 0))
       << " " << std::to_string(255) << '\n';

    draw_transducer(os, width, element_height);

    //output
    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            const double current_intensity = intensities.at(x, y);
            if (bo == background_option::WHITE && current_intensity == 0.0) {
                os << (unsigned char)0xFF << (unsigned char)0xFF
                   << (unsigned char)0xFF;
            } else {
                os << palette.red(current_intensity);
                os << palette.green(current_intensity);
                os << palette.blue(current_intensity);
            }
        }
    }

    //output colorscale
    if (cso == color_scale_option::ENABLED) {
        //some space before it
        for (unsigned i = 0; i < color_reference_height; i++) {
            for (unsigned x = 0; x < width; x++) {
                os << (char)0x00;
                os << (char)0x00;
                os << (char)0x00;
            }
        }
        for (unsigned i = 0; i < color_reference_height; i++) {
            for (unsigned x = 0; x < width; x++) {
                double actual = x;
                actual /= width;
                os << palette.red(actual);
                os << palette.green(actual);
                os << palette.blue(actual);
            }
        }
    }

    os.flush();
}

void
visualizer::populate_queue(
  std::variant<std::reference_wrapper<std::deque<pixel>>,
               std::reference_wrapper<std::deque<tact_coordinate>>> populate_me)
{
    simplexoid s{ 2.0 * c.element_pitch_in_tacts(), overlapping_margin / 2.0 };
    for (unsigned reflector = 0; reflector < tofs.size(); reflector++) {
        basis b;
        // x and y coordinate in tacts (not in meters)
        double x, y;
        if (s.run(tofs[reflector], b) && s.representant_to_coord(b, x, y)) {
            //std::cout << "Reflector has Basis " << b << " in coords (" << x
            //          << ", " << y << ")" << std::endl;
            if (auto* p = std::get_if<
                  std::reference_wrapper<std::deque<tact_coordinate>>>(
                  &populate_me)) {
                p->get().push_back({ x, y, reflector });
            } else if (auto* p =
                         std::get_if<std::reference_wrapper<std::deque<pixel>>>(
                           &populate_me)) {
                unsigned i, j;
                c.tact_coords_to_pixel(
                  intensities.dim2, intensities.dim3, x, y, i, j);
                p->get().push_back({ i, j, reflector });
            }
        }
    }
}

double
visualizer::cos_correction_for_element(unsigned element, double x, double y)
{
    /// X position of element.
    double x_a = x - element * c.element_pitch_in_tacts();

    // border cases : x = 0 => directly under the element
    if (x_a == 0) {
        return 1.0;
    }
    // border cases : y = 0 => neben dem transducer
    if (y == 0) {
        return 0.0;
    }
    /// c is the side beside to the needed cosine.
    double c_a = std::sqrt(x_a * x_a + y * y);
    assert_that(!std::isnan(c_a), "Cannot be NaN!");
    // Ankathete auf Hypothenuse
    double cos_a = y / c_a;
    return cos_a;
}

void
visualizer::draw_max_cos_correction_deviance(unsigned width,
                                             unsigned height,
                                             std::string filename)
{
    intensities.realloca(width, height);
    intensities.for_each([](double& d) { d = 1.0; });

    assert_that(loaded, "Please load a file before trying to draw it.");

    computed = true;

    double x, y;

    arr_2d<arr, double> values(c.elements, c.elements);
    arr_1d<arr, double> cosines(c.elements);

    const unsigned squared_roi_start = std::pow(c.get_roi_start() / 2.0, 2);

    for (unsigned i = 0; i < width; i++) {
        for (unsigned j = 0; j < height; j++) {
            c.pixel_to_tact_coords(
              intensities.dim2, intensities.dim3, i, j, x, y);
            ///ignore the pixels at the beginning.
            bool in_roi = true;
            for (unsigned e = 0; e < c.elements - 1; e++) {
                if (std::pow(x - e * c.element_pitch_in_tacts(), 2) + y * y <=
                    squared_roi_start) {
                    in_roi = false;
                    break;
                }
            }
            if (!in_roi) {
                continue;
            }
            for (unsigned element_a = 0; element_a < c.elements; element_a++) {
                cosines(element_a) =
                  cos_correction_for_element(element_a, x, y);
            }
            for (unsigned element_a = 0; element_a < c.elements; element_a++) {
                for (unsigned element_b = 0; element_b < c.elements;
                     element_b++) {
                    double cos_a = cosines(element_a);
                    double cos_b = cosines(element_b);

                    assert_that(!std::isnan(cos_a), "Cannot be NaN!");
                    assert_that(!std::isnan(cos_b), "Cannot be NaN!");

                    double correction = cos_a * cos_b;

                    values(element_a, element_b) = correction;
                }
            }
            //größter Verhältnis = max/min.
            double max, min;
            values.maxmin(max, min);

            // TODO: constant big/small enough?!
            intensities(i, j) = std::max(1.0, max / (min + 1e-8));
        }
    }

    double min, max;
    intensities.maxmin(max, min);
    std::cout << "Lowest ratio found : " << min
              << " VS highest ratio found : " << max << std::endl;

    //intensities.normalize_to_with_log(1.0);
    intensities.normalize_to(1.0);

    std::stringstream current_filename;
    current_filename << filename << "_max_cos_correction_ratio.pgm";
    std::ofstream current_file(current_filename.str());
    draw(current_file);
}

void
visualizer::draw_cos_correction(unsigned width,
                                unsigned height,
                                unsigned up_to_element_e,
                                std::string filename)
{
    intensities.realloca(width, height);
    assert_that(loaded, "Please load a file before trying to draw it.");

    computed = true;

    double x, y;

    for (unsigned element_a = 0; element_a < up_to_element_e; element_a++) {
        for (unsigned element_b = element_a; element_b < up_to_element_e;
             element_b++) {
            for (unsigned i = 0; i < width; i++) {
                for (unsigned j = 0; j < height; j++) {
                    c.pixel_to_tact_coords(
                      intensities.dim2, intensities.dim3, i, j, x, y);

                    double cos_a = cos_correction_for_element(element_a, x, y);
                    double cos_b = cos_correction_for_element(element_b, x, y);

                    assert_that(!std::isnan(cos_a), "Cannot be NaN!");
                    assert_that(!std::isnan(cos_b), "Cannot be NaN!");

                    double correction = cos_a * cos_b;

                    intensities(i, j) = correction;
                }
            }
            //intensities.normalize_to(1.0); // cosine always lie in [0,1].
            std::stringstream current_filename;
            current_filename << filename << "cos_correction_for_elements_"
                             << element_a << "_" << element_b << ".pgm";
            std::ofstream current_file(current_filename.str());
            draw(current_file);
        }
    }
}

void
visualizer::compute_cells(unsigned width, unsigned height, bool with_diagonal)
{
    auto pixel_to_tof = [&](unsigned i, unsigned j, time_of_flight& tof) {
        double x, y;
        c.pixel_to_tact_coords(width, height, i, j, x, y);
        c.tact_coords_to_tof(x, y, tof);
    };
    compute_all_cells cac(
      pixel_to_tof, width, height, c.elements, c.get_roi_end());
    cac.compute_all(intensities, with_diagonal);
    intensities.normalize_to(1.0);
    computed = true;
}

void
visualizer::compute_saft(unsigned width,
                         unsigned height,
                         const arr<>& measurement)
{
    saft{ width, height, c }.compute(measurement, intensities);
    computed = true;
}

unsigned
visualizer::compute_from_centers(bool verbose)
{
    unsigned problematic_reflectors = 0;
    assert(loaded && "Please load some Data before computing/drawing");

    /// 1 pixel ≃ 1mm.
    unsigned width = std::ceil(c.tacts_to_meter(c.max_distance_x()) * 1000);
    unsigned height = std::ceil(c.tacts_to_meter(c.max_distance_y()) * 1000);

    intensities.realloca(width, height);
    std::fill(intensities.data, intensities.data + intensities.size(), 0);

    std::vector<unsigned> undrawable;
    std::vector<bool> found(tofs.size(), false);

    unsigned i, j;
    double x, y;

    for (unsigned reflector = 0; reflector < tofs.size(); reflector++) {
        if (auto& extension = tofs[reflector].extension) {
            x = std::floor(*assert_that(tofs[reflector].representant_x,
                                        "Expected a x for extended ToFs!"));
            y = std::floor(extension->y);
            //i = std::floor(x + width / 2);
            //j = std::floor(y);
            c.tact_coords_to_pixel(
              intensities.dim2, intensities.dim3, x / 2.0, y / 2.0, i, j);
            intensities.at(i, j) += vals[reflector];
            found[reflector] = true;
        } else {
            if (verbose) {
                std::cerr << "Using a ToF without extension in the visu "
                             "needing x and y!"
                          << std::endl;
            }
        }
    }
    problematic_reflectors = std::count(found.begin(), found.end(), false);

    for (unsigned i = 0; i < tofs.size(); i++) {
        if (!found[i]) {
            undrawable.push_back(i);
        }
    }

    if (verbose && !undrawable.empty()) {
        std::cerr << "Warning: Cannot draw reflectors ";
        for (unsigned n : undrawable) {
            std::cerr << vals_index[n] << ", ";
        }
        std::cerr << std::endl;
    }

    assert((intensities.normalize_to(1.0) || problematic_reflectors > 0 ||
            tofs.size() == 0) &&
           "No problematic_reflectors but image still empty?");
    computed = true;
    return problematic_reflectors;
}

unsigned
visualizer::compute(unsigned width, unsigned height, bool verbose)
{
    unsigned problematic_reflectors = 0;
    assert(loaded && "Please load some Data before computing/drawing");

    if (verbose && (width < 2 * c.get_roi_end() || height < c.get_roi_end())) {
        computed = true;
        std::cerr << "Warning : resolution might be too low!" << std::endl;
    }

    intensities.realloca(width, height);
    std::fill(intensities.data, intensities.data + intensities.size(), 0);

    std::vector<unsigned> undrawable;
    if (old_version) {
        std::vector<bool> found(tofs.size(), false);

        for (unsigned i = 0; i < width; i++) {
            for (unsigned j = 0; j < height; j++) {
                double intensity = 0.0;
                double x, y;
                c.pixel_to_tact_coords(
                  intensities.dim2, intensities.dim3, i, j, x, y);

                for (unsigned reflector = 0; reflector < tofs.size();
                     reflector++) {
                    if (reflector_in_tact_coords(tofs[reflector], x, y)) {
                        intensity = vals[reflector];
                        found[reflector] = true;
                        //std::cout << "One reflector found in pixel (" << i
                        //<< ", " << j << ") with tact coords (" << x
                        //<< ", " << y << ")" << std::endl;
                    }
                }
                intensities.at(i, j) = intensity;
            }
        }
        problematic_reflectors = std::count(found.begin(), found.end(), false);

        for (unsigned i = 0; i < tofs.size(); i++) {
            if (!found[i]) {
                undrawable.push_back(i);
            }
        }
    } else {
        std::deque<pixel> q;
        populate_queue(q);

        BFS bfs{ q,
                 [&](pixel p) -> bool {
                     double x, y;
                     c.pixel_to_tact_coords(
                       intensities.dim2, intensities.dim3, p.x, p.y, x, y);
                     bool ok =
                       p.x < width && p.y < height &&
                       reflector_in_tact_coords(this->tofs[p.reflector], x, y);
                     if (ok) {
                         //std::cout << "One reflector found in pixel (" << p.x
                         //<< ", " << p.y << ") with tact coords (" << x
                         //<< ", " << y << ")" << std::endl;
                     }
                     return ok;
                 },
                 (unsigned)tofs.size() };
        bfs.run();

        for (unsigned reflector = 0; reflector < bfs.colored_pixels.size();
             reflector++) {
            const std::set<pixel>& reflector_pixels =
              bfs.colored_pixels[reflector];

            if (reflector_pixels.empty()) {
                problematic_reflectors++;
                undrawable.push_back(reflector);
            } else {
                for (const pixel& p : reflector_pixels) {
                    intensities.at(p.x, p.y) = vals[p.reflector];
                    double x, y;
                    c.pixel_to_tact_coords(
                      intensities.dim2, intensities.dim3, p.x, p.y, x, y);
                    //std::cout << "One reflector found in pixel (" << p.x << ", "
                    //          << p.y << ") with tact coords (" << x << ", " << y
                    //          << ") and intensity " << vals[p.reflector]
                    //          << std::endl;
                }
            }
        }
    }

    if (verbose && !undrawable.empty()) {
        std::cerr << "Warning: Cannot draw reflectors ";
        for (unsigned n : undrawable) {
            std::cerr << vals_index[n] << ", ";
        }
        std::cerr << std::endl;
    }

    assert((intensities.normalize_to(1.0) || problematic_reflectors > 0 ||
            tofs.size() == 0) &&
           "No problematic_reflectors but image still empty?");
    computed = true;
    return problematic_reflectors;
}

void
visualizer::load(std::istream& is,
                 double threshold,
                 summarise_option summarise,
                 color_option random_colors)
{
    tofs.clear();
    vals.clear();

    tof t;
    val v;

    c.load(is, { t }, { v });

    auto comp = [&](const unsigned a, const unsigned b) {
        bool ret = std::lexicographical_compare(tofs[a].diagonal_begin(),
                                                tofs[a].diagonal_end(),
                                                tofs[b].diagonal_begin(),
                                                tofs[b].diagonal_end());
        return ret;
    };
    unsigned total_summarised = 0;
    unsigned total = 0;
    unsigned total_threshold = 0;

    std::set<unsigned, decltype(comp)> summarise_diagonal(comp);
    const unsigned seed =
      std::chrono::system_clock::now().time_since_epoch().count();
    auto dice = std::bind(std::uniform_real_distribution(0.0, 1.0),
                          std::default_random_engine(seed));
    if (summarise == summarise_option::SUMMARISE) {
        for (unsigned i = 0; i < t.size(); i++) {
            total++;
            if (v[i] < threshold) {
                total_threshold++;
                continue;
            }
            tofs.push_back(std::move(t[i]));
            auto current = summarise_diagonal.find(tofs.size() - 1);
            if (current != summarise_diagonal.end()) {
                /// Summarise into existing ToF.
                vals[*current] += v[i];
                tofs.pop_back();
                total_summarised++;
            } else {
                switch (random_colors) {
                    case RANDOM_COLORS: {
                        vals.push_back(dice());
                        break;
                    }
                    case NORMAL_COLORS: {
                        vals.push_back(v[i]);
                        break;
                    }
                    default: {
                        assert_that(false, "Missing case?");
                        break;
                    }
                }
                vals_index.push_back(i);
                summarise_diagonal.insert(tofs.size() - 1);
            }
        }
    } else {
        for (unsigned i = 0; i < t.size(); i++) {
            total++;
            if (v[i] < threshold) {
                total_threshold++;
                continue;
            }
            switch (random_colors) {
                case RANDOM_COLORS: {
                    vals.push_back(dice());
                    break;
                }
                case NORMAL_COLORS: {
                    vals.push_back(v[i]);
                    break;
                }
                default: {
                    assert_that(false, "Missing case?");
                    break;
                }
            }
            vals_index.push_back(i);
            tofs.push_back(std::move(t[i]));
        }
    }

    std::cout << "Read " << tofs.size() << " ToFs from " << total << ". "
              << total_threshold << " were thresholt and " << total_summarised
              << " were summarised." << std::endl;

    //small normalisation step
    arr_1d<arr, double>(vals.size(), vals.data(), false).normalize_to(1.0);

    /// sort for visu when using warm_start and warm_start_values.
    std::vector<unsigned> indexes(tofs.size());
    std::iota(indexes.begin(), indexes.end(), 0);

    /// ascending order for visu.
    std::sort(
      indexes.begin(),
      indexes.end(),
      [&](const unsigned& a, const unsigned& b) { return vals[a] < vals[b]; });

    std::vector<double> local_vals;
    std::vector<time_of_flight> local_tofs;

    for (unsigned idx : indexes) {
        local_vals.push_back(vals[idx]);
        local_tofs.push_back(std::move(tofs[idx]));
    }

    tofs = std::move(local_tofs);
    vals = std::move(local_vals);

    loaded = true;
    computed = false;
}

/// Trying to simulate the Matlab JET colour palette.
/// You can find it here : https://blogs.mathworks.com/images/loren/73/colormapManip_14.png
/// In the image, colors in the left represent small values, while colors in the right represent big values.
/// color_palette::color_palette()
///   : color_palette({ 0, 0, 0, 0, 0x7f, 0xff, 0xff, 0xff, 0xff },
///                   { 0, 0, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0, 0 },
///                   { 0x7f, 0xff, 0xff, 0xff, 0x7f, 0, 0, 0, 0xff })
/// {}
/// The actual color_palette is the one taken from civa.

/// Trying to emulate the CIVA colour palette.
///color_palette::color_palette()
///  : color_palette({ 0x55, 0xff, 0xaa, 0xaa, 0xff, 0x7f, 0x00, 0x00 },
///                  { 0x55, 0xff, 0x55, 0x00, 0x00, 0x00, 0x00, 0xff },
///                  { 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff })
///{}

/// Pudovikovs colors
color_palette::color_palette()
  : color_palette(
      {
        0xff, 0xe5, 0xcc, 0xb2, 0x99, 0x7f, 0x66, 0x4c, 0x33, 0x19,
        0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      },
      {
        0xff, 0xe5, 0xcc, 0xb2, 0x99, 0x7f, 0x66, 0x4c, 0x33, 0x19,
        0x00, 0xc0, 0x7f, 0x40, 0x2d, 0x23, 0x17, 0x0c, 0x00,
      },
      {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      })
{}

color_palette::color_palette(colors&& reds, colors&& greens, colors&& blues)
  : reds(std::move(reds))
  , greens(std::move(greens))
  , blues(std::move(blues))
{}

color_palette::color
color_palette::red(double intensity) const
{
    return helper(intensity, reds);
}

color_palette::color
color_palette::green(double intensity) const
{
    return helper(intensity, greens);
}

color_palette::color
color_palette::blue(double intensity) const
{
    return helper(intensity, blues);
}

color_palette::color
color_palette::helper(double intensity, const colors& colors) const
{
    assert(intensity >= 0 && intensity <= 1);
    double h = intensity * (colors.size() - 1);
    return interpolation::linear(
      colors[std::floor(h)], colors[std::ceil(h)], h);
}
