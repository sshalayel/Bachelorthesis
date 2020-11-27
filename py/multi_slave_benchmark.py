#!/usr/bin/env python

"""
Tests how much Multiple Concurrent Slaves help.
"""

import launch
import copy
import os
import sys

def multiple_tricks(c):
    l = []
    for combination in [1,2,4,6,8,10]:
        cpy = copy.deepcopy(c)
        cpy.extra_args += "--master_threshold 20000 --slaves " + str(combination) + " "
        cpy.output_file += "_slaves_" + str(combination)
        l.append(cpy)
    return l

os.environ.setdefault("FOLDER", "callback_benchmarks")
os.environ.pop("SAFT", "")
os.environ.setdefault("MAX_COLUMNS", "400")

executions = launch.launch(files=["configs/50_ormh.csv_downsampled_8.csv"], extra_config_generator=multiple_tricks)
