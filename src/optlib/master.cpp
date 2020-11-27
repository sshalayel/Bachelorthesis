#include "master.h"

master_problem::master_problem(bool verbose, std::ostream& output, solver s)
  : verbose(verbose)
  , output(output)
  , s(s)
{}

master_problem::~master_problem() {}
