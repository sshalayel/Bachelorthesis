#include "linear_expression.h"

linear_expression
operator+(linear_expression lhs, linear_expression rhs)
{
    return lef::create_binary(lhs, binary_linear_expression::ADD, rhs);
}

linear_expression
operator-(linear_expression lhs, linear_expression rhs)
{
    return lef::create_binary(lhs, binary_linear_expression::SUB, rhs);
}

linear_expression
operator+(linear_expression lhs, double rhs)
{
    return lef::create_binary(
      lhs, binary_linear_expression::ADD, lef::create_constant(rhs));
}

linear_expression
operator-(linear_expression lhs, double rhs)
{
    return lhs + (-rhs);
}

linear_expression
operator+=(linear_expression& lhs, linear_expression rhs)
{
    return lhs = lhs + rhs;
}

linear_expression_constraint
operator>=(linear_expression lhs, linear_expression rhs)
{
    return lef::create_comparison(
      lhs, linear_expression_constraint::GREATER_EQUAL, rhs);
}

linear_expression_constraint
operator==(linear_expression lhs, linear_expression rhs)
{
    return lef::create_comparison(
      lhs, linear_expression_constraint::EQUAL, rhs);
}

linear_expression_constraint
operator<=(linear_expression lhs, linear_expression rhs)
{
    return lef::create_comparison(
      lhs, linear_expression_constraint::LESS_EQUAL, rhs);
}

linear_expression_constraint
operator>=(linear_expression lhs, double rhs)
{
    linear_expression c = lef::create_constant(rhs);
    return lhs >= c;
}

linear_expression_constraint
operator==(linear_expression lhs, double rhs)
{
    linear_expression c = lef::create_constant(rhs);
    return lhs == c;
}

linear_expression_constraint
operator<=(linear_expression lhs, double rhs)
{
    linear_expression c = lef::create_constant(rhs);
    return lhs <= c;
}

linear_expression operator*(double lhs, linear_expression rhs)
{
    return linear_expression{ new linear_expression_variant{
      rhs->v,
      rhs->coefficient * lhs,
    } };
}

linear_expression
lef::create_binary_sum(unsigned i,
                       unsigned j,
                       binary_sum::mapping to_distance,
                       binary_sum::type type,
                       std::optional<unsigned> start,
                       std::optional<unsigned> end)
{
    return linear_expression{
        new linear_expression_variant{
          binary_sum{
            type,
            i,
            j,
            to_distance,
            start,
            end,
          },
        },
    };
}

linear_expression
lef::create_binary_variable(unsigned i, unsigned j, unsigned k)
{
    return linear_expression{ new linear_expression_variant{ variable{
      variable::BINARY,
      i,
      j,
      k,
    } } };
}

linear_expression
lef::create_diameter_variable(unsigned i, unsigned j)
{
    return linear_expression{ new linear_expression_variant{ variable{
      variable::DIAMETER,
      i,
      j,
      0,
    } } };
}

linear_expression
lef::create_quadratic_variable(unsigned i)
{
    return linear_expression{ new linear_expression_variant{ variable{
      variable::QUADRATIC,
      i,
      0,
      0,
    } } };
}

linear_expression
lef::create_x_representant()
{
    return linear_expression{ new linear_expression_variant{ variable{
      variable::X_REPRESENTANT,
    } } };
}

linear_expression
lef::create_constant(double d)
{
    return linear_expression{ new linear_expression_variant{
      constant{},
      d,
    } };
}

linear_expression
lef::create_binary(linear_expression lhs,
                   binary_linear_expression::operation op,
                   linear_expression rhs)
{
    return linear_expression{ new linear_expression_variant{
      binary_linear_expression{ lhs, op, rhs },
    } };
}

void
linear_expression_constraint::dump(std::ostream& os)
{
    dump_linear_expression dle{ os };
    dle(*this);
}

void
linear_expression_variant::dump(std::ostream& os)
{
    dump_linear_expression{ os }(linear_expression(this));
}

linear_expression_constraint
lef::create_comparison(linear_expression lhs,
                       linear_expression_constraint::operation op,
                       linear_expression rhs)
{
    return linear_expression_constraint{ lhs, op, rhs };
}
void
dump_linear_expression::operator()(const linear_expression_constraint e)
{
    operator()(e.lhs);
    switch (e.op) {
        case linear_expression_constraint::EQUAL:
            out << " = ";
            break;
        case linear_expression_constraint::GREATER_EQUAL:
            out << " >= ";
            break;
        case linear_expression_constraint::LESS_EQUAL:
            out << " <= ";
            break;
        default:
            assert(false);
    }
    operator()(e.rhs);
    out << std::endl;
}
void
dump_linear_expression::operator()(const linear_expression e)
{
    if (e->coefficient == 0.0) {
        out << "0";
        return;
    }
    if (e->coefficient != 1.0) {
        out << e->coefficient << "*( ";
    }
    std::visit(*this, e->v);
    if (e->coefficient != 1.0) {
        out << " )";
    }
}
void
dump_linear_expression::operator()(const binary_linear_expression e)
{
    operator()(e.lhs);
    switch (e.op) {
        case binary_linear_expression::ADD:
            out << " + ";
            break;
        case binary_linear_expression::SUB:
            out << " - ";
            break;
        default:
            assert(false);
    }
    operator()(e.rhs);
}

void
dump_linear_expression::operator()(const binary_sum e)
{
    out << "\\sum_{" << e.start.value_or(0)
        << " <= k <= " << (e.end ? std::to_string(*e.end) : "dim3") << "} ";
    switch (e.t) {
        case binary_sum::SUM:
            out << "b_{" << e.i << ", " << e.j << ", k}";
            break;
        case binary_sum::DISTANCE:
            out << "(k \\cdot b_{" << e.i << ", " << e.j << ", k})";
            break;
        case binary_sum::SQUARED_DISTANCE:
            out << "(k^2 \\cdot b_{" << e.i << ", " << e.j << ", k})";
            break;
        default:
            assert(false);
    }
}

void
dump_linear_expression::operator()(const variable e)
{
    switch (e.t) {
        case variable::QUADRATIC:
            out << "q_{" << e.i << "}";
            if (values) {
                out << "(" << std::pow(values->tof.at(e.i, e.i), 2) << ")";
            }
            break;
        case variable::DIAMETER:
            out << "d_{" << e.i << ", " << e.j << "}";
            if (values) {
                out << "(" << values->tof.at(e.i, e.j) << ")";
            }
            break;
        case variable::BINARY:
            out << "b_{" << e.i << ", " << e.j << ", " << e.k << "}";
            if (values) {
                out << "("
                    << (values->tof.at(e.i, e.j) == values->to_distance(e.k)
                          ? 1.0
                          : 0.0)
                    << ")";
            }
            break;
        case variable::X_REPRESENTANT:
            out << "x";
            if (values && values->tof.representant_x) {
                out << "(" << *values->tof.representant_x << ")";
            }
            break;
        default:
            assert(false);
    }
}

void
dump_linear_expression::operator()(const constant e)
{
    out << 1.0;
}
