#!/usr/bin/env python

import numpy as np
import matplotlib.pyplot as plt
import sys
import scipy.signal as sig
import os

stdargv = ["data/simulated/9.04.19/model_Dvoie.bin", "12", "13", "16", "4"];
argv_use = """
Please use """ + sys.argv[0] + """ filename position positions elements maxX 
where:
    filename    : dataset to plot
    position    : plot at position
    positions   : #measured positions
    elements    : #sending elements
    maxX        : measurements to plot in x/y direction

or just """ + sys.argv[0] + """ without arguments for standard arguments """ + sys.argv[0] + " " + " ".join(stdargv) + """

"""

def main():
    args = sys.argv

    if len(args) == 1:
        args += stdargv

    if len(args[1:]) != len(stdargv):
        print(argv_use)
    else:
        file = args[1];
        pos = int(args[2])
        poss = int(args[3])
        el = int(args[4])
        x = range(int(args[5]))
        plot(file, pos, poss, el, x, x, decimate=os.environ.get("DECIMATE"))

""" Removes an XML_header ending in a specific tag. """
def remove_xml_header(a, ending=b'</LucidImage>'):
    header_end=np.frombuffer(ending, dtype=np.byte)
    header_len=len(header_end)
    arbitrary_offset = 4

    for i in range(len(a)):
        if (a[i:i+header_len] == header_end).all():
            return a[i + header_len + arbitrary_offset :]

""" Load data from Civa-csv, Matlab-dat and binary files """
def load_data(file):
    if file.endswith(".csv"):
        # civa format
        return np.loadtxt(file, dtype=np.double, usecols=(2,), delimiter=';')
    elif file.endswith(".dat"):
        # matlab "binary" format that contains an xml-header (as np.byte/char)
        a = np.fromfile(file, dtype=np.byte)
        # the rest of the data is actually in int16
        a = remove_xml_header(a).view(np.int16)
        if np.isnan(a).any():
            print("Data contains NaNs!")
        return a

    else:
        # simple binary format where everything is simply saved as doubles
        return np.fromfile(file, dtype=np.double)


""" Plot the data """
def plot(file, pos, poss, senders, x, y, symmetry_check=False, decimate=None):
    a = load_data(file)
    a = a.reshape((poss, senders, senders, -1))
    if decimate is not None:
        a = sig.decimate(a, int(decimate), ftype='fir', axis=-1)

    f, ax = plt.subplots(len(x),len(y))
    plt.suptitle(file)

    for i in x:
        for j in y:
            if (len(x) == 1 and len(y) == 1):
                current_ax = ax
            else:
                current_ax = ax[i,j]

            scatter = False
            if scatter:
                idx = np.arange(len(a))
                current_ax.scatter(idx, a.reshape((poss, senders, senders, -1))[pos,i,j,:])
            else:
                current_ax.plot(a.reshape((poss, senders, senders, -1))[pos,i,j,:])

            if symmetry_check and i != j:
                current_ax.plot(a.reshape((poss, senders, senders, -1))[pos,j,i,:])
    plt.show()


if __name__ == "__main__":
    main()
