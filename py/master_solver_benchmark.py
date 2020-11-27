#!/usr/bin/env python

import launch
import copy
import os
import sys
#import master_solver_benchmark_table_builder as tb
#import master_solver_benchmark_plotter as msb_plotter
#import matplotlib.pyplot as plt

"""
Copies the configs for every solver method (simplex, dual simplex and barrier).
"""
def multiple_solvers(c):
    l = []
    for solver in range(3):
        cpy = copy.deepcopy(c)
        cpy.extra_args += " --master_solver " + str(solver) + " --no_warm_start_values --slow_warm_start"
        cpy.output_file += "_" + str(solver)
        l.append(cpy)

    cpy = copy.deepcopy(c)
    cpy.extra_args += " --no_warm_start_values "
    cpy.output_file += ".all_columns_at_once"
    l.append(cpy)
    return l

os.environ.setdefault("FOLDER", "master_solver_benchmarks")
os.environ.pop("SAFT", "")
os.environ.setdefault("MAX_COLUMNS", "0")

executions = launch.launch(extra_config_generator=multiple_solvers)

#msb = msb_plotter.msb_plotter(executions)
#fig = msb.plot()
#plt.savefig("master_solver_benchmark_plot.png", dpi=300)
#plt.close(fig)
#
#tb.table_generator().generate(executions)
