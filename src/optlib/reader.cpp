#include "reader.h"

void
reader::read(std::string filename, arr<>& out)
{
    unsigned ext_start = filename.rfind('.');
    std::string extension = filename.substr(ext_start + 1);
    if (extension == "bin") {
        binary_reader<double>().run(filename, out);
        return;
    }
    if (extension == "csv") {
        civa_txt_reader().run(filename, out);
        return;
    }
    throw new std::logic_error("Unknown file extension " + extension + " !");
}
bool
reader::run(std::istream& str, arr<>& out)
{
    dim1 = out.dim1;
    dim2 = out.dim2;
    dim3 = out.dim3;
    return this->decode(str, out);
}

bool
reader::run(arr<>& out)
{
    // if pointer != null and (bool) istr is true
    if (last_istr && *last_istr) {
        this->decode(*last_istr, out);
        return true;
    } else {
        return false;
    }
}

bool
reader::run(const std::string& str, arr<>& out)
{
    std::ifstream istr(str);
    return this->run(istr, out);
}

reader::reader()
  : last_istr(nullptr)
{}
reader::~reader() {}

civa_txt_reader::civa_txt_reader() {}

civa_txt_reader::~civa_txt_reader() {}

bool
civa_txt_reader::decode(std::istream& istr, arr<>& data)
{
    std::string current;

    double d;
    char newline;
    // loop over the actual data
    for (unsigned i = 0; i < data.dim3; i++) {
        // the table has 3 columns : x-Axis, y-Axis (in db), y-Axis (normalized
        // to 100)
        // x-Axis
        // std::cout << "ignore :";
        while (std::getline(istr, current, ';') &&
               (current[0] > '9' || current[0] < '0')) {
            // ignore line with text
            std::getline(istr, current);
            if (!istr) {
                return false;
            }
            // std::cout << current << " ";
        }
        // std::cout << "\n";
        // y-Axis (in db)
        // std::cout << "x_axis: " << current << '\n';
        std::getline(istr, current, ';');
        if (!istr) {
            return false;
        }
        // std::cout << "y_axis(db): " << current << '\n';
        // y-Axis (normalized to 100)
        std::getline(istr, current, ';');
        if (!istr) {
            return false;
        }
        // std::cout << "y_axis(n): " << current << '\n';

        // remove everything after
        do {
            istr.read(&newline, 1);
            if (!istr) {
                return false;
            }
        } while (newline != '\n');

        d = std::stod(current);
        assert_that(!std::isnan(d), "Found a NaN in a CSV!");
        data(0, 0, i) = d;
    }
    return true;
}
