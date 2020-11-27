#!/usr/bin/env python

import numpy as np
import data_reader
import math
import sys
import os
import scipy.signal as sig

"""
Script that downsamples a given file.
"""
class simple_down_sampling:
    def __init__(self, stepsize = os.environ.get("STEPSIZE", None), elements = os.environ.get("ELEMENTS", None), offset  = os.environ.get("OFFSET", None)):
        self.stepsize = int(stepsize) if stepsize is not None else None
        self.offset = int(offset) if offset is not None else 0
        self.elements = int(elements)
        self.data = None
        self.resized_data = None
        self._decimate = True
    
    def read(self, input_file):
        """ Read from file """
        self.data = data_reader.load_data(input_file).reshape((self.elements, self.elements, -1))
        self.input_file = input_file

    def round_n_up_until_dividible(self, n, divisor):
        return math.ceil(float(n)/divisor) * divisor

    """
    Creates the resized_data that is easier to downsample, pads every measurement with 0.
    """
    def resize_for_stepsize(self):
        assert(self.data is not None)
        resized_shape = self.data.shape[:-1]
        resized_shape += (self.round_n_up_until_dividible(self.offset + self.data.shape[-1], self.stepsize),)
        self.resized_data = np.zeros(resized_shape)

        self.new_offset = self.offset // self.stepsize

        self.resized_data[:,:,self.offset:self.offset + self.data.shape[-1]] = self.data

    """
    Splits resized_data into buckets of stepsize.
    """
    def prepare_buckets(self):
        assert(self.resized_data is not None)

        steps = self.resized_data.shape[-1]//self.stepsize
        new_shape = (self.elements, self.elements, steps, self.stepsize)
        self.buckets = self.resized_data.reshape(new_shape)

    def down_sample(self):
        self.resize_for_stepsize()
        self.prepare_buckets()
        self.result = self.buckets.sum(axis=-1)[:,:,self.new_offset:self.new_offset + self.data.shape[-1]]

    def decimate(self):
        self.new_offset = self.offset // self.stepsize
        self.result = sig.decimate(self.data, self.stepsize, ftype='fir', axis=-1)

    """
    Pads the data of input_file with 0s until its size is a multiple of stepsize.
    Then, sums all stepsize many entries together to get the entries of the down_sampled array.
    The result is saved into output_file as binary file.
    """
    def to(self, input_file=None, output_file=None):
        if input_file is not None:
            self.read(input_file)
        else:
            assert(self.data is not None)

        if self._decimate:
            if output_file is None:
                output_file = self.input_file+"_decimate_"+str(self.stepsize)+".bin"
            self.decimate()
        else:
            if output_file is None:
                output_file = self.input_file+"_downsampled_"+str(self.stepsize)+".bin"
            self.down_sample()
        self.result.tofile(output_file)

        return output_file

def main():
    sds = simple_down_sampling()
    assert(sds.elements is not None)
    if sds.stepsize is None:
        #print("Specify STEPSIZE and ELEMENTS via environment variable please.")
        for current_file in sys.argv[1:]:
            sds.read(current_file)
            for stepsize in range(10, 20):
                sds.stepsize = stepsize
                sds.to()
        return
    for current_file in sys.argv[1:]:
        sds.to(current_file, current_file+"_downsampled_"+str(sds.stepsize)+".bin")

if __name__ == "__main__":
    main()
