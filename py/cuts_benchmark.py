#!/usr/bin/env python

"""
Tests how much the Cuts are actually helping.
"""

import launch
import copy
import os
import sys

def multiple_tricks(c):
    l = []
    for combination in ["", "slave_cuts", "compare_slave_cuts"]:
        cpy = copy.deepcopy(c)
        cpy.extra_args += " --" + str(combination) + " "
        cpy.output_file += "_" + str(combination).replace(" ", "_")
        l.append(cpy)
    return l

os.environ.setdefault("FOLDER", "cuts_benchmarks")
os.environ.pop("SAFT", "")
os.environ.setdefault("MAX_COLUMNS", "100")

executions = launch.launch(extra_config_generator=multiple_tricks)