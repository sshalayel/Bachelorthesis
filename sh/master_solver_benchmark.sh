#!/bin/bash

source ./pandora_env.sh
source ./launcher.sh

#replaces the launcher-single-run
function multiple_solver {
    old_output_file="$outputfile"
    for solver in 0 1 2
    do
        outputfile="${old_output_file}_${solver}"
        EXTRA_ARGS="--master_solver $solver --no_warm_start_values"
        single_run
    done
}

SINGLE_RUN=multiple_solver
FOLDER=master_solver_benchmarks
# no saft needed
SAFT=
MAX_COLUMNS=0

launch ./configs/master_solver_benchmarks/*
