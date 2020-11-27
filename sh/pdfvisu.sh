#!/bin/bash

BASE="$(dirname $0)"
OLDCWD="$(pwd)"
cd "$BASE" && source ./visu_helper.sh
cd "$OLDCWD"
VISU="$BASE/build9/visu"
echo $(pwd)
make

WITH_CENTERS=1 pdf $* && WITH_CENTERS= pdf $*
