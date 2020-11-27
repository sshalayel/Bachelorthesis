#!/usr/bin/env python

import numpy as np
import sys
import data_reader as dr

stdargv = ["data/simulated/9.04.19/model_Dvoie.bin", "16", "4", "1"];
argv_use = """
Please use """ + sys.argv[0] + """ filename elements elements_after_trimming positions
where:
    filename    : dataset to plot
    elements    : #sending and receiving elements in the input
    elements_after_trimming    : #sending and receiving elements in the output
    positions   : number of scanning positions (standard is 1)

or just """ + sys.argv[0] + """ without arguments for standard arguments """ + sys.argv[0] + " " + " ".join(stdargv) + """

"""

def out_name(file, out_elements):
    a = file.split('/')
    a[-1] = str(out_elements) + "x" + str(out_elements) +"_" + a[-1]
    out = '/'.join(a)
    print(out)
    return out

def main():
    args = sys.argv

    if len(args) == 1:
        args += stdargv

    if len(args[1:]) != len(stdargv):
        print(argv_use)
    else:
        file = args[1];
        elements = int(args[2])
        out_elements = int(args[3])
        positions = int(args[4])

        data = dr.load_data(file);
        data = data.reshape((1, elements, elements, -1));
        orig_shape = data.shape
        data = data[:,:out_elements,:out_elements,:]
        new_shape = data.shape
        data.tofile(out_name(file, out_elements))
        print("Changed shape ", orig_shape, " to new shape ", new_shape)

if __name__ == "__main__":
    main()
