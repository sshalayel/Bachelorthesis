#!/usr/bin/env python

import matplotlib.pyplot as plt
import pickle
import launch
import sys
import numpy as np
import math

"""
Generates plots from configs list.
"""

class msb_plotter:
    def __init__(self, configs, sizes=[25,50, 75] + [i*100 for i in range(1,10)]):
        rest = len(configs) % 3 
        if (rest != 0):
            print("Ignoring the last " + str(rest) + " entries...")

        self.configs = configs[:-rest] if rest > 0 else configs
        self.names = {0:"Simplex", 1:"Dual Simplex", 2:"Barrier"}
        self.colors = {0:"green", 1:"orange", 2:"blue"}
        self.sizes = sizes


        self.split_configs()

    """
    Splits a list of configs into a new list L where L[size][solver] = configs for solver solver and size size.
    """
    def split_configs(self):
        self.split_by_size()
        self.split_by_solver()

    def split_by_size(self):
        self.configs =  [[c for c in self.configs if str(size) + "_"  in c.output_file] for size in self.sizes]

    def split_by_solver(self):
        self.configs = [
                [
                    [
                        c for c in splitted_config if "--master_solver " + str(i) in c.extra_args
                    ] for i in self.names.keys()
                ] for splitted_config in self.configs
            ]

    def plot(self):
        figs = []
        for i, configs_by_size in enumerate(self.configs):
            if (len(configs_by_size[0]) == 0):
                continue
            width = 0.2
            fig, ax = plt.subplots()

            ax.set_ylabel("minutes")
            figtitle = "Different methods solving a masterproblem with " + str(self.sizes[i]) + " simulated columns"
            fig.suptitle(figtitle)

            ###print("Doing " + figtitle + " for " + ",\n\n".join([
            ###    "(" + ",\n".join([
            ###        c.input_file + c.extra_args for c in x
            ###    ]) + ")" for x in configs_by_size
            ###]))

            x_axis = np.arange(len(configs_by_size[0]))

            #times = [[math.log(1.0 + (c.stats.user_time + c.stats.system_time)/60) for c in x] for x in configs_by_size]
            times = [[((c.stats.user_time + c.stats.system_time)/60) for c in x] for x in configs_by_size]

            handles = [ax.bar(x_axis + (i - 1) * width, times[i], width, label=self.names[i], color=self.colors[i]) for i in range(len(self.names))]

            xticklabels = [chr(x) for x in range(ord('A'), ord('A') + x_axis.shape[0])]
            ax.set_xticks(x_axis)
            ax.set_xticklabels(xticklabels)
            ax.set_yscale('log')

            if (len(handles) > 0):
                fig.legend(handles=handles)
            figs.append(fig)
        return figs


def main():
    mean = True
    if mean:
        configs = []
        for arg in sys.argv[1:]:
            with open(arg, "rb") as i:
                configs+= pickle.load(i)
        meaned_configs = launch.config.mean(
                filter(
                lambda x: "--slow_warm_start" in x.extra_args and "--no_warm_start_values" in x.extra_args,
                configs
                )
            )

        p = msb_plotter(meaned_configs)
        print(p.sizes)
        for i, fig in enumerate(p.plot()):
            fig.set_size_inches((10,5))
            filename = "mean_" +str(p.sizes[i])+".png"
            print("Saving to " + filename)
            fig.savefig(filename, dpi=300)
            plt.close(fig)

    else:
        for arg in sys.argv[1:]:
            with open(arg, "rb") as i:
                l = []
                p = msb_plotter(pickle.load(i))
                l = p.plot()
                for i, fig in enumerate(l):
                    fig.set_size_inches((10,5))
                    filename = arg + "_" +str(i)+".png"
                    print("Saving to " + filename)
                    fig.savefig(filename, dpi=300)
                    plt.close(fig)

if __name__ == "__main__":
    main()

