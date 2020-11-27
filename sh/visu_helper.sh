#!/bin/bash

VISU=./build9/visu

function bitmap {
    echo " ==== Creating Bitmap ====" &&
    "$VISU" $*
}

function pdf {
    [ -z "$WITH_LINES" ] && ADD_LINES="" || ADD_LINES="--linetex"
    [ -z "$WITH_LINES" ] && FILES="${*/%/.tex}" || FILES="${*/%/.lines.tex}"
    #[ -z "$WITH_CENTERS" ] && ADD_LINES="${ADD_LINES} --hints " || { export FILES="${FILES//.tex/.center.tex}"; ADD_LINES="${ADD_LINES} --center "; }
    [ -z "$WITH_CENTERS" ] && ADD_LINES="${ADD_LINES} " || { export FILES="${FILES//.tex/.center.tex}"; ADD_LINES="${ADD_LINES} --center "; }

    TEMP=temp
    mkdir -p "$TEMP"
    echo " ==== Generating tikz-code for the PDF ====" &&
    echo "$VISU" --tex $ADD_LINES $* &&
    "$VISU" --tex $ADD_LINES $* &&
    echo " ==== Compiling the PDF ====" &&
    for i in $FILES
    do
        DIR="$(dirname "$i" 2>>/dev/null)"
        IS_FILE="$?"
        [ "$IS_FILE" -eq 0 ] &&
            {
                FILE="$(basename "$i")" &&
                pdflatex --output-directory "$TEMP" "$i" &>/tmp/pdfvisu.log &&
                mv "${TEMP}/${FILE/%tex/pdf}" "${DIR}/${FILE/%tex/pdf}" || return 1
            }
        #dvilualatex --output-directory "$TEMP" "$i" &&
        #dvisvgm -o "${TEMP}/${FILE/%tex/svg}" "${TEMP}/${FILE/%tex/dvi}" &&
        #mv "${TEMP}/${FILE/%tex/svg}" "${DIR}/${FILE/%tex/svg}"
    done
}
