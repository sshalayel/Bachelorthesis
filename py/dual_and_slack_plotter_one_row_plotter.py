#!/usr/bin/env python

import dual_and_slack_plotter as dasp
import sys

def main():
    d = dasp.dual_and_slack_plotter(sys.argv[1:])
    d.plot(3)

if __name__ == "__main__":
    main()
