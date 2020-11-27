#!/bin/bash
# start optimisation using the launcher.sh-script

FOLDER=.

source ./pandora_env.sh
source ./launcher.sh

launch $*
exit $?
