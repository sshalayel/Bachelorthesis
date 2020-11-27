#ifndef READER_H
#define READER_H

#include "arr.h"
#include "csv_tools.h"
#include "exception.h"
#include <cmath>
#include <fstream>

/// An abstract class for getting data.
class reader
{
  private:
    std::istream* last_istr;

  protected:
    unsigned dim1;
    unsigned dim2;
    unsigned dim3;

  public:
    /// Subroutine that chooses the right reader for you (based on the file extension).
    static void read(std::string filename, arr<>& out);
    reader();
    bool run(
      std::istream& istr,
      arr<>&
        out); ///< Reads 3D-data with decode() from file and the length in each dimension.
    bool run(
      const std::string& filename,
      arr<>&
        out); ///< Reads 3D-data with decode() from file and the length in each dimension.

    /// Like run(...) but continue reading with same parameters used in last run. May return null when stream is empty.
    /// For example when opening a 4D-array (positions, senders, receivers, samples), you get the first position with run(filename, senders, receivers, samples).
    /// If you need the positions after the first, run() will continue reading where it left off.
    bool run(arr<>& out);

    virtual ~reader();

  protected:
    virtual bool decode(
      std::istream& str,
      arr<>&) = 0; ///< Allows subclasses to read data from streams.
};

/// @brief Reads 3D-data that is saved like an T[] (Civas binary format).
///
/// Because Civa stores its data in 4D when using multiple probe_positions,
/// it retrieves the first position
/// (and other positions can be read by seeking the filestream or running run (stream/file, dim1, dim2, dim3) followed by multiple run()
template<typename T>
class binary_reader : public reader
{
  public:
    binary_reader();
    ~binary_reader();

  protected:
    virtual bool decode(std::istream& istr, arr<>&) override;
};

/// Reads 1D-data (reference signals) from Civa Txt files.
class civa_txt_reader : public reader
{
  public:
    civa_txt_reader();
    virtual ~civa_txt_reader() override;

  protected:
    virtual bool decode(std::istream& istr, arr<>&) override;
};

template<typename T>
binary_reader<T>::binary_reader()
{}

template<typename T>
bool
binary_reader<T>::decode(std::istream& istr, arr<>& data)
{

    T val;
    for (unsigned i = 0; i < dim1; i++) {
        for (unsigned j = 0; j < dim2; j++) {
            for (unsigned k = 0; k < dim3; k++) {
                assert_read(istr, "Error while reading stream!");
                istr.read((char*)&val, sizeof(val));
                data(i, j, k) = (double)val;
            }
        }
        if (!istr) {
            return false;
        }
    }
    return true;
}

template<typename T>
binary_reader<T>::~binary_reader()
{}

#endif // READER_H
