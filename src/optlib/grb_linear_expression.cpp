#include "grb_linear_expression.h"

GRBLinExpr
grb_linear_expression_visitor::operator()(constant c)
{
    return 1.0;
}

GRBLinExpr
grb_linear_expression_visitor::operator()(binary_sum s)
{
    GRBLinExpr ret = 0;

    const unsigned sum_start =
      std::min(s.start.value_or(0), variables.values.dim3);
    const unsigned sum_end =
      std::min(s.end.value_or(variables.values.dim3), variables.values.dim3);
    for (unsigned k = sum_start; k < sum_end; k++) {
        switch (s.t) {
            case binary_sum::SUM:
                ret += variables.values.at(s.i, s.j, k);
                break;
            case binary_sum::DISTANCE:
                ret += s.to_distance(k) * variables.values.at(s.i, s.j, k);
                break;
            case binary_sum::SQUARED_DISTANCE:
                ret += std::pow(s.to_distance(k), 2) *
                       variables.values.at(s.i, s.j, k);
                break;
        }
    }
    return ret;
}

GRBLinExpr
grb_linear_expression_visitor::operator()(variable v)
{
    switch (v.t) {
        case variable::BINARY:
            return variables.values(v.i, v.j, v.k);
        case variable::DIAMETER:
            return variables.diameter_values(v.i, v.j);
        case variable::QUADRATIC:
            return variables.quadratic_values(v.i);
        case variable::X_REPRESENTANT:
            return variables.representant_x;
        default:
            assert(false);
            return variables.representant_x; // make compiler happy
    }
}

GRBLinExpr
grb_linear_expression_visitor::operator()(binary_linear_expression v)
{
    GRBLinExpr lhs = operator()(v.lhs);
    GRBLinExpr rhs = operator()(v.rhs);
    switch (v.op) {
        case binary_linear_expression::ADD:
            return lhs + rhs;
        case binary_linear_expression::SUB:
            return lhs - rhs;
        default:
            assert(false);
    }
    // will never be reached but removes the compiler warning.
    return 0;
}

GRBLinExpr
grb_linear_expression_visitor::operator()(linear_expression le)
{
    return le->coefficient * std::visit(*this, le->v);
}

GRBTempConstr
grb_linear_expression_visitor::operator()(linear_expression_constraint le)
{
    GRBLinExpr lhs = operator()(le.lhs);
    GRBLinExpr rhs = operator()(le.rhs);

    switch (le.op) {
        case linear_expression_constraint::LESS_EQUAL:
            return lhs <= rhs;
        case linear_expression_constraint::EQUAL:
            return lhs == rhs;
        case linear_expression_constraint::GREATER_EQUAL:
            return lhs >= rhs;
        default:
            assert(false && "Implement me!");
            return lhs >= rhs; // for compiler warning
    }
}
