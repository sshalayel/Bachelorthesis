#!/usr/bin/env python

import launch
import copy
import os
import sys
import subprocess
import re

class tangent_config(launch.config):
    def __init__(self, exec_path, input_file="", current_saft=""):
        launch.config.__init__(self, exec_path, input_file, current_saft)

    """
    Executes the current run and fill stats.
    """
    def execute(self):
        output_file = self.output_file
        for step in [0.1, 0.2, 0.3, 0.4, 0.5, 0.75, 1, 2, 5]:
            os.environ["TANGENT_STEP"] = str(step)
            self.output_file = output_file + ".tangent_step_"  + str(step)
            return_status = launch.config.execute(self)
        return return_status


os.environ.setdefault("FOLDER", "tangent_steps")
os.environ.pop("SAFT", "")

executions = launch.launch(config_implementation=tangent_config)
