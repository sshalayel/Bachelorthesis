#ifndef COORDINATES_H
#define COORDINATES_H

#include "arr.h"
#include "exception.h"
#include "statistics.h"
#include <cassert>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <string>
#include <variant>
#include <vector>

/// Helper struct to easily overload multiple lambdas.
template<class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};
/// Template-deduction Helper.
template<class... Ts>
overload(Ts...)->overload<Ts...>;
/// Visits i with the overloaded lambdas Ts.
template<class Input, class... Ts>
decltype(auto)
visit_with_overloads(Input i, Ts... lambdas)
{
    std::visit(overload{ lambdas... }, i);
}

/// The type of the arr of binary variables b_ijk.
template<typename T>
using binary_t = symmetric_arr<proxy_arr<T>>;
/// The type of the arr of diameter variables d_ij.
template<typename T>
using diameter_t = symmetric_arr_2d<arr, T>;
/// The type of the arr of binary variables q_i.
template<typename T>
using quadratic_t = arr_1d<proxy_arr, T>;

/// Represents an interval [lower, upper] that can be added/multiplied with other intervals.
struct bounds
{
    /// The lower bound.
    double lower;
    /// The upper bound.
    double upper;

    /// Returns the new bound obtained when 2 variables from both bounds are multiplied.
    bounds operator*(bounds b);
    /// Returns the new bound obtained when 2 variables from both bounds are divided.
    bounds operator/(bounds b);
    /// Returns the new bound obtained when 2 variables from both bounds are added.
    bounds operator+(bounds b);
    /// Returns the new bound obtained when 2 variables from both bounds are subtracted.
    bounds operator-(bounds b);
    /// Returns the new bound obtained when squaring a variable.
    bounds square();
    /// Returns the new bound obtained when taking the square root of a variable.
    bounds root();
    /// Returns if this lies inside d.
    bool is_inside(bounds d);
    /// Returns if this intersects d
    bool is_intersecting(bounds d);

    /// Returns the new bound obtained when intersecting 2 intervals.
    bounds intersect(bounds d);

    /// Rounds the upper bound down and the lower bound up to the next integers.
    bounds round();

    /// Constructs a bound for an exact value.
    bounds(double exact);

    /// Constructs a bound and makes sure that lower < upper.
    bounds(double lower, double upper);

    void dump(std::ostream& os) const;
};

std::ostream&
operator<<(std::ostream& os, bounds b);
/// New bounds obtained when multiplying a bound with a constant.
bounds operator*(double d, bounds b);

/// For coloring : represents a pixel at position x and y belonging to some reflector.
struct pixel
{
    unsigned x, y, reflector;

    bool operator<(const pixel p) const
    {
        if (x != p.x) {
            return x < p.x;
        }

        if (y != p.y) {
            return y < p.y;
        }

        return reflector < p.reflector;
    }
};

/// The origin lies at the 0th element, and the distances are in tacts. Represent the position if a reflector for the tikz --hints option.
struct tact_coordinate
{
    /// X coordinate in tacts.
    double x;
    /// Y coordinate in tacts.
    double y;
    /// The corresponding reflector.
    unsigned reflector;
};

///Helper functions for computing cosines.
class coordinate_tools
{
  public:
    static double square(double base);
    /// Gamma is the angle lying on the opposite side of the length c.
    static double cosinus_gamma(double a, double b, double c);
    /// Returns a lower bound for the cosinus,
    /// assuming that b is exact and a and c are the floor of their real values.
    static double lower_cosinus_gamma(double a, double b, double c);
    /// Returns a upper bound for the cosinus,
    /// assuming that b is exact and a and c are the floor of their real values.
    static double upper_cosinus_gamma(double a, double b, double c);
    /// Returns the length of the third side of triangle
    /// with 2 sides of length a and b with an angle of c.
    static double squared_length_of_c(double a, double b, double cosinus_gamma);

    /// Returns an upper and a lower bound for the squared_length.
    static bounds squared_length_of_c(bounds a, bounds b, bounds cosinus_gamma);
    /// Returns an upper and a lower bound for the cosine.
    static bounds cosinus_gamma(bounds a, bounds b, bounds c);

    /// Returns a lower bound for the squared_length,
    /// assuming that b is exact and a is the floor of their real values.
    static double lower_squared_length_of_c(double a,
                                            double b,
                                            double cosinus_gamma);
    /// Returns a upper bound for the squared length,
    /// assuming that b is exact and a is the floor of their real values.
    static double upper_squared_length_of_c(double a,
                                            double b,
                                            double cosinus_gamma);

    static bool right_cosine(double left,
                             double upper,
                             double right,
                             double& lower_cos,
                             double& upper_cos);

    static bool check_left_length(double left,
                                  double upper,
                                  double right,
                                  double lower_cos,
                                  double upper_cos);

    /// Checks if a tof is valid.
    template<typename InputIt>
    static bool tof_feasible_in_range(InputIt start,
                                      InputIt end,
                                      unsigned _n,
                                      double element_pitch_in_tacts);
};

template<typename InputIt>
bool
coordinate_tools::tof_feasible_in_range(InputIt start,
                                        InputIt end,
                                        unsigned _n,
                                        double element_pitch_in_tacts)
{
    const double double_element_pitch_in_tacts = 2 * element_pitch_in_tacts;
    unsigned n = _n;
    assert_that(n != 0 && start != end, "ToF-Iterator cannot be empty");
    if (n == 1) {
        return true;
    }

    const unsigned right = *--end;

    // For 2 elements: just check the cosine
    double lower_cosine, upper_cosine;
    if (!right_cosine(*start,
                      n * double_element_pitch_in_tacts,
                      right,
                      lower_cosine,
                      upper_cosine)) {
        return false;
    }

    ++start;

    // For more then 2 elements: check cosine and lengths.
    for (; start != end; ++start) {
        const unsigned left = *start;
        const double upper = --n * double_element_pitch_in_tacts;

        if (!check_left_length(
              left, upper, right, lower_cosine, upper_cosine)) {
            return false;
        }
    }
    assert(n == 2);
    return true;
}

/// Point with (x,y) coordinates.
template<typename T = double>
class cartesian
{
  public:
    int x;
    int y;
    T value;
    operator std::string() const;
    cartesian(int x, int y, T value);
    cartesian(std::string& s);
    cartesian();
};

/// Struct for (rcv_tact_1, recv_tact_2, ... , recv_tact_n) - Coordinates that are used in the LP's.
class time_of_flight : public arr_2d<arr, unsigned>
{
  public:
    time_of_flight(unsigned senders,
                   unsigned receivers,
                   std::optional<double> representant_x);

    /// Wrap around existing data.
    time_of_flight(unsigned senders,
                   unsigned receivers,
                   unsigned* data,
                   bool owner,
                   std::optional<double> representant_x);

    time_of_flight(time_of_flight&& tof);

    time_of_flight& operator=(time_of_flight&& tof);

    /// Compares two ToF by their diagonal values.
    bool operator<(time_of_flight& t);

    /// Compares two ToF by their diagonal values.
    bool operator==(time_of_flight& t);

    ~time_of_flight() override;

    unsigned& senders = this->dim2;
    unsigned& receivers = this->dim3;

    /// Value of x-representant.
    std::optional<double> representant_x;

    /// represents values needed in cgdump2 for center-visu.
    struct cgdump2_extension
    {
        cgdump2_extension(double y,
                          diameter_t<double>&& d,
                          quadratic_t<double>&& q);

        cgdump2_extension(cgdump2_extension&& ext);
        cgdump2_extension& operator=(cgdump2_extension&& ext);

        double y;
        diameter_t<double> d;
        quadratic_t<double> q;
    };

    std::optional<cgdump2_extension> extension;

    void simulate(const arr<>& reference_signal, arr<>& out) const;

    double dot_product_with_dual(const arr<>& reference_signal,
                                 const arr<>& dual,
                                 unsigned offset) const;
    /// Convenience-function that fills the tof from its current diagonal.
    void fill_from_diagonal();
};

/// Represents all slave solutions of a slave run.
struct slavedump
{
    slavedump(unsigned elements);

    /// The binary variables compactly represented as ToF.
    time_of_flight tof;
    /// The x-representant.
    double x;
    /// The y-representant.
    double y;
    /// The diameter values.
    diameter_t<double> d;
    /// The quadratic values.
    quadratic_t<double> q;
};

template<typename T>
cartesian<T>::cartesian()
{}

template<typename T>
cartesian<T>::operator std::string() const
{
    std::string d = "var";
    d += std::to_string(x);
    d += '_';
    d += std::to_string(y);
    d += '_';
    return d;
}

template<typename T>
cartesian<T>::cartesian(int x, int y, T value)
  : x(x)
  , y(y)
  , value(value)
{}

template<typename T>
cartesian<T>::cartesian(std::string& s)
{
    int x_pos = s.find('_');
    x = std::stoi(s.substr(3, x_pos));
    int y_pos = s.find('_');
    y = std::stoi(s.substr(x_pos + 1, y_pos - x_pos - 1));
}

///@brief Prints tofs as tikz-pdf.
///
/// Needs to be destructed to finish writing to os.
class tikz_tof_printer
{
    /// Ignores non-diagonal elements of tofs when enabled.
    bool only_diagonal = true;

  public:
    tikz_tof_printer(std::ostream& os,
                     double width_in_m,
                     double height_in_m,
                     double pitch_in_m,
                     double tact_in_m,
                     unsigned elements);

    virtual ~tikz_tof_printer();
    virtual void add_tof(time_of_flight& tof,
                         unsigned char red,
                         unsigned char green,
                         unsigned char blue,
                         bool with_line);

    /// Prints the ToF-Path for one ellipse without using latex makros (to be able to compute square roots and so).
    void add_ellipse_path(unsigned sender,
                          unsigned receiver,
                          double tof_value,
                          bool start_between_elements,
                          bool left_to_right);
    /// Prints the ToF-Path (2 ellipses: the big and the small one) without using latex makros (to be able to compute square roots and so).
    void add_tof_path(unsigned sender, unsigned receiver, unsigned tof_value);

    /// Computes the x-Radius of the ellipse needed in Tikz to draw ellipses.
    double compute_radius_x(double value, unsigned sender, unsigned receiver);
    /// Same as compute_radius_x but with the y-axis.
    double compute_radius_y(double value, unsigned sender, unsigned receiver);

    template<typename InputIt>
    void add_tof(InputIt tof_begin,
                 InputIt tof_end,
                 unsigned char red,
                 unsigned char green,
                 unsigned char blue,
                 bool with_line);
    void set_current_color(unsigned char red,
                           unsigned char green,
                           unsigned char blue);

    void add_reflector_hint(tact_coordinate hint,
                            unsigned char red,
                            unsigned char green,
                            unsigned char blue);

    std::string insert_char_as_double(unsigned char c);

    std::ostream& os;
    double tact_in_mm;
    double pitch_in_mm;
    constexpr static const double scale = 1000;
    constexpr static const double opacity = 0.7;
};

/// The center-visualisation.
struct tikz_center_printer : tikz_tof_printer
{
    tikz_center_printer(std::ostream& os,
                        double width_in_m,
                        double height_in_m,
                        double pitch_in_m,
                        double tact_in_m,
                        unsigned elements,
                        double minimum_radius_in_m);

    double minimum_radius_in_m;

    ~tikz_center_printer() override;

    void add_tof(time_of_flight& tof,
                 unsigned char red,
                 unsigned char green,
                 unsigned char blue,
                 bool with_line) override;

    double relative_error(time_of_flight& tof, unsigned element);

    double get_radius(time_of_flight& tof);
};

template<typename InputIt>
void
tikz_tof_printer::add_tof(InputIt diag_begin,
                          InputIt diag_end,
                          unsigned char red,
                          unsigned char green,
                          unsigned char blue,
                          bool with_line)
{

    os << "\\begin{scope}\n";
    set_current_color(red, green, blue);

    InputIt diag_for_lines{ diag_begin };
    InputIt diag_end_for_lines{ diag_end };

    if (with_line) {
        os << "\\begin{scope}\n";
    }
    --diag_end;
    unsigned i = 0;
    for (; diag_begin != diag_end; ++diag_begin, ++i) {
        os << "\\clip \\tofpath{" << std::to_string(i) << "}{"
           << std::to_string(*diag_begin * tact_in_mm / 2.0) << "cm};\n";
    }
    os << "\\fill[color=currentColor, opacity=\\currentOpacity] \\tofpath{"
       << std::to_string(i) << "}{"
       << std::to_string(*diag_end * tact_in_mm / 2.0) << "cm};\n";
    //os << "\\fill[color=currentColor, opacity=\\currentOpacity] \\canvas;\n";
    os << "\\end{scope}\n";

    if (with_line) {
        for (unsigned i = 0; diag_for_lines != diag_end_for_lines;
             ++diag_for_lines, ++i) {
            os << "\\draw[color=currentColor, opacity=\\currentOpacity] "
                  "\\innertofline{"
               << std::to_string(i) << "}{"
               << std::to_string(*diag_for_lines * tact_in_mm / 2.0)
               << "cm};\n"
                  "\\draw[color=currentColor, opacity=\\currentOpacity] "
                  "\\outertofline{"
               << std::to_string(i) << "}{"
               << std::to_string(*diag_for_lines * tact_in_mm / 2.0)
               << "cm};\n";
        }
        os << "\\end{scope}\n";
    }
}

/// Contains one time_of_flight + some statistics about it (that can be dumped and plotted in python).
struct column
{
    /// The time_of_flight.
    time_of_flight tof;
    /// Information about the tof.
    slave_statistics stats;
    /// Optimality of column. Optimal solutions need special treatment.
    enum optimality
    {
        OPTIMAL,
        USER_BOUND_REACHED,
        NON_OPTIMAL
    } optimality = NON_OPTIMAL;

    column(time_of_flight&& tof, slave_statistics stats)
      : tof(std::move(tof))
      , stats(stats)
    {}

    column(column&& c)
      : tof(std::move(c.tof))
      , stats(c.stats)
      , optimality(c.optimality)
    {}
};

/// Represents a dual solution accompagned with some stats (that can be dumped and plotted in python).
struct dual_solution
{
    arr<>& values;
    master_statistics stats;

    dual_solution(arr<>& values, master_statistics stats)
      : values(values)
      , stats(stats)
    {}
};

using columns = std::vector<column>;

#endif // COORDINATES_H
