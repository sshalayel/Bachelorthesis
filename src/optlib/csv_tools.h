#ifndef CSV_TOOLS_H
#define CSV_TOOLS_H

#include <iostream>
#include <limits>

/// Extracts a semicolon from the stream.
void
semi(std::istream& is);

/// Discards all characters and stops after finding (and removing) a newline.
void
nextline(std::istream& is);

/// Discards all lines starting with #
void
skip_comments(std::istream& is);

/// Exception when files are not ok.
class read_exception : public std::exception
{
  public:
    read_exception(std::string s);
    virtual const char* what() const throw() override;
    std::string s;
};

/// Make sure that the last extraction operation was successful and throw read_exception if not.
template<typename T>
void
assert_read(T&& should_be_true, std::string message)
{
    if (!(bool)should_be_true) {
        throw read_exception(message);
    }
}

/// Make sure that the last extraction operation was successful and throw read_exception if not.
template<typename T>
T&
assert_read(T& should_be_true, std::string message)
{
    if (!(bool)should_be_true) {
        throw read_exception(message);
    }
    return should_be_true;
}

/// Extracts some element T followed by some semicolon from stream.
template<typename T>
void
extract_with_optional_semicolon(std::istream& is, T& field)
{
    is >> field;
    if (is.peek() == ';') {
        semi(is);
    }
}

/// Extracts some element T followed by some semicolon from stream.
template<typename T>
void
extract_with_semicolon(std::istream& is, T& field)
{
    is >> field;
    semi(is);
}

/// Inserts some element T followed by some semicolon from stream.
template<typename T>
void
insert_with_semicolon(std::ostream& os, T&& field)
{
    os << field << ';';
}

#endif
