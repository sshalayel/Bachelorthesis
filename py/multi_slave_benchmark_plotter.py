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

class multi_slave_plotter:
    def __init__(self, configs):
        digits = tuple(map(str, range(10)))
        self.configs = list(filter(lambda x: x.is_file() and x.name.endswith(digits), os.scandir(configs)))
        self.configs.sort(key=self.extract_slaves)
        self.slaves = list(map(self.extract_slaves, self.configs))
        self.run_time = list(map(self.compute_total_runtime, self.configs))

    def plot(self):
        fig, ax = plt.subplots()
        #handles = [ax.scatter(ignored_solution, run_time, label=step) for ignored_solution, run_time, step in zip(self.ignored_solutions, self.run_time, self.steps)]
        #fig.legend(handles=handles)
        width = 0.3
        x_axis = np.arange(len(self.run_time))
        handles = []
        #handles += ax.bar(x_axis - width/2, self.run_time, color="blue", width=width)
        handles += ax.plot(self.run_time, 'bo-')

        ax.set_xticks(range(len(self.slaves)))
        ax.set_xticklabels(self.slaves)
        ax.set_ylabel("Solving time in seconds", color="blue")
        ax.set_yscale('log')
        ax.set_xlabel("Number of Concurrent Subproblems")

        fig.suptitle("Runtime of CG when varying the number of subproblems.")
        return fig

    def extract_slaves(self, config_file: os.DirEntry):
        splitted = config_file.name.split("_")
        #print(f"splitted :{splitted} obtained from {config_file.name}")
        return int(splitted[-1])

    def compute_total_runtime(self, config_file):
        times = np.loadtxt(config_file.path + ".times", delimiter=';', usecols=(0,1))
        return times.sum()

def main():
    for i in sys.argv[1:]:
        tsp = multi_slave_plotter(i)
        tsp.plot()
    plt.savefig("multi_slave_benchmark_plot.png", dpi=300)
    plt.show()

if __name__ == "__main__":
    main()