#!/usr/bin/env python

"""
Runs the benchmark files until optimality.
"""

import launch
import os

os.environ["FOLDER"] = "benchmarks_until_optimal"
os.environ["SAFT"] = "0.5"
os.environ.setdefault("EXTRA_ARGS","")
os.environ["EXTRA_ARGS"] += " --saft_resolution 2000 "
os.environ["MAX_COLUMNS"] = "1000000"
launch.launch([
    "./configs/benchmark.csv",
    "./configs/benchmark_4x4.csv",
    ])
