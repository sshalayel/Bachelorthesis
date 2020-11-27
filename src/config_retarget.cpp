#include "optlib/arr.h"
#include "optlib/cgdump_analyser.h"
#include "optlib/config.h"
#include "optlib/reader.h"
#include <fstream>
#include <getopt.h>
#include <iostream>

const struct option options[] = {
    { "help", no_argument, nullptr, 'h' },
    { "to", required_argument, nullptr, 't' },
    { 0, 0, 0, 0 },
};

const char* short_options = "ht:";

int
main(int argc, char** argv)
{
    config target_to;
    bool target_to_loaded;

    char current;

    while ((current =
              getopt_long(argc, argv, short_options, options, nullptr)) != -1) {
        switch (current) {
            case 't': {
                std::ifstream is(optarg);
                target_to_loaded = target_to.load(is, {}, {});
                break;
            }
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
    assert_that(target_to_loaded,
                "Please specify the target config using --to config.csv!");

    for (int i = optind; i < argc; i++) {
        std::string in = argv[i];
        std::ifstream is(in);
        config c;
        std::vector<time_of_flight> tofs;
        std::vector<double> vals;
        c.load(is, tofs, vals);

        //target_to.retarget_tofs_from(c, container_input_iterator{ tofs });
        cgdump_analyser_from_xy analyser(target_to);
        for (time_of_flight& tof : tofs) {
            double scale = (double)c.wave_speed / target_to.wave_speed;
            std::cout << "scale is " << scale << std::endl;
            // scale coordinates as tacts changes sizes
            assert_that(tof.extension, "Expected extended ToF!");
            assert_that(tof.representant_x, "Expected extended ToF!");
            *tof.representant_x *= scale;
            tof.extension->y *= scale;

            analyser.rewrite(tof, in + ".infeasible_retarget.");
        }

        std::ofstream os(in + ".retargeted.cgdump");
        container_input_iterator cii{ tofs };
        target_to.save(os, cii, vals);
    }
    return 0;
}
