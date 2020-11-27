#!/usr/bin/env python

import launch
import copy
import os
import sys

"""
Loads the config as slave_warm_start.
"""
def warm_slaves(c):
    c.extra_args += " --slave_warm_start " + c.input_file
    c.input_file = ""
    return [c]

os.environ["FOLDER"] = "slave_warm_starts"
os.environ.pop("SAFT", "")
os.environ["MAX_COLUMNS"] = "10"

executions = launch.launch(extra_config_generator=warm_slaves)
