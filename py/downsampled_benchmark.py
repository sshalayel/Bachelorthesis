#!/usr/bin/env python

import launch
import os

os.environ.setdefault("FOLDER", "benchmarks")
os.environ.setdefault("SAFT", "0.5")
os.environ.setdefault("MAX_COLUMNS", "400")
launch.launch([
    "./configs/30_ormh.csv_downsampled_32.csv",
    "./configs/30_ormh.csv_downsampled_16.csv",
    "./configs/30_ormh.csv_downsampled_8.csv",
    ])
