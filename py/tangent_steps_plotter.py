#!/usr/bin/env python

import matplotlib.pyplot as plt
import pickle
import launch
import sys
import numpy as np
import math
import subprocess
import stats_plotter
import os

"""
Generates plots from tangentSteps files
"""

class tangent_step_plotter:
    def __init__(self, configs):
        digits = tuple(map(str, range(10)))
        self.configs = list(filter(lambda x: x.is_file() and x.name.endswith(digits), os.scandir(configs)))
        self.configs.sort(key=lambda x: x.name)
        self.steps = list(map(self.extract_step, self.configs))
        self.ignored_solutions = list(map(self.count_ignored, self.configs))
        self.run_time = list(map(self.compute_total_runtime, self.configs))
        print(list(self.configs))
        print(list(self.steps))
        print(list(self.ignored_solutions))
        print(list(self.run_time))

    def plot(self):
        fig, ax = plt.subplots()
        #handles = [ax.scatter(ignored_solution, run_time, label=step) for ignored_solution, run_time, step in zip(self.ignored_solutions, self.run_time, self.steps)]
        #fig.legend(handles=handles)
        width = 0.3
        x_axis = np.arange(len(self.run_time))
        handles = []
        #handles += ax.bar(x_axis - width/2, self.run_time, color="blue", width=width)
        handles += ax.plot(self.run_time, 'bo-')

        ax.set_xticks(range(len(self.steps)))
        ax.set_xticklabels(self.steps)
        ax.set_ylabel("Solving time in seconds", color="blue")
        ax.set_xlabel("Tangent-step")

        fig.suptitle("Ignored Solutions and Time needed for different Tangent-steps")

        ax2 = ax.twinx()
        ax2.set_ylabel("Ignored solutions (out of 50)", color="orange")
        #handles += ax2.bar(x_axis + width/2, self.ignored_solutions, color="orange", width=width)
        handles += ax2.plot(self.ignored_solutions, '-o', color="orange")

        return fig

    def extract_step(self, config_file: os.DirEntry):
        splitted = config_file.name.split("_")
        #print(f"splitted :{splitted} obtained from {config_file.name}")
        return float(splitted[-1])

    def count_ignored(self, config_file):
        ret = subprocess.run("grep -ce '(LAZY-TANGENTS) Ignored GRB_CB_MIPSOL-solution' '" + config_file.path + "'", stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        #ret.check_returncode() # grep returns 1 when not found
        return int(ret.stdout)

    def compute_total_runtime(self, config_file):
        times = np.loadtxt(config_file.path + ".times", delimiter=';', usecols=(0,1))
        return times.sum()

    def compute_slave_runtime(self, config_file):
        master_column = 0
        slave_column = 1
        return self.compute_runtime(config_file, slave_column)

    def compute_master_runtime(self, config_file):
        master_column = 0
        slave_column = 1
        return self.compute_runtime(config_file, master_column)

    def compute_runtime(self, config_file, slave_or_master):
        times = np.loadtxt(config_file + ".times", delimiter=';', usecols=(0,1))
        return times[slave_or_master, :].sum()


def main():
    for i in sys.argv[1:]:
        tsp = tangent_step_plotter(i)
        tsp.plot()
    plt.savefig("tangent_steps_plot.png", dpi=300)
    plt.show()

if __name__ == "__main__":
    main()
