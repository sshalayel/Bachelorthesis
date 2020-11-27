#!/usr/bin/env python

import launch
import copy
import os
import sys
import subprocess
import re

__find_cg_dump_iteration = re.compile("_([0-9]*).cgdump")
def find_cgdump_iteration(filename):
    match = __find_cg_dump_iteration.search(filename)
    return int(match.group(1))

class pyramid_config(launch.config):
    def __init__(self, exec_path, input_file="", current_saft=""):
        launch.config.__init__(self, exec_path, input_file, current_saft)

    """
    Executes the current run and fill stats.
    """
    def execute(self):
        return_status = launch.config.execute(self)

        #get latest cgdump
        files = os.scandir(os.path.dirname(self.output_file))
        cgdumps = filter(lambda x: x.is_file() and x.name.startswith(os.path.basename(self.output_file)) and x.name.endswith(".cgdump"), files)
        cgdumps = filter(lambda x: not x.name.endswith("warm.cgdump"), cgdumps)
        #sorted_cgdumps = sorted(map(lambda x: x.path, cgdumps), reverse=True)
        sorted_cgdumps = sorted(map(lambda x: x.path, cgdumps), key=find_cgdump_iteration, reverse=True)
        cgdump = sorted_cgdumps[0]

        # retarget + change input_file
        retarget_bin = os.path.join("build8", "config_retarget") if "PANDORA" in os.environ else os.path.join("build9", "config_retarget")
        retarget_command = retarget_bin + " --to " + str(os.environ["PYRAMID_TO"]) + " " + str(cgdump)
        print(retarget_command)
        status = subprocess.run(retarget_command, universal_newlines=True, shell=True)

        #start as slavewarmstart!
        self.input_file = ""
        self.extra_args = " --slave_warm_start " + str(cgdump) + ".retargeted.cgdump" + str(self.extra_args)
        self.output_file += ".pyramid"


        return_status = launch.config.execute(self)

        return return_status


os.environ.setdefault("FOLDER", "pyramids")
os.environ.pop("SAFT", "")

os.environ["PYRAMID_TO"] = sys.argv[2]

executions = launch.launch(files=[sys.argv[1]], config_implementation=pyramid_config)