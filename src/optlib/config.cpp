#include "config.h"
#include <cassert>

config::config() {}
config::~config() {}

bool
config::operator!=(config c) const
{
    return !(
      measurement_file == c.measurement_file &&
      reference_file == c.reference_file && max_columns == c.max_columns &&
      slave_threshold == c.slave_threshold &&
      master_threshold == c.master_threshold && x_position == c.x_position &&
      sampling_rate == c.sampling_rate && wave_speed == c.wave_speed &&
      elements == c.elements && wave_length == c.wave_length &&
      samples == c.samples && reference_samples == c.reference_samples &&
      verbose == c.verbose && output == c.output && slavestop == c.slavestop &&
      offset == c.offset && roi_start == c.roi_start && roi_end == c.roi_end);
}

bool
config::save(std::ostream& os,
             optional_time_of_flight_input_vector tofs,
             optional_values tof_values) const
{
    os << std::boolalpha;
    //header
    os << "#measurement_file;reference_file;max_columns;x_position;"
          "pitch;"
          "sampling_rate;wave_speed;elements;wave_length;slave_"
          "threshold;"
          "samples;reference_samples;verbose;output;slavestop;offset\n";

    insert_with_semicolon(os, measurement_file);
    insert_with_semicolon(os, reference_file);
    insert_with_semicolon(os, max_columns);
    insert_with_semicolon(os, x_position);
    insert_with_semicolon(os, pitch);

    double sampling_rate_helper = sampling_rate;
    insert_with_semicolon(os, sampling_rate_helper);

    insert_with_semicolon(os, wave_speed);
    insert_with_semicolon(os, elements);
    insert_with_semicolon(os, wave_length);
    insert_with_semicolon(os, slave_threshold);
    insert_with_semicolon(os, samples);
    insert_with_semicolon(os, reference_samples);
    insert_with_semicolon(os, verbose);
    insert_with_semicolon(os, output);
    insert_with_semicolon(os, slavestop);
    insert_with_semicolon(os, offset);
    os << '\n';
    return save_tofs(os, tofs, tof_values);
}

bool
config::save_tofs(std::ostream& os,
                  optional_time_of_flight_input_vector tofs,
                  optional_values tof_values) const
{
    if (tofs && tof_values) {
        //sort tofs in descending order

        using ValueTofTuple =
          std::tuple<double, std::reference_wrapper<time_of_flight>>;
        std::vector<ValueTofTuple> sort_me;
        for (double current_value : tof_values->get()) {
            std::reference_wrapper<time_of_flight> current_tof = *assert_that(
              tofs->get().next(),
              "Each tof should have its own value (and vice-versa)!");
            sort_me.emplace_back(current_value, current_tof);
        }

        std::sort(sort_me.begin(),
                  sort_me.end(),
                  [](const ValueTofTuple& a, const ValueTofTuple& b) {
                      // ascending order : a before b is true if a>b.
                      return std::get<double>(a) > std::get<double>(b);
                  });

        for (auto&& [current_value, _current_tof] : sort_me) {
            time_of_flight& current_tof = _current_tof.get();
            os << current_value << ';';

            for (unsigned j = 0; j < current_tof.senders; j++) {
                for (unsigned k = 0; k < current_tof.receivers; k++) {
                    insert_with_semicolon(os, current_tof(j, k));
                }
            }

            if (current_tof.extension) {
                assert_that(current_tof.representant_x,
                            "Should be set at the same time as the extension!");
                auto& ext = *current_tof.extension;
                insert_with_semicolon(os, *current_tof.representant_x);
                insert_with_semicolon(os, ext.y);

                for (unsigned j = 0; j < current_tof.senders; j++) {
                    for (unsigned k = 0; k < current_tof.receivers; k++) {
                        insert_with_semicolon(os, ext.d(j, k));
                    }
                }
                for (unsigned k = 0; k < current_tof.receivers; k++) {
                    insert_with_semicolon(os, ext.q(k));
                }
            }
            os << '\n';
        }
    }
    os.flush();

    return (bool)os;
}

bool
config::load_tofs(std::istream& is,
                  optional_time_of_flight_output_vector warm_start,
                  optional_values warm_start_values)
{
    if (warm_start || warm_start_values) {
        double val;
        while (is >> val, is) {
            if (warm_start_values) {
                warm_start_values->get().push_back(val);
            }
            semi(is);

            if (warm_start) {
                time_of_flight t(elements, elements, {});
                for (unsigned j = 0; j < elements; j++) {
                    for (unsigned k = 0; k < elements; k++) {
                        is >> t.at(j, k);
                        semi(is);
                    }
                }
                if (is.peek() != '\n') {
                    /// load extended part
                    //
                    double x;
                    double y;
                    extract_with_semicolon(is, x);
                    t.representant_x = x;
                    extract_with_semicolon(is, y);
                    if (std::isnan(y)) {
                        y = 0.0;
                    }

                    diameter_t<double> d(elements, elements);
                    for (unsigned j = 0; j < t.senders; j++) {
                        for (unsigned k = 0; k < t.receivers; k++) {
                            extract_with_semicolon(is, d(j, k));
                        }
                    }
                    quadratic_t<double> q(elements);
                    for (unsigned k = 0; k < t.receivers; k++) {
                        extract_with_semicolon(is, q(k));
                    }

                    t.extension = time_of_flight::cgdump2_extension(
                      y, std::move(d), std::move(q));
                }
                warm_start->get().push_back(std::move(t));
            }
            nextline(is);
        }
    }
    return ((bool)is || is.eof());
}

template<typename T>
void
config::dump_helper::add(std::string name, T value)
{
    os << std::right << std::setw(40) << name << ": " << std::left
       << std::setw(49) << value << "\n"
       << std::right;
}

bool
config::dump_human_readable(std::ostream& os,
                            optional_time_of_flight_input_vector tofs,
                            optional_values tof_values,
                            tof_analyser tof_analyser)
{
    os << std::boolalpha;
    dump_helper dh{ os };
    dh.add("Measurement File", measurement_file);
    dh.add("Reference File", reference_file);
    dh.add("Max Columns", max_columns);
    dh.add("X-Position", x_position);
    dh.add("Pitch (in m)", pitch);
    dh.add("Pitch (in tacts)", element_pitch_in_tacts());
    dh.add<double>("Sampling Rate", sampling_rate);
    dh.add("Wave Speed (in m/s)", wave_speed);
    dh.add("Number of elements", elements);
    dh.add("Wave Length (in m)", wave_length);
    dh.add("Wave Length (in tacts)", meters_to_tacts(wave_length));
    dh.add("Samples in Measurement", samples);
    dh.add("Samples in Reference Signal", reference_samples);
    dh.add("SlaveStop", slavestop);
    dh.add("Offset (in m)", tacts_to_meter(offset));
    dh.add("Offset (in tacts)", offset);

    os << '\n';
    if (tofs && tof_values) {
        for (unsigned i = 0; i < tof_values->get().size(); i++) {
            dh.add("Amplitude", (tof_values->get())[i]);
            const time_of_flight& current_tof =
              *assert_that(tofs->get().next(),
                           "tofs and tof_values should have the same size!");
            for (unsigned j = 0; j < current_tof.senders; j++) {
                for (unsigned k = 0; k < current_tof.receivers; k++) {
                    os << std::setw(6) << current_tof.at(j, k);
                }
                os << "\n";
            }
            os << '\n';
            if (tof_analyser) {
                tof_analyser(current_tof);
            }
            if (current_tof.extension) {
                auto& ext = *current_tof.extension;
                dh.add("X-position (in tacts)", *current_tof.representant_x);
                dh.add("X-position (in meters)",
                       tacts_to_meter(*current_tof.representant_x));
                dh.add("Y-position (in tacts)", ext.y);
                dh.add("Y-position (in meters)", tacts_to_meter(ext.y));
                for (unsigned j = 0; j < current_tof.senders; j++) {
                    for (unsigned k = 0; k < current_tof.receivers; k++) {
                        os << std::setw(6) << ext.d(j, k);
                    }
                    os << "\n";
                }
                os << '\n';
                for (unsigned k = 0; k < current_tof.senders; k++) {
                    os << std::setw(9) << ext.q(k);
                }
                os << '\n';
            }
        }
    }
    os.flush();

    return (bool)os;
}

bool
config::load(std::istream& is,
             optional_time_of_flight_output_vector warm_start,
             optional_values warm_start_values)
{
    is >> std::boolalpha;
    skip_comments(is);

    std::string maybe_x_position;
    extract_with_semicolon(is, maybe_x_position);

    char* end;
    x_position = std::strtod(maybe_x_position.c_str(), &end);

    if (*end == '\0') {
        //old cgdump
        extract_with_semicolon(is, pitch);
        extract_with_semicolon(is, sampling_rate);
        extract_with_semicolon(is, wave_speed);
        extract_with_semicolon(is, elements);
        extract_with_semicolon(is, elements);
        extract_with_semicolon(is, wave_length);
        extract_with_semicolon(is, samples);
        extract_with_optional_semicolon(is, reference_samples);
    } else {
        //new cgdump
        measurement_file = maybe_x_position;
        extract_with_semicolon(is, reference_file);
        extract_with_semicolon(is, max_columns);
        extract_with_semicolon(is, x_position);
        extract_with_semicolon(is, pitch);
        double sampling_rate_helper;
        extract_with_semicolon(is, sampling_rate_helper);
        sampling_rate = (unsigned)sampling_rate_helper;
        extract_with_semicolon(is, wave_speed);
        extract_with_semicolon(is, elements);
        extract_with_semicolon(is, wave_length);
        extract_with_semicolon(is, slave_threshold);
        extract_with_semicolon(is, samples);
        extract_with_semicolon(is, reference_samples);
        extract_with_semicolon(is, verbose);
        extract_with_semicolon(is, output);
        extract_with_semicolon(is, slavestop);
        extract_with_optional_semicolon(is, offset);
    }

    return load_tofs(is, warm_start, warm_start_values);
}

double
config::probe_x_pos_in_tacts() const
{
    return meters_to_tacts(x_position);
}

double
config::element_pitch_in_tacts() const
{
    return meters_to_tacts(pitch);
}

double
config::meters_to_tacts(double meters) const
{
    return meters * sampling_rate / wave_speed;
}

double
config::tacts_to_meter(double tacts) const
{
    return tacts * wave_speed / sampling_rate;
}

double
config::distance_in_tacts(double tacts_x, double tacts_y, unsigned e) const
{
    assert(e < elements);

    double distance =
      std::sqrt(std::pow(tacts_x - (e * element_pitch_in_tacts()), 2) +
                std::pow(tacts_y, 2));
    return distance;
}

void
config::tact_coords_to_tof(double tacts_x,
                           double tacts_y,
                           time_of_flight& tof) const
{
    for (unsigned s = 0; s < tof.senders; s++) {
        double sender_distance = distance_in_tacts(tacts_x, tacts_y, s);
        tof.at(s, s) = (unsigned)std::floor(2.0 * sender_distance);

        for (unsigned r = s + 1; r < tof.receivers; r++) {
            double receiver_distance = distance_in_tacts(tacts_x, tacts_y, r);
            unsigned length =
              (unsigned)std::floor(sender_distance + receiver_distance);
            tof.at(s, r) = length;
            tof.at(r, s) = length;
        }
    }
    tof.representant_x = tacts_x;
}

double
config::max_distance_x() const
{
    return 2.0 * (get_roi_end() / 2.0 + element_pitch_in_tacts() * elements);
}

double
config::max_distance_y() const
{
    return get_roi_end() / 2.0;
}

void
config::pixel_to_tact_coords(unsigned width,
                             unsigned height,
                             unsigned i,
                             unsigned j,
                             double& x,
                             double& y) const
{
    x = i;
    x -= width / 2;
    x *= max_distance_x();
    x /= width;

    y = j;
    y *= max_distance_y();
    y /= height;
}

void
config::tact_coords_to_pixel(unsigned width,
                             unsigned height,
                             double tacts_x,
                             double tacts_y,
                             unsigned& i,
                             unsigned& j) const
{
    i = std::round((tacts_x * width) / max_distance_x() + (width / 2));
    j = std::round((tacts_y * height) / max_distance_y());
}

unsigned
config::get_roi_start() const
{
    return roi_start.value_or(offset);
}

unsigned
config::get_roi_end() const
{
    return roi_end.value_or(samples + offset);
}

unsigned
config::get_roi_length() const
{
    return get_roi_end() - get_roi_start();
}

void
config::consistency_check() const
{
    assert_that(slavestop >= slave_threshold,
                "Incompatible slavestop and slave_threshold! Why should the "
                "slave stop at a slavestop < slave_threshold?");
}

void
config::retarget_tofs_from(config& down_sampled,
                           input_iterator<time_of_flight>&& tofs)
{
    const unsigned scale = down_sampled.wave_speed / wave_speed;
    assert_that(
      scale > 0,
      "The down_sampled config should be a down_sampled version of this!");

    while (auto tof = tofs.next()) {
        tof->get().for_each([=](unsigned& i) { i *= scale; });
    }
}

double
config::wave_frequency() const
{
    return wave_speed / wave_length;
}
