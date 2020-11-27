#ifndef GRB_LINEAR_EXPRESSION_H
#define GRB_LINEAR_EXPRESSION_H

#include "gurobi_c++.h"
#include "linear_expression.h"
#include "slave_problem.h"

/// A visitor that converts an linear_expression into GRB equivalent classes.
struct grb_linear_expression_visitor
{
    slave_problem::variables<GRBVar> variables;

    /// Visits a double.
    GRBLinExpr operator()(constant c);
    /// Visits a variable, taken from values.
    GRBLinExpr operator()(variable v);
    /// Visits a variable, taken from values.
    GRBLinExpr operator()(binary_sum s);
    /// Visits a binary expression recursively.
    GRBLinExpr operator()(binary_linear_expression b);
    /// Visits an linear_expression.
    GRBLinExpr operator()(linear_expression le);

    /// Main entry point for constraint conversion, calls recursively the other operator()-overloads.
    GRBTempConstr operator()(linear_expression_constraint e);
};

#endif
