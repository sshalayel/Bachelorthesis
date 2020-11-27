#include "optlib/config.h"
#include "optlib/coordinates.h"
#include "optlib/iterator.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <string>

int
main(int argc, char** argv)
{
    for (int i = 1; i < argc; i++) {
        config c;
        std::vector<time_of_flight> warm_start;
        std::vector<double> warm_start_values;
        std::ifstream input(argv[i]);
        c.load(input, warm_start, warm_start_values);

        auto warm_start_it = container_input_iterator{ warm_start };
        c.dump_human_readable(std::cout, warm_start_it, warm_start_values, {});
    }
}
