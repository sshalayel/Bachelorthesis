#!/bin/bash
# The code used to launch optimisation that copies the binary (so i can run other commits in parallel), computes the passed time with time, executes with and without saft, saves the results with date + used commit.

#allows to modify single_run by scripts calling this script
SINGLE_RUN=single_run

[ -z $FOLDER ] && FOLDER="."

function single_run {
    echo "Running $MAX_COLUMNS Columns, saft set to $current_saft, slavestop is $SLAVESTOP on $config, extra args is $EXTRA_ARGS"
    echo "Running $MAX_COLUMNS Columns, saft set to $current_saft, slavestop is $SLAVESTOP on $config, extra args is $EXTRA_ARGS" >> "$timelog"
    date &>> "$timelog"
    { time  "$EXEC_PATH" -C "$configfile" -o "$outputfile" --slavestop "$SLAVESTOP" --max_columns "$MAX_COLUMNS" --saft "$current_saft" $EXTRA_ARGS; }  &>> "$timelog"
        #extraargs is not in "!!!
    echo >> "$timelog"
    echo >> "$timelog"
}

function launch {
    CURRENT_COMMIT="$(date --rfc-3339=seconds | tr ' ' -)-$(git rev-parse HEAD)"
    OUTPUT_DIR="./${FOLDER}/${CURRENT_COMMIT}"
    EXEC_PATH="./${FOLDER}/${CURRENT_COMMIT}/EXEC"
    [ -z $SLAVESTOP ] && SLAVESTOP=10000
    [ -z $MAX_COLUMNS ] && MAX_COLUMNS=100

    mkdir -p "$OUTPUT_DIR"

    MAKE || return 1

    cp ./build/opt "$EXEC_PATH"

    echo "Using directory $OUTPUT_DIR"
    timelog="${OUTPUT_DIR}/time"
    for configfile in $*
    do
        # $SAFT not quoted so it disappears when empty
        for current_saft in $SAFT 1
        do
            config=$(basename "$configfile")
            [ $current_saft == 1 ] && outputfile="${OUTPUT_DIR}/${config}_log" || outputfile="${OUTPUT_DIR}/${config}_warm"
            $SINGLE_RUN
        done
    done
}
