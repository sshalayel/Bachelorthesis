#!/usr/bin/env python

import launch
import copy
import os

"""
Creates a config with and one without master_solution_threshold.
"""
def mst_adder(c):
    l = []
    for mst in [1e-5, 0]:
        cpy = copy.deepcopy(c)
        cpy.extra_args += " --master_solution_threshold " + str(mst) + " "
        if mst != 0:
            cpy.output_file += "_mst" + str(mst)
        l.append(cpy)
    return l

"""
Runs some configs with and without master clean (master_solution_threshold).
"""
def main():
    os.environ["FOLDER"] = "benchmarks"
    os.environ["SAFT"] = "0.5"
    launch.launch([
        "./configs/20_ormh.csv",
        "./configs/benchmark.csv",
        ], mst_adder)

if __name__ == "__main__":
    main()
