#include "printer.h"

ofstream_with_dirs::ofstream_with_dirs(const std::string& file)
  : std::ofstream(open_file(file))
{}
const std::string&
ofstream_with_dirs::open_file(const std::string& file)
{
    std::size_t end_dir = file.rfind('/');
    if (end_dir != std::string::npos) {
        std::string dir = file.substr(0, end_dir);
        if (!fs::is_directory(dir) ||
            !fs::exists(dir)) {        // Check if src folder exists
            fs::create_directory(dir); // create src folder
        }
    }
    return file;
}

simple_printer::simple_printer(std::ostream& os, unsigned arr_size_limit)
  : os(os)
  , owner(false)
  , arr_size_limit(arr_size_limit)
{}

simple_printer::simple_printer(std::string& filename, unsigned arr_size_limit)
  : os(*new ofstream_with_dirs(filename))
  , owner(true)
  , arr_size_limit(arr_size_limit)
{}

simple_printer::~simple_printer()
{
    if (owner) {
        delete &os;
    }
}

printer::~printer() {}

void
simple_printer::log(const std::string message)
{
    //flush = (flush + 1) % flush_rate;
    //if (flush == 0) {
    //    os.flush();
    //}
    const char* header = " ==== ";
    os << header << message << header << '\n';
    os.flush();
}

void
simple_printer::log(const std::string message, const arr<unsigned>& dump_me)
{
    log(message);
    if (dump_me.size() < arr_size_limit) {
        os << dump_me;
    }
    os.flush();
}

void
simple_printer::log(const std::string message, const arr<>& dump_me)
{
    log(message);
    if (dump_me.size() < arr_size_limit) {
        os << dump_me;
    }
    os.flush();
}

void
simple_printer::raw(const std::vector<time_of_flight>& data,
                    const std::vector<double>& value)
{
    assert(data.size() == value.size() &&
           "Every Entry in data should have a value!");
    for (unsigned i = 0; i < data.size(); i++) {
        os << "Reflector with amplitude ";
        os << value[i];
        os << " at Position ";
        data[i].dump(os);
    }
    os.flush();
}
void
simple_printer::converted(const std::vector<cartesian<>>& data,
                          const std::vector<double>& value)
{
    assert(false && "Implement Me!");
}

muted_printer::~muted_printer() {}

void
muted_printer::log(const std::string message, const arr<unsigned>& dump_me)
{}

void
muted_printer::log(const std::string message, const arr<>& dump_me)
{}

void
muted_printer::log(const std::string message)
{}

void
muted_printer::raw(const std::vector<time_of_flight>& data,
                   const std::vector<double>& value)
{}

void
muted_printer::converted(const std::vector<cartesian<>>& data,
                         const std::vector<double>& value)
{}
