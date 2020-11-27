#!/usr/bin/env python

import numpy as np
import matplotlib.pyplot as plt
import sys
import subprocess
import launch

"""
Plots Dual and Slack from one or multiple CGDUMP-files using simu.
"""
class dual_and_slack_plotter:
    def __init__(self, cgdumps):
        self.cgdumps = cgdumps
        self.original_data = {}
        self.original_data_elements = {}
        self.simus = None

    """
    Get the signal sparsely represented by CGDUMP.
    """
    def simu(self):
        launch.make()
        subprocess.run("./build/simu " + " ".join(self.cgdumps), shell=True)
        self.simus = [cgdump + ".bin" for cgdump in self.cgdumps]

    """
    Loads informations like number of elements and original_data from the CGDUMP.
    """
    def load_info_from(self, cgdump):
        with open(cgdump) as f:
            a = str(f.readline())
            while (a[0] == "#"):
                a = str(f.readline())
            self.original_data[cgdump] = a.split(";")[0]
            self.original_data_elements[cgdump] = int(a.split(";")[7])

    """
    Plots all files created by simu for all sender-receiver-pairs.
    """
    def plot(self, max_plot=None):
        if self.simus is None:
            self.simu()
        for cgdump, simu in zip(self.cgdumps, self.simus) :
            self.load_info_from(cgdump)
            elements = self.original_data_elements[cgdump]
            a = np.fromfile(simu, dtype=np.double).reshape(elements, elements, -1)
            original_a = np.fromfile(self.original_data[cgdump], dtype=np.double).reshape(elements, elements, -1)
            original_a *= 100 / original_a.max()
            count = 0

            for s in range(elements):
                for r in range(s, elements):
                    name = "_".join([cgdump, str(s), str(r)])
                    plt.gcf().canvas.set_window_title(name)
                    plt.gca().set_title(cgdump + " with (sender, receiver)-combination " + str((s,r)))
#increase those values to change the image resolution !(in inches)
                    plt.gcf().set_size_inches((10,5))

                    indexs = np.arange(a.shape[-1])

                    bars = True
                    if bars:
                        bar_width = 1.0
                        simu_handle = plt.bar(indexs, a[s,r,:], bar_width, label="Sparse representation from CGDUMP", color="green")
                        original_handle = plt.bar(indexs, original_a[s,r,:]-a[s,r,:], bar_width, bottom = a[s,r,:], label="Slack", color="red")
                    else:
                        simu_handle, = plt.plot(a[s,r,:], 'g', linewidth=0.5, label="Sparse representation from CGDUMP")
                        original_handle, = plt.plot(original_a[s,r,:], 'b', linewidth=0.5, label="Real measurement")
                        plt.legend(handles=[simu_handle, original_handle])
                    plt.savefig(name + ".png", dpi=300)
                    plt.close(plt.gcf())
                    count +=1
                    if max_plot is not None and max_plot < count:
                        break
                if max_plot is not None and max_plot < count:
                    break

def main():
    dasp = dual_and_slack_plotter(sys.argv[1:])
    dasp.plot()

if __name__ == "__main__":
    main()
