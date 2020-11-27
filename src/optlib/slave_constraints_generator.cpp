#include "slave_constraints_generator.h"

void
slave_constraint_generator::all(callback callback)
{
    diameter_definition(callback);
    diameter_equality(callback);
    quadratic_definition(callback);
    quadratic_equality(callback);

    binary_sum_to_one(callback);
    triangle_inequality_0th_element(callback);

    root_tangents(callback);
}

void
slave_constraint_generator::root_tangents(callback callback)
{
    const unsigned start = slave_mapping.to_distance(0);
    const unsigned end = slave_mapping.to_distance(f.dim3 - 1);
    double step;

    if (const char* step_as_c_string = std::getenv("TANGENT_STEP")) {
        step = std::stod(step_as_c_string);
    } else {
        //step = 0.5;
        return;
    }

    for (unsigned i = 0; i < f.dim1; i++) {
        linear_expression var_d_ii = lef::create_diameter_variable(i, i);
        linear_expression var_q_i = lef::create_quadratic_variable(i);

        for (double d_i = start; d_i <= end; d_i += step) {
            double sqrt_q_i = d_i;
            double q_i = d_i * d_i;
            callback(var_d_ii <=
                     (1.0 / (2.0 * sqrt_q_i)) * (var_q_i - q_i) + sqrt_q_i);
        }
    }
}

void
slave_constraint_generator::triangle_inequality_0th_element(callback callback)
{
    for (unsigned i = 1; i < f.dim1; i++) {
        for (unsigned k = 0; k < f.dim3; k++) {
            linear_expression s_iik = lef::create_binary_variable(i, i, k);

            linear_expression sum_s00h_in_H = lef::create_constant(0.0);

            /// Upper Bounds obtained by triangle inequality from h <= 2*delta*i + (k + 0.5) up to samples
            const double _h_start =
              i * 2.0 * pitch + slave_mapping.to_distance(k) + 0.5;
            const std::optional<unsigned> h_start =
              slave_mapping.to_index(std::ceil(_h_start));

            if (h_start) {
                sum_s00h_in_H +=
                  lef::create_binary_sum(0,
                                         0,
                                         slave_mapping.to_distance,
                                         binary_sum::SUM,
                                         h_start,
                                         {});

                //callback(s_iik + sum_s00h_in_H <= 1);
            }

            /// Lower Bounds obtained by triangle inequality from 0 to 2*delta*i - (k + 0.5) <= h and (k + 0.5) - 2*delta*i <= h
            const double _h_lower =
              std::fabs(i * 2.0 * pitch - slave_mapping.to_distance(k) - 0.5);
            const std::optional<unsigned> h_lower =
              slave_mapping.to_index(std::floor(_h_lower));

            if (h_lower) {
                sum_s00h_in_H +=
                  lef::create_binary_sum(0,
                                         0,
                                         slave_mapping.to_distance,
                                         binary_sum::SUM,
                                         {},
                                         h_lower);
                //callback(s_iik + sum_s00h_in_H <= 1);
            }
            if (h_lower || h_start) {
                callback(s_iik + sum_s00h_in_H <= 1);
            }
        }
    }
}

void
slave_constraint_generator::diameter_definition(callback callback)
{
    linear_expression x = lef::create_x_representant();
    linear_expression d_0 = lef::create_diameter_variable(0, 0);

    /// d_0 >= x_0
    callback(d_0 >= x);
    callback(d_0 >= -1.0 * x);

    for (unsigned i = 0; i < f.dim1; i++) {
        for (unsigned j = i; j < f.dim1; j++) {
            linear_expression lower =
              lef::create_binary_sum(
                i, j, slave_mapping.to_distance, binary_sum::DISTANCE) -
              0.5;
            linear_expression upper =
              lef::create_binary_sum(
                i, j, slave_mapping.to_distance, binary_sum::DISTANCE) +
              0.5;

            linear_expression d_ij = lef::create_diameter_variable(i, j);
            callback(d_ij >= lower);
            callback(d_ij <= upper);
        }
    }
}

void
slave_constraint_generator::diameter_inequality(callback callback)
{
    assert_that(!rounded_down_non_diameters || !*rounded_down_non_diameters,
                "Cannot combine diameter_equality with diameter_inequality");
    rounded_down_non_diameters = false;
    for (unsigned i = 0; i < f.dim1; i++) {
        linear_expression d_ii = lef::create_diameter_variable(i, i);
        for (unsigned j = i + 1; j < f.dim1; j++) {
            linear_expression d_jj = lef::create_diameter_variable(j, j);
            linear_expression d_ij = lef::create_diameter_variable(i, j);

            callback(d_ii + d_jj - 1.0 <= 2.0 * d_ij);
            callback(2.0 * d_ij <= d_ii + d_jj + 1.0);
        }
    }
}

void
slave_constraint_generator::diameter_equality(callback callback)
{
    assert_that(!rounded_down_non_diameters || *rounded_down_non_diameters,
                "Cannot combine diameter_equality with diameter_inequality");
    rounded_down_non_diameters = true;
    for (unsigned i = 0; i < f.dim1; i++) {
        linear_expression d_ii = lef::create_diameter_variable(i, i);
        for (unsigned j = i + 1; j < f.dim1; j++) {
            linear_expression d_jj = lef::create_diameter_variable(j, j);
            linear_expression d_ij = lef::create_diameter_variable(i, j);

            callback(2.0 * d_ij == d_jj + d_ii);
        }
    }
}

void
slave_constraint_generator::quadratic_definition(callback callback)
{
    for (unsigned i = 0; i < f.dim1; i++) {
        linear_expression squared_sum = lef::create_binary_sum(
          i,
          i,
          [&](unsigned i) { return slave_mapping.to_distance(i); },
          binary_sum::SQUARED_DISTANCE);
        linear_expression sum = lef::create_binary_sum(
          i,
          i,
          [&](unsigned i) { return slave_mapping.to_distance(i); },
          binary_sum::DISTANCE);
        linear_expression d_ii = lef::create_diameter_variable(i, i);

        linear_expression lower_bound = squared_sum - 0.5 * sum - 0.5 * d_ii;
        linear_expression upper_bound = squared_sum + 0.5 * sum + 0.5 * d_ii;
        linear_expression q_ii = lef::create_quadratic_variable(i);
        callback(lower_bound <= q_ii);
        callback(q_ii <= upper_bound);
    }
}

void
slave_constraint_generator::quadratic_equality(callback callback)
{
    linear_expression q_0 = lef::create_quadratic_variable(0);
    linear_expression x = lef::create_x_representant();
    for (unsigned i = 1; i < f.dim1; i++) {
        linear_expression q_i = lef::create_quadratic_variable(i);
        linear_expression q_i_value =
          q_0 + (i * 2.0 * pitch) * (-2.0 * x + i * 2.0 * pitch);
        callback(q_i == q_i_value);
    }
}

void
slave_constraint_generator::binary_sum_to_one(callback callback)
{
    for (unsigned i = 0; i < f.dim1; i++) {
        for (unsigned j = i; j < f.dim2; j++) {
            linear_expression sum = lef::create_binary_sum(
              i, j, slave_mapping.to_distance, binary_sum::SUM);
            callback(sum == 1.0);
        }
    }
}
