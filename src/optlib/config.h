#ifndef CONFIG_H
#define CONFIG_H

#include "coordinates.h"
#include "csv_tools.h"
#include "iterator.h"
#include <functional>
#include <iostream>
#include <optional>
#include <vector>

/// @brief Loads and contains all data needed to do column generation.
///
/// Reads a csv file containing :
/// One or more comments starting with #,
/// followed by
/// measurement_file;reference_file;max_columns;slave_threshold;x_position;pitch;sampling_rate;wave_speed;elements;wave_length;slave_threshold;samples;reference_samples;verbose;output;slavestop
class config
{
    struct dump_helper
    {

        std::ostream& os;

        template<typename T>
        void add(std::string name, T value);
    };
    using tof_analyser = std::function<void(const time_of_flight&)>;

  public:
    config();
    ~config();

    using optional_time_of_flight_input_vector =
      std::optional<std::reference_wrapper<input_iterator<time_of_flight>>>;
    using optional_time_of_flight_output_vector =
      std::optional<std::reference_wrapper<std::vector<time_of_flight>>>;
    using optional_values =
      std::optional<std::reference_wrapper<std::vector<double>>>;

    double probe_x_pos_in_tacts() const;
    double element_pitch_in_tacts() const;
    double meters_to_tacts(double meters) const;
    double tacts_to_meter(double tacts) const;

    bool load(std::istream& is,
              optional_time_of_flight_output_vector warm_start,
              optional_values warm_start_values);
    bool save(std::ostream& os,
              optional_time_of_flight_input_vector tofs,
              optional_values vals) const;
    bool dump_human_readable(std::ostream& os,
                             optional_time_of_flight_input_vector tofs,
                             optional_values tof_values,
                             tof_analyser tof_analyser);
    bool operator!=(config c) const;

    /// Transforms the tofs from down_sampled into tofs for this by multiplying every entry of the tof by the down_scale factor.
    void retarget_tofs_from(config& down_sampled,
                            input_iterator<time_of_flight>&& tofs);

    /// Returns the distance between coordinate (tacts_x,tacts_y) and element e in tacts.
    double distance_in_tacts(double tacts_x, double tacts_y, unsigned e) const;
    /// Transforms a coordinate (tacts_x, tacts_y) into one time-of-flight.
    void tact_coords_to_tof(double coord_x,
                            double coord_y,
                            time_of_flight& out) const;
    double max_distance_x() const;

    double max_distance_y() const;

    /// Returns the coordinates of the middle of pixel (i,j) in tacts.
    void pixel_to_tact_coords(unsigned width,
                              unsigned height,
                              unsigned i,
                              unsigned j,
                              double& x,
                              double& y) const;

    /// Returns the pixel that represents the coordinates (tacts_x, tacts_y).
    void tact_coords_to_pixel(unsigned width,
                              unsigned height,
                              double tacts_x,
                              double tacts_y,
                              unsigned& i,
                              unsigned& j) const;
    /// Throws an exception when something is not ok.
    void consistency_check() const;

  protected:
    /// Contains the data missing in cgdump but needed in cgdump2.
    struct cgdump_to_cgdump2 {

    };
    bool load_tofs(std::istream& is,
                   optional_time_of_flight_output_vector warm_start,
                   optional_values warm_start_values);
    bool save_tofs(std::ostream& os,
                   optional_time_of_flight_input_vector tofs,
                   optional_values tof_values) const;

  public:
    std::string measurement_file = "./data/simulated/9.04.19/model_Dvoie.bin",
                reference_file =
                  "./data/simulated/9.04.19/reference_signal.csv";
    unsigned max_columns = 50;
    double slave_threshold = 0.1;
    double x_position = 0.0, pitch = 1.2e-3;
    unsigned sampling_rate = 40e6;
    unsigned wave_speed = 6350;
    unsigned elements = 16;
    double wave_length = 6350.0 / 5e6;
    unsigned samples = 3518;
    unsigned reference_samples = 512;
    bool verbose = false;
    std::string output;
    double slavestop = 1e6;
    unsigned offset = 0;
    unsigned master_solver = 0;

    std::optional<unsigned> roi_start;
    std::optional<unsigned> roi_end;

    /// CG ends when master has an objective <= this value.
    double master_threshold = 0.1;

    /// Lower bound for the x-variable in slaveLP.
    std::optional<double> horizontal_roi_start;
    /// Upper bound for the x-variable in slaveLP.
    std::optional<double> horizontal_roi_end;

    /// Remove solutions from the master that are below threshold.
    std::optional<double> master_solution_threshold;

    double wave_frequency() const;

    unsigned get_roi_start() const;
    unsigned get_roi_end() const;
    unsigned get_roi_length() const;
};

#endif
