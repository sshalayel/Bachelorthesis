#include "exception.h"

exception::exception(std::string s)
  : s(s)
{}

exception::~exception() {}

void
exception::raise()
{
    const bool classic_way = true;
    if (classic_way) {
        throw *this;
    } else {
        std::abort();
    }
}

const char*
exception::what() const throw()
{
    return s.c_str();
}
