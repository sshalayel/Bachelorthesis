#!/usr/bin/env python

import launch
import os

os.environ.setdefault("FOLDER", "benchmarks")
os.environ.setdefault("SAFT", "0.5")
os.environ.setdefault("MAX_COLUMNS", "400")
launch.launch([
    "./configs/benchmark.csv",
    "./configs/benchmark_4x4.csv",
    ])
