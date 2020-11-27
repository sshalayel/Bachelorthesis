#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <cstdlib>
#include <exception>
#include <string>
#include <utility>

/// An exception that can be constructed from an error message.
struct exception : std::exception
{
    /// The error message.
    std::string s;
    exception(std::string s);
    /// Return the error message.
    const char* what() const throw() override;
    /// Throw this exception. Can be used to control if program should be aborted immediately.
    void raise();
    ~exception() override;
};

/// A self-made assertion, returns its input for further computation.
template<typename Condition>
Condition
assert_that(Condition&& c, std::string error_message)
{
    if (!(bool)c) {
        exception(error_message).raise();
    }
    return c;
}

/// A self-made assertion, returns its input for further computation.
template<typename Condition>
Condition&
assert_that(Condition& c, std::string error_message)
{
    if (!(bool)c) {
        exception(error_message).raise();
    }
    return c;
}

#endif
