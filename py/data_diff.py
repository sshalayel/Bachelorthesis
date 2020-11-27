#!/usr/bin/env python

import numpy as np
import matplotlib.pyplot as plt
import sys

stdargv = ["data/simulated/one_reflector_multiple_heights/4x4_50_fmc.bin", "4", "289", "one_reflector_multiple_heights/50_fmc.log_16.cgdump.bin"];

argv_use = """
Please use """ + sys.argv[0] + """ ref_file elements sender receiver file1 file2 ...
where:
    ref_file    : dataset to plot
    elements    : #elements
    offset      : shift the first plot
    files       : dataset to plot

or just """ + sys.argv[0] + """ without arguments for standard arguments """ + sys.argv[0] + " " + " ".join(stdargv) + """

"""

def main(one_against_all = True, norm = True):
    args = sys.argv

    if len(args) == 1:
        args += stdargv

    if len(args[1:]) < len(stdargv):
        print(argv_use)
    else:
        file1 = args[1];
        e = int(args[2])
        offset = int(args[3])

        if one_against_all:
            for i in range(4, len(sys.argv)):
                a = np.fromfile(file1, dtype=np.double)
                b = np.fromfile(args[i], dtype=np.double)

                for s in range(e):
                    plot_and_save(file1, args[i], a, b, e, s, s, norm, offset)
        else:
            forerunner = file1
            for i in range(5, len(sys.argv)):
                a = np.fromfile(file1, dtype=np.double)
                b = np.fromfile(args[i], dtype=np.double)

                for s in range(e):
                    plot_and_save(forerunner, args[i],a,b, e, s, s, norm,offset)
                forerunner = args[i]


def norm(d, n):
    if n:
        m = np.max([abs(np.max(d)), abs(np.min(d))])
        d *= 100
        d /= m
    return d

def plot_and_save(file1, file2, a, b, e, s, r, n, offset):

    shifted_a = np.zeros(offset + len(a)//(e *e))
    shifted_a[offset:] = a.reshape((e,e,-1))[s,r,:]

    plt.gcf().canvas.set_window_title(file2)
    #increase those values to change the image resolution! (in inches)
    plt.gcf().set_size_inches((14,7))

    ha, = plt.plot(norm(shifted_a, n),'g', linewidth=0.5, label=file1)
    hb, = plt.plot(norm(b.reshape((e,e,-1))[s,r,:], n),'r', linewidth=0.5, label=file2)
    plt.legend(handles=[ha, hb])
    #f, ax = plt.subplots(2)

    #ax[0].plot(a.reshape((e,e,-1))[s,r,:])
    #ax[0].set_title(file1)
    #ax[1].plot(b.reshape((e,e,-1))[s,r,:])
    #ax[1].set_title(file2)

    #plt.show();
    plt.savefig(file2 + "_diff_" + str(s) + "_" + str(r)+ ".png", dpi=300)
    plt.close(plt.gcf())


if __name__ == "__main__":
    main()
