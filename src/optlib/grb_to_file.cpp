#include "grb_to_file.h"

grb_to_file::grb_to_file(std::ostream& os, std::string prefix)
  : os(os)
  , prefix(prefix)
{}

grb_to_file::~grb_to_file() {}

void
grb_to_file::callback()
{
    if (this->where == GRB_CB_MESSAGE) {
        std::stringstream ss;
        ss << this->getStringInfo(GRB_CB_MSG_STRING);
        std::string current_line;
        while (std::getline(ss, current_line)) {
            os << prefix << current_line << "\n";
        }
        os.flush();
    }
}

slow_grb_to_file::slow_grb_to_file(std::ostream& os)
  : grb_to_file(os)
{}

slow_grb_to_file::~slow_grb_to_file() {}

void
slow_grb_to_file::callback()
{
    grb_to_file::callback();
    std::this_thread::yield();
}
