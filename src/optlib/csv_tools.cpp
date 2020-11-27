#include "csv_tools.h"

read_exception::read_exception(std::string s)
  : s(s)
{}

const char*
read_exception::what() const throw()
{
    return s.data();
}

void
semi(std::istream& is)
{
    assert_read(is, "Failure! Could not read!");
    assert_read(is.get() == ';', "Should contain some ;");
}

void
nextline(std::istream& is)
{
    assert_read(is, "Failure! Could not read!");
    is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void
skip_comments(std::istream& is)
{
    while (is.peek() == '#') {
        nextline(is);
    }
}

template<>
void
extract_with_semicolon<double>(std::istream& is, double& d)
{
    /// Simply doing `is >> d` is not enough, as -nan cannot be parsed :(.
    std::string line;
    std::getline(is, line, ';');
    d = std::stod(line);
}

template<>
void
extract_with_semicolon<std::string>(std::istream& is, std::string& s)
{
    std::getline(is, s, ';');
}
