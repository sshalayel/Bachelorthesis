#ifndef LINEAR_EXPRESSION_H
#define LINEAR_EXPRESSION_H

#include "arr.h"
#include "coordinates.h"
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <variant>

struct linear_expression_variant;

///@brief Solver-agnostic linear_expression.
///
/// It can be transformed into a concrete value or into a corresponding solver-framework linear_expression.
/// For example, for gurobi use the grb_linear_expression_visitor.
using linear_expression = std::shared_ptr<linear_expression_variant>;

/// Represents a linear expression (used for building objectives and constraints).
struct binary_linear_expression
{
    enum operation
    {
        ADD,
        SUB,
    };
    linear_expression lhs;
    operation op;
    linear_expression rhs;
};

/// A double-constant.
struct constant
{};

/// A variable.
struct variable
{
    enum type
    {
        BINARY,
        DIAMETER,
        QUADRATIC,

        X_REPRESENTANT,
    } t;
    /// Only defined for BINARY, DIAMETER, QUADRATIC.
    unsigned i;
    /// Only defined for BINARY and DIAMETER.
    unsigned j;
    /// Only defined for BINARY.
    unsigned k;
};

/// A summation of binary variables.
struct binary_sum
{
    enum type
    {
        SUM,
        DISTANCE,
        SQUARED_DISTANCE
    } t;

    unsigned i;
    unsigned j;
    /// maps indexes k to distances
    using mapping = std::function<double(unsigned)>;
    mapping to_distance;

    /// Specifies a start when not starting summing up from 0.
    std::optional<unsigned> start;
    /// Specifies an end when not ending summing up at f.dim3.
    std::optional<unsigned> end;
};

/// A part of a linear_expression containing a factor in \RR and a linear subexpression. Use linear_expression instead of this.
struct linear_expression_variant
{
    /// The inner linear_expression part.
    std::variant<binary_linear_expression, constant, variable, binary_sum> v;
    /// Multiplied to v.
    double coefficient = 1.0;

    /// For debugging.
    void dump(std::ostream& os);
};

/// Represents a constraint linexp <= linexp, can be translated into another solver-framework-constraint.
struct linear_expression_constraint
{
    /// Differentiates the differenct constraint-types.
    enum operation
    {
        LESS_EQUAL,
        EQUAL,
        GREATER_EQUAL,
    };
    /// The Left-Hand-Side.
    linear_expression lhs;
    /// Relationship between lhs and rhs.
    operation op;
    /// The Right-Hand-Side.
    linear_expression rhs;
    /// For debugging.
    void dump(std::ostream& os);
};

/// Creating a linear_expression requires 2 extra constructor calls (smart pointer and variant), which are done here.
struct linear_expression_factory
{
    /// Creates a \sum_{k\in K} C\cdot b_ijk, where C is 1, k or k^2 depending on binary_sum::type.
    static linear_expression create_binary_sum(
      unsigned i,
      unsigned j,
      binary_sum::mapping to_distance,
      binary_sum::type type,
      std::optional<unsigned> start = {},
      std::optional<unsigned> end = {});

    /// Creates an object representing b_ijk.
    static linear_expression create_binary_variable(unsigned i,
                                                    unsigned j,
                                                    unsigned k);
    /// Creates an object representing d_ij.
    static linear_expression create_diameter_variable(unsigned i, unsigned j);
    /// Creates an object representing q_i.
    static linear_expression create_quadratic_variable(unsigned i);
    /// Creates an object representing x.
    static linear_expression create_x_representant();
    /// Creates an object representing a constant K.
    static linear_expression create_constant(double d);
    /// Creates an object representing `op(lhs, rhs)`.
    static linear_expression create_binary(
      linear_expression lhs,
      binary_linear_expression::operation op,
      linear_expression rhs);
    /// Creates an object representing `comparison_op(lhs, rhs)`.
    static linear_expression_constraint create_comparison(
      linear_expression lhs,
      linear_expression_constraint::operation op,
      linear_expression rhs);
};

/// This name was too long.
using lef = linear_expression_factory;

/// For ease of use, creates a new linear_expression.
linear_expression
operator+(linear_expression self, linear_expression other);

/// For ease of use, creates a new linear_expression.
linear_expression
operator+(linear_expression lhs, double rhs);

/// For ease of use, creates a new linear_expression.
linear_expression
operator-(linear_expression lhs, double rhs);

/// For ease of use, creates a new linear_expression.
linear_expression_constraint
operator<=(linear_expression self, linear_expression other);

/// For ease of use, creates a new linear_expression.
linear_expression_constraint
operator>=(linear_expression self, linear_expression other);

/// For ease of use, creates a new linear_expression.
linear_expression_constraint
operator==(linear_expression self, linear_expression other);

/// For ease of use, creates a new linear_expression.
linear_expression_constraint
operator<=(linear_expression self, double other);

/// For ease of use, creates a new linear_expression.
linear_expression_constraint
operator>=(linear_expression self, double other);

/// For ease of use, creates a new linear_expression.
linear_expression_constraint
operator==(linear_expression self, double other);

/// For ease of use, creates a new linear_expression.
linear_expression
operator-(linear_expression self, linear_expression other);

/// For ease of use, creates a new linear_expression.
linear_expression
operator+=(linear_expression& lhs, linear_expression rhs);

/// For ease of use, creates a new linear_expression.
linear_expression operator*(double lhs, linear_expression rhs);

/// Optional dependency for dump_linear_expression to dump the actual values of the variables.
struct dump_values
{
    /// The values of the variables. Binary and Quadratic are computed from it.
    time_of_flight& tof;
    /// Distance mapping.
    std::function<unsigned(unsigned)> to_distance;
};

/// ,,Pretty''-prints a given linear_expression, mostly needed for debugging purposes.
struct dump_linear_expression
{
    /// Where to dump.
    std::ostream& out;
    /// Optional values to dump with variables.
    std::optional<dump_values> values;
    /// Used by `std::visit` to recursively dump everything.
    void operator()(const linear_expression_constraint e);
    /// Used by `std::visit` to recursively dump everything.
    void operator()(const linear_expression e);
    /// Used by `std::visit` to recursively dump everything.
    void operator()(const binary_linear_expression e);
    /// Used by `std::visit` to recursively dump everything.
    void operator()(const variable e);
    /// Used by `std::visit` to recursively dump everything.
    void operator()(const binary_sum e);
    /// Used by `std::visit` to recursively dump everything.
    void operator()(const constant e);
};
#endif
