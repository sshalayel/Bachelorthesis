#!/usr/bin/bash

function plot_reference {
    ./data_reader.py data/simulated/one_reflector_multiple_heights/50_fmc.bin 0 1 16 3
}

BASEFILE="data/simulated/one_reflector_multiple_heights/50_fmc.bin"
BASEFILE_ARGS=" 0 1 16 3 "

function plot_downsampled {
ELEMENTS=16 STEPSIZE=$1 python ./simple_down_sampling.py "$BASEFILE" &&
    ./data_reader.py "${BASEFILE}_downsampled_${1}.bin" 0 1 $ELEMENTS 3
}

f