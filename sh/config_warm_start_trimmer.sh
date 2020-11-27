#!/bin/bash

function trim {
    for i in $*
    do
        head -n "$COLUMN_LIMIT" "$i" >> "$(dirname "$i")/${COLUMN_LIMIT}_$(basename "$i")"
    done
}

for COLUMN_LIMIT in {100..900..100}
do
    trim $*
done
