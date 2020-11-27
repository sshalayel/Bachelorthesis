#ifndef WRITER_H
#define WRITER_H

#include "arr.h"
#include "printer.h"
#include <iostream>

/// An abstract class for writing measurements/duals/...
template<typename T>
class writer
{
  public:
    /// Writes 3D-data with encode() from file.
    bool run(std::ostream& istr, const arr<T>& in)
    {
        return this->encode(istr, in);
    }
    /// Writes 3D-data with encode() from file.
    bool run(const std::string& filename, const arr<T>& in)
    {
        ofstream_with_dirs os(filename);
        return this->encode(os, in);
    }
    virtual ~writer() {}

  protected:
    /// For subclasses.
    virtual bool encode(std::ostream& str, const arr<T>&) = 0;
};

/// Doesn't do anything.
template<typename T>
class muted_writer : public writer<T>
{
  public:
    muted_writer(){};
    ~muted_writer(){};

  protected:
    virtual bool encode(std::ostream& os, const arr<T>& a) override
    {
        return true;
    }
};

/// Writes the data in binary (as doubles) so it can be read in python with `np.fromfile(...)`.
template<typename T>
class binary_writer : public writer<T>
{
  public:
    binary_writer(){};
    ~binary_writer(){};

  protected:
    virtual bool encode(std::ostream& os, const arr<T>& a) override
    {
        os.write((char*)a.begin(), a.size() * sizeof(T));
        os.flush();
        return (bool)os;
    }
};

#endif
