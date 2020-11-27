#include "optlib/arr.h"
#include "optlib/config.h"
#include "optlib/reader.h"
#include <iostream>

int
main(int argc, char** argv)
{

    //batch mode
    for (int i = 1; i < argc; i++) {
        config solutions;
        std::vector<time_of_flight> tofs;
        std::vector<double> tof_values;
        std::ifstream is(argv[i]);

        solutions.load(is, tofs, tof_values);

        arr_1d<arr, double> reference(solutions.reference_samples);
        arr<double> recreated(
          solutions.elements, solutions.elements, solutions.get_roi_length());

        std::fill(recreated.data, recreated.data + recreated.size(), 0.0);

        reader::read(solutions.reference_file, reference);

        double max, min;
        reference.maxmin(max, min);
        double limit = (max - min) / 100.0;
        arr_1d<arr, double> trimmed_reference(0, nullptr, false);
        reference.trim(-limit, limit, trimmed_reference);
        trimmed_reference.scale_to(100.0);

        for (unsigned reflector = 0; reflector < tofs.size(); reflector++) {
            time_of_flight& tof = tofs[reflector];
            for (unsigned s = 0; s < tof.senders; s++) {
                for (unsigned r = 0; r < tof.receivers; r++) {
                    for (unsigned k = 0; k < trimmed_reference.dim3; k++) {
                        int shift = tof.at(s, r) + k - solutions.get_roi_start();
                        assert(shift >= 0);
                        if ((unsigned)shift >= recreated.dim3) {
                            break;
                        }
                        recreated.at(s, r, shift) +=
                          tof_values[reflector] * trimmed_reference.at(s, r, k);
                    }
                }
            }
        }

        std::string out = argv[i];
        out += ".bin";
        std::ofstream os(out);
        os.write((char*)recreated.data,
                 recreated.size() * sizeof(double) / sizeof(char));
    }
    return 0;
}