#!/usr/bin/env python

import sys
from matplotlib import pyplot as plt
from stats_plotter import run

if __name__ == "__main__":
    l = run(sys.argv[1:], True)
    plt.show()
    for db in l:
        plt.close(db.fig)

