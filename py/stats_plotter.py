#!/usr/bin/env python

import matplotlib.pyplot as plt
import numpy as np
import sys
import os

"""
Plots graphs with 2 different y-Axes. Has nothing to do with the LP-Dual.
"""
class dual_plotter:
    def __init__(self, fig, axes, title):
        self.handles = {}
        self.a = axes
        self.fig = fig

        self.fig.canvas.set_window_title(title)
        self.fig.suptitle(title)

    def plot_on(self, data, label, color, a, below_bars=False, over_bars=None, auto_twin=True):
        try:
            current_a = self.a[a] if not auto_twin or a in self.handles else self.a[a].twinx()
        except:
            current_a = self.a if not auto_twin or a in self.handles else self.a.twinx()

        if below_bars:
            h = current_a.bar(range(len(data)), data, label=label, color=color);
        elif over_bars is not None:
            h = current_a.bar(range(len(data)), data, label=label, color=color, bottom=over_bars);
        else:
            h, = current_a.plot(data, label=label, color=color);
        current_a.tick_params(axis='y', labelcolor=color)
        self.handles.setdefault(a, []).append(h)

    def legend(self):
        for ax, handles in self.handles.items():
            try:
                self.a[ax].legend(handles=handles)
            except:
                self.a.legend(handles=handles)


class master_stats:
    def __init__(self, input_file):
        self.table = np.loadtxt(input_file, delimiter=';', usecols=(0,1))
        self.objective_column = 0
        self.run_time_column = 1

        self.input_file = os.path.basename(input_file.rstrip("/"))
        self.accumulate_time = False

    def plot(self):
        fig, a = plt.subplots(1,1)
        dp = dual_plotter(fig, a, "Master-stats of " + self.input_file)

        dp.plot_on(self.table[:, self.objective_column], "Objective", "blue", 0)

        data = self.table[:, self.run_time_column]
        if self.accumulate_time:
            data = np.cumsum(data)

        dp.plot_on(data, "Time needed in seconds by master", "orange", 0)
        dp.legend()
        return dp

class two_row_plotter:
    def __init__(self, input_file):
        self.table = np.loadtxt(input_file, delimiter=';', usecols=(0,1))
        self.master_column = 0
        self.slave_column = 1
        self.input_file = os.path.basename(input_file.rstrip("/"))

    def plot(self):
        fig, a = plt.subplots(1,1)
        dp = dual_plotter(fig, a, self.title)

        dp.plot_on(self.table[:, self.master_column], self.a_legend, "blue", 0, below_bars=self.below_bars, auto_twin=self.auto_twin)
        dp.plot_on(self.table[:, self.slave_column], self.b_legend, "orange", 0, over_bars=self.over_bars, auto_twin=self.auto_twin)

        dp.legend()
        return dp

class cuts_stats(two_row_plotter):
    def __init__(self, input_file):
        two_row_plotter.__init__(self, input_file)

        self.title = "Times needed for cut-choosing (" + self.input_file + ")"
        self.a_legend = "Time needed by greedy cut chooser"
        self.b_legend = "Time needed by ILP cut chooser"
        self.below_bars=None
        self.auto_twin=True
        self.over_bars=None

class times_stats(two_row_plotter):
    def __init__(self, input_file):
        two_row_plotter.__init__(self, input_file)

        self.title = "Times needed by iteration for " + self.input_file
        self.a_legend = "Time needed by Master in seconds"
        self.b_legend = "Time needed by (possibly already running) Slaves in minutes"
        self.below_bars=True
        self.auto_twin=False
        self.over_bars=self.table[:, self.master_column]


class slave_stats:
    def __init__(self, input_file):
        self.iteration_column = 0
        self.objective_column = 1
        self.run_time_column = 2

        self.table = np.loadtxt(input_file, delimiter=';', usecols=(0,1,2))

        self.input_file = os.path.basename(input_file.rstrip("/"))

        self.remove_duplicate_objectives()
        self.split_per_iteration()

        self.iteration_wise = {}
        self.accumulate_time = False

    def remove_duplicate_objectives(self):
        #TODO: np.unique messes columns up
        pass

    def split_per_iteration(self):
        _, self.solutions_per_iteration = np.unique(self.table[:, self.iteration_column], return_counts=True)

        self.iterations = np.split(self.table, indices_or_sections=np.cumsum(self.solutions_per_iteration[:-1]), axis=0)

    def plot(self):
        #fig, a = plt.subplots(3,1)
        fig, a = plt.subplots(1,1)
        dp = dual_plotter(fig, a, "Slave-stats of " + self.input_file)

       #dp.plot_on([x.shape[0] for x in self.iterations], "Solutions found", "green", 0)
       #dp.plot_on([np.amax(a[:, self.run_time_column]) for a in self.iterations], "Time needed in minutes by slave", "orange", 0)

       #dp.plot_on([np.amax(a[:, self.objective_column]) for a in self.iterations], "Maximum Objective", "blue", 1)
       #dp.plot_on([np.amin(a[:, self.objective_column]) for a in self.iterations], "Minimum Objective", "red", 1)

        data = [np.amax(a[:, self.run_time_column]) for a in self.iterations]
        if self.accumulate_time:
            data = np.cumsum(data)
        dp.plot_on(data , "Seconds between starting slave and finding a solution", "orange", 2)
        dp.plot_on([np.amax(a[:, self.objective_column]) for a in self.iterations], "Maximum Objective", "blue", 2)

        dp.legend()
        return dp

def run(filelist, accumulate_time):
    l = []
    for arg in filelist:
        if arg.endswith(".slavestats"):
            ss = slave_stats(arg)
            ss.accumulate_time = accumulate_time
            l.append(ss.plot())
        elif arg.endswith(".masterstats"):
            ms = master_stats(arg)
            ms.accumulate_time = accumulate_time
            l.append(ms.plot())
        elif arg.endswith(".times"):
            ts = times_stats(arg)
            ts.accumulate_time = accumulate_time
            l.append(ts.plot())
        elif arg.endswith(".cutstats"):
            ts = cuts_stats(arg)
            ts.accumulate_time = accumulate_time
            l.append(ts.plot())
    return l

if __name__ == "__main__":
    l = run(sys.argv[1:], False)
    plt.show()
    for db in l:
        plt.close(db.fig)

