#!/usr/bin/env python

import launch
import copy
import os
import sys
import pickle

"""
Generates tables from configs lists.
"""
class table_generator:
    out = sys.stdout
    names = ["Simplex", "Dual Simplex", "Barrier"]

    """
    Generates the header with the names of the runs
    """
    def generate_header(self, first_row):
        #first column is empty in header, as it contains the name of the used algorithm
        self.out.write("\\hline\n&")
        self.out.write("&".join(self.names))
        self.out.write("\\\\\n\\hline\n")

    def generate_one_row(self, row, rowname):
        #first column is empty in header, as it contains the name of the used algorithm
        self.out.write(str(rowname) + "&")
        self.out.write("&".join([str(round(run.stats.user_time, 2))+" s." for run in row]))
        self.out.write("\\\\\n")

    def generate_latex_begin(self):
        self.out.write("\\begin{tabular}{\n");
        self.out.write("c".join(["|" for i in range(len(self.names) + 2)]))
        self.out.write("\n}\n");

    def generate_latex_end(self):
        self.out.write("\\hline\n\\end{tabular}\n");

    def generate(self, configs):
        solvers = []
        for i in range(3):
            solvers.append([c for c in configs if "--master_solver " + str(i) in c.extra_args])

        if len(solvers[0]) != len(solvers[1]) or len(solvers[1]) != len(solvers[2]):
            print(solvers)
            raise BaseException("Not all solvers where executed!")

        self.generate_latex_begin()

        self.generate_header(solvers[0])
        for runs in zip(*solvers):
            if [runs[0].run_name()] * 3 != [r.run_name() for r in runs]:
                raise BaseException("Wrong order of configs? :" + str([r.run_name() for r in runs]))
            self.generate_one_row(runs, runs[0].run_name())

        self.generate_latex_end()

def main():
    t = table_generator()
    for f in sys.argv[1:]:
        with open(f,"rb") as i:
            t.generate(pickle.load(i))

if __name__ == "__main__":
    main()

