#include "optlib/reader.h"
#include "optlib/visualizer.h"
#include <getopt.h>

const struct option options[] = {
    { "help", no_argument, nullptr, 'h' },
    { "width", required_argument, nullptr, 'x' },
    { "height", required_argument, nullptr, 'y' },
    { "file", required_argument, nullptr, 'f' },
    { "margin", required_argument, nullptr, 'm' },
    { "output", required_argument, nullptr, 'o' },
    { "threshold", required_argument, nullptr, 't' },
    { "cells", no_argument, nullptr, 'c' },
    { "circlecells", no_argument, nullptr, 'C' },
    { "hints", no_argument, nullptr, 'H' },
    { "tex", no_argument, nullptr, 'l' },
    { "linetex", no_argument, nullptr, 'L' },
    { "saft", required_argument, nullptr, 'S' },
    { "cos_correction", required_argument, nullptr, 'q' },
    { "cos_correction_deviance", no_argument, nullptr, 'd' },
    { "white_background", no_argument, nullptr, 'w' },
    { "old", no_argument, nullptr, 'O' },
    { "no_summarise", no_argument, nullptr, 's' },
    { "random_colors", no_argument, nullptr, 'r' },
    { "center", no_argument, nullptr, 'T' },
    { "pgm_center", no_argument, nullptr, 'p' },
    { "roi_start", required_argument, nullptr, '!' },
    { "roi_end", required_argument, nullptr, '@' },
    { 0, 0, 0, 0 },
};
const char* short_options = "hx:y:f:o:m:t:OSLlcC";

int
main(int argc, char** argv)
{
    unsigned width = 3000, height = 1500;
    std::string file;
    std::string output;
    double margin = 1.0;
    double threshold = 1e-2;
    bool cells = false;
    bool circlecells = false;
    bool old = false;
    bool tex = false;
    bool linetex = false;
    std::optional<double> saft;
    std::optional<unsigned> roi_start;
    std::optional<unsigned> roi_end;
    bool hints = false;
    unsigned cos_correction = 0;
    bool cos_correction_deviance = false;
    bool center = false;
    bool pgm_center = false;
    visualizer::background_option white_background =
      visualizer::background_option::NORMAL;
    visualizer::summarise_option summarise = visualizer::SUMMARISE;
    visualizer::color_option random_colors = visualizer::NORMAL_COLORS;

    char current;
    while ((current =
              getopt_long(argc, argv, short_options, options, nullptr)) != -1) {
        switch (current) {
            case '!': {
                roi_start = std::stoul(optarg);
                break;
            }
            case '@': {
                roi_end = std::stoul(optarg);
                break;
            }
            case 'T': {
                center = true;
                summarise = visualizer::DO_NOT_SUMMARISE;
                break;
            }
            case 'r': {
                random_colors = visualizer::color_option::RANDOM_COLORS;
                break;
            }
            case 'q': {
                cos_correction = std::stoul(optarg);
                break;
            }
            case 'w': {
                white_background = visualizer::background_option::WHITE;
                break;
            }
            case 's': {
                summarise = visualizer::DO_NOT_SUMMARISE;
                break;
            }
            case 'd': {
                cos_correction_deviance = true;
                break;
            }
            case 'm': {
                margin = std::stod(optarg);
                break;
            }
            case 'c': {
                cells = true;
                break;
            }
            case 'C': {
                circlecells = true;
                break;
            }
            case 'x': {
                width = std::stoi(optarg);
                break;
            }
            case 'y': {
                height = std::stoi(optarg);
                break;
            }
            case 'f': {
                file = optarg;
                break;
            }
            case 'o': {
                output = optarg;
                break;
            }
            case 't': {
                threshold = std::stod(optarg);
                break;
            }
            case 'O': {
                old = true;
                break;
            }
            case 'L': {
                linetex = true;
                break;
            }
            case 'l': {
                tex = true;
                break;
            }
            case 'H': {
                hints = true;
                break;
            }
            case 'S': {
                saft = std::stod(optarg);
                break;
            }
            case 'p': {
                pgm_center = true;
                break;
            }
            case 'h':
            default: {
                int ret = 0;
                if (current != 'h') {
                    std::cerr << "Unrecognized Option!\n";
                    ret = 1;
                }
                std::cout << "Possible options:\n";
                unsigned x = 1; //number of args without options.
                const struct option* current = options;
                while (current < options + x) {
                    std::cout << "--" << current->name << "\n";
                    current++;
                }
                while (current->name != 0) {
                    std::cout << "--" << current->name << " value\n";
                    current++;
                }
                return ret;
            }
        }
    }

    if ((file.empty() || output.empty()) && optind == argc) {
        std::cout
          << "Need filenames for the input parameter file and output image!"
          << std::endl;
        return 1;
    }

    visualizer v(margin, old);

    //batch mode
    for (int i = optind; i < argc; i++) {
        std::string in = argv[i];
        v.load(in, threshold, summarise, random_colors);
        if (roi_start) {
            v.c.roi_start = *roi_start;
        }
        if (roi_end) {
            v.c.roi_end = *roi_end;
        }
        auto _max = std::max_element(v.vals.begin(), v.vals.end());

        //avoid division by 0
        double max = (_max == v.vals.end() || *_max == 0) ? 1 : *_max;

        if (cos_correction > 0) {
            v.draw_cos_correction(
              width, height, cos_correction, output.empty() ? in : output);
            continue;
        } else if (cos_correction_deviance) {
            v.draw_max_cos_correction_deviance(
              width, height, output.empty() ? in : output);
            continue;
        } else if (cells || circlecells) {
            std::ofstream s2(output.empty() ? (in + "cells.pgm") : output);
            v.compute_cells(width, height, !circlecells);
            v.draw(
              s2, visualizer::color_scale_option::DISABLED, white_background);
            s2.flush();
        } else {
            std::string name = output.empty() ? in : output;
            if (saft) {
                arr<> measurement(v.c.elements, v.c.elements, v.c.samples);
                reader::read(v.c.measurement_file, measurement);
                measurement.scale_to(100);

                v.compute_saft(width, height, measurement);
                v.intensities.threshold_to(*saft);

                std::string saft_name =
                  name + ".saft" + std::to_string(*saft) + ".pgm";
                std::ofstream s(saft_name);

                v.draw(
                  s, visualizer::color_scale_option::ENABLED, white_background);
            }

            auto do_tex = [&](bool with_lines, bool center) {
                std::string current_name = name + (center ? ".center" : "") +
                                           (with_lines ? ".lines.tex" : ".tex");
                std::ofstream s(current_name);

                std::unique_ptr<tikz_tof_printer> p =
                  center
                    ? std::make_unique<tikz_center_printer>(
                        s,
                        v.c.tacts_to_meter(v.c.max_distance_x()), //width
                        v.c.tacts_to_meter(v.c.max_distance_y()), //height
                        v.c.pitch,
                        v.c.tacts_to_meter(1.0), //tact_in_m
                        v.c.elements,
                        //radius of smallest reflectors as we cannot resolve over one tact.
                        //v.c.wave_length * v.c.wave_frequency() / v.c.sampling_rate / 2.0)
                        //v.c.tacts_to_meter(1.0) / 2.0)
                        0.1e-3) // 0.1 mm
                    : std::make_unique<tikz_tof_printer>(
                        s,
                        v.c.tacts_to_meter(v.c.max_distance_x()), //width
                        v.c.tacts_to_meter(v.c.max_distance_y()), //height
                        v.c.pitch,
                        v.c.tacts_to_meter(1.0), //tact_in_m
                        v.c.elements);

                for (unsigned i = 0; i < v.tofs.size(); i++) {
                    p->add_tof(v.tofs[i],
                               v.palette.red(v.vals[i] / max),
                               v.palette.green(v.vals[i] / max),
                               v.palette.blue(v.vals[i] / max),
                               with_lines);
                }
                if (hints) {
                    std::deque<tact_coordinate> hints;
                    v.populate_queue(hints);
                    for (auto hint : hints) {
                        p->add_reflector_hint(
                          hint,
                          v.palette.red(v.vals[hint.reflector] / max),
                          v.palette.green(v.vals[hint.reflector] / max),
                          v.palette.blue(v.vals[hint.reflector] / max));
                    }
                }
            };
            if (center) {
                do_tex(false, true);
            }
            if (tex) {
                do_tex(false, false);
            }
            if (linetex) {
                do_tex(true, false);
            }

            if (pgm_center) {
                std::ofstream s2(name + ".pgm_center.pgm");
                v.compute_from_centers(!circlecells);
                v.draw(s2,
                       visualizer::color_scale_option::ENABLED,
                       white_background);
                s2.flush();
            }

            if (old) {
                name += ".old";
            }
            if (!tex && !linetex) {
                name += ".pgm";
                std::ofstream s(name);
                v.draw(width,
                       height,
                       s,
                       visualizer::color_scale_option::ENABLED,
                       white_background);
            }
        }
    }

    return 0;
}