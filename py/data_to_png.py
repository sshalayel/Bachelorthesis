#!/usr/bin/env python

import numpy as np
import matplotlib.pyplot as plt
import sys

stdargv =["4", "3","one_reflector_multiple_heights/50_fmc.log_16.cgdump.bin"];

argv_use = """
Please use """ + sys.argv[0] + """ ref_file elements sender receiver file
where:
    elements    : #elements
    e           : plot until elementpair (e,e)
    files       : all files to be plotted

or just """ + sys.argv[0] + """ without arguments for standard arguments """ + sys.argv[0] + " " + " ".join(stdargv) + """

"""

def main():
    args = sys.argv

    if len(args) == 1:
        args += stdargv

    if len(args[1:]) < len(stdargv):
        print(argv_use)
    else:
        e = int(args[1])
        maxe = int(args[2])
        for i in range(3, len(args)):
            for s in range(maxe):
                for r in range(s + 1, maxe):
                    plot_and_save(args[i], e, s, r)

def plot_and_save(f, e, s, r):
    a = np.fromfile(f, dtype=np.double)

    name = f + "_" +str(s) +"_" + str(r)
    plt.gcf().canvas.set_window_title(name)
#increase those values to change the image resolution !(in inches)
    plt.gcf().set_size_inches((10,5))

    ha, = plt.plot(a.reshape((e,e,-1))[s,r,:],'g', linewidth=0.5, label=name)
    plt.legend(handles=[ha])
    plt.savefig(name + ".png", dpi=300)
    plt.close(plt.gcf())


if __name__ == "__main__":
    main()
