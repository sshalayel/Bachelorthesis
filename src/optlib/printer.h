#ifndef PRINTER_H
#define PRINTER_H
#include "coordinates.h"
#include <fstream>

#ifdef PANDORA
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

/// Helper class that emulates an ofstream but creates directories if needed.
class ofstream_with_dirs : public std::ofstream
{
  protected:
    /// Creates the needed directories and returns file.
    static const std::string& open_file(const std::string& file);

  public:
    /// Calls std::ofstream constructor with open_file.
    ofstream_with_dirs(const std::string& file);
};

/// Used for logging events during column_generation.
class printer
{
  public:
    /// Output log messages.
    void print_log(const std::string message, const arr<unsigned>& dump_me)
    {
        this->log(message, dump_me);
    }
    /// Output log messages.
    void print_log(const std::string message, const arr<>& dump_me)
    {
        this->log(message, dump_me);
    }
    /// Output log messages.
    void print_log(const std::string message) { this->log(message); }
    /// Outputs the Results obtained by Column Generation or from a Master Problem into some readable form.
    void print_raw(const std::vector<time_of_flight>& data,
                   const std::vector<double>& values)
    {
        this->raw(data, values);
    }
    /// Outputs the Results obtained by Column Generation or from a Master Problem into some readable form.
    void print_converted(const std::vector<cartesian<>>& data,
                         const std::vector<double>& values)
    {
        this->converted(data, values);
    }
    virtual ~printer();

  protected:
    virtual void raw(const std::vector<time_of_flight>& data,
                     const std::vector<double>& value) = 0;
    virtual void converted(const std::vector<cartesian<>>& data,
                           const std::vector<double>& value) = 0;
    virtual void log(const std::string message,
                     const arr<unsigned>& dump_me) = 0;
    virtual void log(const std::string message, const arr<>& dump_me) = 0;
    virtual void log(const std::string message) = 0;
};

/// Doesnt print anything.
class muted_printer : public printer
{
  public:
    virtual ~muted_printer() override;

  protected:
    virtual void log(const std::string message,
                     const arr<unsigned>& dump_me) override;
    virtual void log(const std::string message, const arr<>& dump_me) override;
    virtual void log(const std::string message) override;
    virtual void raw(const std::vector<time_of_flight>& data,
                     const std::vector<double>& value) override;
    virtual void converted(const std::vector<cartesian<>>& data,
                           const std::vector<double>& value) override;
};

/// Prints in textform to stream.
class simple_printer : public printer
{
  public:
    simple_printer(std::ostream& os, unsigned arr_size_limit = 1000);
    simple_printer(std::string& filename, unsigned arr_size_limit = 1000);
    virtual ~simple_printer() override;

  protected:
    std::ostream& os;
    bool owner;
    unsigned arr_size_limit;
    //int flush = 0;
    //const int flush_rate = 3;
    virtual void log(const std::string message,
                     const arr<unsigned>& dump_me) override;
    virtual void log(const std::string message, const arr<>& dump_me) override;
    virtual void log(const std::string message) override;
    virtual void raw(const std::vector<time_of_flight>& data,
                     const std::vector<double>& value) override;
    virtual void converted(const std::vector<cartesian<>>& data,
                           const std::vector<double>& value) override;
};

#endif // PRINTER_H
