#!/bin/bash
# the simple benchmark on ormh 40 to see if implementation runs faster after implementing stuff...

FOLDER=benchmarks

source ./pandora_env.sh
source ./launcher.sh

launch ./configs/benchmark.csv ./configs/benchmark_4x4.csv
exit $?