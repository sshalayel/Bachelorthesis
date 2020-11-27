#include "coordinates.h"

double
coordinate_tools::square(double x)
{
    return x * x;
}

double
coordinate_tools::cosinus_gamma(double a, double b, double c)
{
    //std::cout << "a: " << a << " b: " << b << " c: " << c << std::endl;
    double r = (a * a + b * b - c * c) / (2.0 * a * b);
    return r;
}

double
coordinate_tools::lower_cosinus_gamma(double a, double b, double c)
{
    double r = (a * a + b * b - square(c + 1.0)) / (2.0 * (a + 1.0) * b);
    return r;
}

double
coordinate_tools::upper_cosinus_gamma(double a, double b, double c)
{
    double r = (square(a + 1) + b * b - c * c) / (2.0 * a * b);
    return r;
}

double
coordinate_tools::squared_length_of_c(double a, double b, double cosinus_gamma)
{
    return a * a + b * b - 2.0 * a * b * cosinus_gamma;
}

double
coordinate_tools::lower_squared_length_of_c(double a,
                                            double b,
                                            double cosinus_gamma)
{
    return a * a + b * b - 2.0 * (a + 1.0) * b * cosinus_gamma;
}

bounds operator*(double d, bounds b)
{
    //the constructor swaps them for negative d
    return bounds{ d * b.lower, d * b.upper };
}

bounds::bounds(double exact)
  : bounds(exact, exact)
{}

bounds::bounds(double lower, double upper)
  : lower(lower)
  , upper(upper)
{
    if (this->lower > this->upper) {
        std::swap(this->lower, this->upper);
    }
}

bounds
bounds::operator+(bounds b)
{
    bounds ret{ lower + b.lower, upper + b.upper };
    return ret;
}

bounds
bounds::operator-(bounds b)
{
    bounds ret{ lower - b.upper, upper - b.lower };
    return ret;
}

bounds bounds::operator*(bounds b)
{
    const std::vector<double> values{
        lower * b.lower, lower * b.upper, upper * b.lower, upper * b.upper
    };
    bounds ret{
        *std::min_element(values.begin(), values.end()),
        *std::max_element(values.begin(), values.end()),
    };
    return ret;
}

bounds
bounds::operator/(bounds b)
{
    assert_that(b.lower > 0 || b.upper < 0, "Cannot divide by 0!");
    const std::vector<double> values{
        lower / b.lower,
        upper / b.lower,
        lower / b.upper,
        upper / b.upper,
    };
    bounds ret{
        *std::min_element(values.begin(), values.end()),
        *std::max_element(values.begin(), values.end()),
    };
    return ret;
}

bounds
bounds::root()
{
    assert_that(upper > 0, "Cannot take the square root of negative number!");
    return bounds{
        lower > 0.0 ? std::sqrt(lower) : 0.0,
        std::sqrt(upper),
    };
}

bounds
bounds::square()
{
    return bounds{
        lower * lower,
        upper * upper,
    };
}

bool
bounds::is_intersecting(bounds d)
{
    return std::max(lower, d.lower) <= std::min(upper, d.upper);
}

bool
bounds::is_inside(bounds d)
{
    return upper <= d.upper && lower >= d.lower;
}

bounds
bounds::intersect(bounds d)
{
    const double new_lower = std::max(lower, d.lower);
    const double new_upper = std::min(upper, d.upper);
    assert_that(new_lower <= new_upper,
                "Empty Intervals are not allowed here!");
    return bounds{ new_lower, new_upper };
}

bounds
bounds::round()
{
    return bounds(std::ceil(lower), std::floor(upper));
    //return bounds(std::round(lower), std::round(upper));
}

bounds
coordinate_tools::squared_length_of_c(bounds a, bounds b, bounds cosinus_gamma)
{
    return a.square() + b.square() + (-2.0) * a * b * cosinus_gamma;
}

bounds
coordinate_tools::cosinus_gamma(bounds a, bounds b, bounds c)
{
    return (a.square() + b.square() - c.square()) / (2.0 * a * b);
}

void
bounds::dump(std::ostream& os) const
{
    os << "[" << lower << ", " << upper << "]";
}

std::ostream&
operator<<(std::ostream& os, bounds b)
{
    b.dump(os);
    return os;
}

double
coordinate_tools::upper_squared_length_of_c(double a,
                                            double b,
                                            double cosinus_gamma)
{
    return square(a + 1.0) + b * b - 2.0 * a * b * cosinus_gamma;
}

bool
coordinate_tools::right_cosine(double left,
                               double upper,
                               double right,
                               double& lower_cos,
                               double& upper_cos)
{
    lower_cos = coordinate_tools::lower_cosinus_gamma(right, upper, left);
    upper_cos = coordinate_tools::upper_cosinus_gamma(right, upper, left);

    assert_that(lower_cos <= upper_cos, "Interval cannot be empty");

    return lower_cos <= 1 && upper_cos >= -1;
}

bool
coordinate_tools::check_left_length(double left,
                                    double upper,
                                    double right,
                                    double lower_cos,
                                    double upper_cos)
{
    //TODO: recheck this parameter for numerical instabilities.
    const double epsylon = 5e-2;
    double lower_theoretical_left_sq =
      coordinate_tools::lower_squared_length_of_c(right, upper, upper_cos);
    double upper_theoretical_left_sq =
      coordinate_tools::upper_squared_length_of_c(right, upper, lower_cos);

    // Use the binomische formeln to get squared epsylon.
    const double lower_epsylon =
      -2.0 * std::sqrt(lower_theoretical_left_sq) * epsylon - epsylon * epsylon;
    const double upper_epsylon =
      2.0 * std::sqrt(upper_theoretical_left_sq) * epsylon + epsylon * epsylon;

    assert_that(lower_theoretical_left_sq <= upper_theoretical_left_sq,
                "Interval cannot be empty");

    double left_sq_lower = left * left;
    double left_sq_upper = (left + 1) * (left + 1);

    assert_that(left_sq_lower <= left_sq_upper, "Interval cannot be empty");

    double _dummy, _dummy2;
    return left_sq_lower <= upper_theoretical_left_sq + upper_epsylon &&
           lower_theoretical_left_sq - lower_epsylon <= left_sq_upper &&
           right_cosine(left, upper, right, _dummy, _dummy2);
}

tikz_center_printer::tikz_center_printer(std::ostream& os,
                                         double width_in_m,
                                         double height_in_m,
                                         double pitch_in_m,
                                         double tact_in_m,
                                         unsigned elements,
                                         double minimum_radius_in_m)
  : tikz_tof_printer(os,
                     width_in_m,
                     height_in_m,
                     pitch_in_m,
                     tact_in_m,
                     elements)
  , minimum_radius_in_m(minimum_radius_in_m * scale)
{}

void
tikz_center_printer::add_tof(time_of_flight& tof,
                             unsigned char red,
                             unsigned char green,
                             unsigned char blue,
                             bool with_line)
{
    set_current_color(red, green, blue);
    assert_that(tof.extension, "Need extension to work!");

    double y = tof.extension->y;
    if (std::isnan(y)) {
        y = 0.0;
    }

    os << "\\filldraw[color=currentColor] \\zerothprobelement ++ ("
       << *tof.representant_x * tact_in_mm / 2.0 << "cm, -"
       << tof.extension->y * tact_in_mm / 2.0
       << "cm) circle[radius=" << std::sqrt(get_radius(tof)) * tact_in_mm / 2.0
       << "cm];\n";
}

tikz_center_printer::~tikz_center_printer() {}

double
tikz_center_printer::relative_error(time_of_flight& tof, unsigned i)
{
    const double t_i_squared = std::pow(tof(i, i), 2);
    const double relative_error =
      std::fabs((tof.extension->q(i) - t_i_squared) / t_i_squared);
    return relative_error;
}

double
tikz_center_printer::get_radius(time_of_flight& tof)
{
    double max = minimum_radius_in_m;
    for (unsigned i = 0; i < tof.senders; i++) {
        max = std::max(max, relative_error(tof, i));
    }
    return max;
}

tikz_tof_printer::tikz_tof_printer(std::ostream& os,
                                   double width_in_m,
                                   double height_in_m,
                                   double pitch_in_m,
                                   double tact_in_m,
                                   unsigned elements)
  : os(os)
  , tact_in_mm(tact_in_m * scale)
  , pitch_in_mm(pitch_in_m * scale)
{
    const double width_to_0th_element =
      (width_in_m / 2.0) - pitch_in_m * elements;
    // print header
    os
      << "\\documentclass{standalone}\n"
         "\\usepackage{graphicx}\n"
         "\\usepackage{tikz}\n"

         "\\usetikzlibrary{fadings}\n" // from https://tex.stackexchange.com/questions/82425/tikz-radial-shading-of-a-ring/82444#82444
         "\\pgfdeclareradialshading{myring}{\\pgfpointorigin}\n"
         "{\n"
         "color(0cm)=(transparent!0);\n"
         "color(5mm)=(pgftransparent!50);\n"
         "color(1cm)=(pgftransparent!100)\n"
         "}\n"
         "\\pgfdeclarefading{ringo}{\\pgfuseshading{myring}}\n"

         "\\def\\width{"
      << width_in_m * scale
      << "cm}\n"
         "\\def\\height{"
      << std::ceil(height_in_m * scale / 10.0) *
           10.0 // align the grid to the transducer
      << "cm}\n"
         "\\def\\pitch{"
      << pitch_in_m * scale
      << "cm}\n"
         "\\def\\canvas{(0, 0) rectangle (\\width,\\height)}\n"
         "%%% \\def\\innertofline#1#2{(\\width/2 + \\pitch*#1,\\height) ++ "
         "(#2, "
         "0) arc (0: -180: #2)}\n"
         "\\def\\tact{"
      << tact_in_m * scale / 2
      << "cm}\n"
         "%%%\\def\\outertofline#1#2{\\innertofline{#1}{#2+\\tact}}\n"
         "\\def\\zerothprobelement{("
      << std::ceil(width_to_0th_element * scale / 10.0) *
           10.0 //align grid to transducer
      << "cm,\\height)}\n"
         "\\def\\tofpath#1#2{\\zerothprobelement ++ (\\pitch*#1, 0) ++ "
         "(#2+\\tact, 0) arc (0: -180: #2+\\tact) ++ (\\tact, 0) arc (-180: "
         "0: #2) ++ "
         "(\\tact, 0)}\n"
         "\\def\\currentOpacity{"
      << opacity
      << "}\n"
         "\\def\\probeelement#1{\\zerothprobelement ++ (- \\pitch/2 + "
         "#1*\\pitch, 0) "
         "rectangle ++ (\\pitch, \\pitch)}\n"
         "\\begin{document}\n"
         "\\begin{tikzpicture}\n"
         //"\\draw \\canvas;\n"
         "\\draw[step=10cm] (0,0) grid ++(\\width,\\height); % "
         "1cm-grid \n"
         "\\foreach \\element in {0, ..., "
      << elements - 1
      << "} {\n"
         "\\draw \\probeelement{\\element};\n"
         "}\n"
         // unit block for massstab
         //   "\\draw (0.3cm,0.3cm) rectangle "
         //   "++("
         //<< scale / 100 << "cm," << scale / 100
         //<< "cm);\n"
         //   "\\draw (0.8cm,0.8cm) node {"
         ////<< std::setprecision(2) << (double)scale / 1000.0 << std::defaultfloat
         //<< (double)scale / 1000.0
         //<< "mm};\n"

         "\\draw \\canvas;\n"
         "\\clip \\canvas;\n";
}

void
tikz_tof_printer::set_current_color(unsigned char red,
                                    unsigned char green,
                                    unsigned char blue)
{
    os << "\\definecolor{currentColor}{rgb}{" << insert_char_as_double(red)
       << "," << insert_char_as_double(green) << ","
       << insert_char_as_double(blue) << "}\n";
}

tikz_tof_printer::~tikz_tof_printer()
{
    os << "\\end{tikzpicture}\n"
          "\\end{document}\n";
    os.flush();
}

void
tikz_tof_printer::add_reflector_hint(tact_coordinate t,
                                     unsigned char red,
                                     unsigned char green,
                                     unsigned char blue)
{
    set_current_color(red, green, blue);
    os << "\\draw[color=currentColor] \\zerothprobelement ++ ("
       << t.x * tact_in_mm << "cm, -" << t.y * tact_in_mm
       << "cm) circle[radius=1cm];\n";
}

std::string
tikz_tof_printer::insert_char_as_double(unsigned char c)
{
    return std::to_string((double)c / 255.0);
}

double
tikz_tof_printer::compute_radius_x(double value,
                                   unsigned sender,
                                   unsigned receiver)
{
    assert_that(sender <= receiver, "Please sort the elements!");
    return value / 2.0;
}

double
tikz_tof_printer::compute_radius_y(double value,
                                   unsigned sender,
                                   unsigned receiver)
{
    assert_that(sender <= receiver, "Please sort the elements!");
    const double distance_between_elements = (receiver - sender) * pitch_in_mm;

    const double radius_y_square =
      std::pow(value / 2.0, 2) - std::pow(distance_between_elements / 2.0, 2);
    assert_that(radius_y_square >= 0, "Square needs to be positive!");
    double ret = std::sqrt(radius_y_square);
    return ret;
}

void
tikz_tof_printer::add_ellipse_path(unsigned sender,
                                   unsigned receiver,
                                   double tof_value,
                                   bool start_between_elements,
                                   bool left_to_right)
{
    assert_that(sender <= receiver, "Please sort the elements!");

    const double small_value = tof_value * tact_in_mm;
    const double small_radius_x =
      compute_radius_x(small_value, sender, receiver);
    const double small_radius_y =
      compute_radius_y(small_value, sender, receiver);

    if (start_between_elements) {
        const double middle = (receiver + sender) * pitch_in_mm / 2.0;

        /// go between the two elements.
        os << "\\zerothprobelement ++ (" << middle << "cm, 0) ";
        /// Go to right.
        if (left_to_right) {
            os << "++(" << -small_radius_x << "cm,0) ";
        } else {
            os << "++(" << small_radius_x << "cm,0) ";
        }
    }
    int start_angle = left_to_right ? -180 : 0;
    int end_angle = left_to_right ? 0 : -180;
    /// Do the smaller ellipse from right to left.
    os << "arc[start angle=" << start_angle << ", end angle=" << end_angle
       << ", x radius=" << small_radius_x << "cm, y radius=" << small_radius_y
       << "cm] ";
}

void
tikz_tof_printer::add_tof_path(unsigned sender,
                               unsigned receiver,
                               unsigned tof_value)
{
    assert_that(sender <= receiver, "Please sort the elements!");

    const double overlap = 1.0;
    const double small_value = tof_value - overlap / 2.0;
    const double big_value = tof_value + overlap / 2.0;
    const double small_radius_x =
      compute_radius_x(small_value, sender, receiver);
    const double big_radius_x = compute_radius_x(big_value, sender, receiver);

    add_ellipse_path(sender, receiver, small_value, true, false);
    os << "++(" << (small_radius_x - big_radius_x) * tact_in_mm << "cm, 0) ";
    add_ellipse_path(sender, receiver, big_value, false, true);
    ///close the path
    //os << "++(" << small_radius_x - big_radius_x << ", -1) ";
    os << "++(" << -overlap * tact_in_mm - 0.1 << "cm, 0) ";
}

void
tikz_tof_printer::add_tof(time_of_flight& tof,
                          unsigned char red,
                          unsigned char green,
                          unsigned char blue,
                          bool with_line)
{
    assert_that(tof.senders == tof.receivers, "Not supported!");

    // Only needed for debugging.
    const bool debug = false;
    if (debug) {
        std::cout << "Pitch is " << pitch_in_mm << std::endl;
        auto f = [&](unsigned s, unsigned r, std::string color, bool a) {
            if (a) {
                std::cout << s << ", " << r << " : " << tof(s, r) << std::endl;
                os << "\\fill [color=" << color << "]";
                add_tof_path(s, r, tof(s, r));
                os << ";\n";
            } else {
                os << "\\draw [color=" << color << "]";
                add_tof_path(s, r, tof(s, r));
                os << ";\n";
            }
        };
        for (unsigned i = 0; i < 2; i++) {
            //f(0, 0, "red", i == 0);
            //f(0, 1, "blue", i == 0);
            //f(0, 2, "orange", i == 0);
            //
            //f(0, 3, "pink", i == 0);
            //f(10, 15, "green", i == 0);
            //f(8, 8, "orange", i == 0);
            //
            f(0, 0, "pink", i == 0);
            f(3, 3, "blue", i == 0);
            f(15, 15, "green", i == 0);
            f(10, 10, "red", i == 0);
            f(8, 8, "orange", i == 0);
        }
    } else {

        os << "\\begin{scope}\n";
        set_current_color(red, green, blue);

        for (unsigned s = 0; s < tof.senders; s++) {
            const unsigned limit = only_diagonal ? s + 1 : tof.receivers;
            for (unsigned r = s; r < limit; r++) {
                //fill in last iteration, else clip.
                if (s == tof.senders - 1 && r == tof.receivers - 1) {
                    os << "\\fill [color=currentColor, "
                          "opacity=\\currentOpacity]";
                } else {
                    os << "\\clip ";
                }
                add_tof_path(s, r, tof(s, r));
                os << ";\n";
            }
        }
        if (with_line) {
            os << "\\end{scope}\n";
            os << "\\begin{scope}\n";
            for (unsigned s = 0; s < tof.senders; s++) {
                const unsigned limit = only_diagonal ? s + 1 : tof.receivers;
                for (unsigned r = s; r < limit; r++) {
                    set_current_color(255, 0, 0);
                    os << "\\draw[color=currentColor] ";
                    add_ellipse_path(s, r, tof(s, r) + 0.5, true, true);
                    os << ";\n";
                    set_current_color(0, 0, 255);
                    os << "\\draw[color=currentColor] ";
                    add_ellipse_path(s, r, tof(s, r), true, true);
                    os << ";\n";
                    set_current_color(0, 255, 0);
                    os << "\\draw[color=currentColor] ";
                    add_ellipse_path(s, r, tof(s, r) - 0.5, true, true);
                    os << ";\n";
                }
            }
        }
        os << "\\end{scope}\n";
    }
}

time_of_flight::time_of_flight(unsigned senders,
                               unsigned receivers,
                               std::optional<double> representant_x)
  : arr_2d(senders, receivers)
  , representant_x(representant_x)
{}

bool
time_of_flight::operator<(time_of_flight& tof)
{
    return std::lexicographical_compare(diagonal_begin(),
                                        diagonal_end(),
                                        tof.diagonal_begin(),
                                        tof.diagonal_end());
}

bool
time_of_flight::operator==(time_of_flight& tof)
{
    return std::equal(diagonal_begin(),
                      diagonal_end(),
                      tof.diagonal_begin(),
                      tof.diagonal_end());
}

/// Wrap around existing data.
time_of_flight::time_of_flight(unsigned senders,
                               unsigned receivers,
                               unsigned* data,
                               bool owner,
                               std::optional<double> representant_x)
  : arr_2d(senders, receivers, data, owner)
  , representant_x(representant_x)
{}

time_of_flight::time_of_flight(time_of_flight&& tof)
  : arr_2d(std::move(tof))
  , representant_x(tof.representant_x)
  , extension(std::move(tof.extension))
{}

time_of_flight&
time_of_flight::operator=(time_of_flight&& tof)
{
    arr_2d::operator=(std::move(tof));
    representant_x = tof.representant_x;
    extension = std::move(tof.extension);
    return *this;
}

time_of_flight::~time_of_flight() {}

void
time_of_flight::simulate(const arr<>& reference_signal, arr<>& out) const
{
    out.for_each([](double& d) { d = 0.0; });

    for (unsigned i = 0; i < senders; i++) {
        for (unsigned j = 0; j < receivers; j++) {
            const double* reference_begin = &reference_signal.at(i, j, 0);
            const unsigned steps =
              std::min(reference_signal.dim3, out.dim3 - at(i, j));
            double* simulation_begin = &out.at(i, j, at(i, j));

            std::copy_n(reference_begin, steps, simulation_begin);
        }
    }
}

double
time_of_flight::dot_product_with_dual(const arr<>& reference_signal,
                                      const arr<>& dual,
                                      unsigned offset) const
{
    double dot_product = 0.0;
    for (unsigned i = 0; i < senders; i++) {
        for (unsigned j = 0; j < receivers; j++) {
            if ((int)dual.dim3 <= (int)(at(i, j)) - (int)offset) {
                continue;
            }
            const double* reference_begin = &reference_signal.at(i, j, 0);
            const unsigned steps = std::min(reference_signal.dim3,
                                            (int)dual.dim3 - at(i, j) + offset);
            const double* reference_end = reference_begin + steps;
            const double* simulation_begin =
              &dual.at(i, j, (int)at(i, j) - offset);

            dot_product = std::inner_product(
              reference_begin, reference_end, simulation_begin, dot_product);
        }
    }
    return dot_product;
}

void
time_of_flight::fill_from_diagonal()
{
    for (unsigned i = 0; i < senders; i++) {
        for (unsigned j = 0; j < receivers; j++) {
            if (i != j) {
                at(i, j) = (at(i, i) + at(j, j)) / 2;
            }
        }
    }
}

using ce = time_of_flight::cgdump2_extension;

ce::cgdump2_extension(double y, diameter_t<double>&& d, quadratic_t<double>&& q)
  : y(y)
  , d(std::move(d))
  , q(std::move(q))
{}

ce::cgdump2_extension(cgdump2_extension&& ext)
  : y(ext.y)
  , d(std::move(ext.d))
  , q(std::move(ext.q))
{}

ce&
ce::operator=(cgdump2_extension&& ext)
{
    y = ext.y;
    d = std::move(ext.d);
    q = std::move(ext.q);
    return *this;
}
