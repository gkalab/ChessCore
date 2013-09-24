#!/bin/sh
#
# From: http://stackoverflow.com/questions/462100/bash-script-to-create-symbolic-links-to-shared-libraries
#
#set -x

for baselib in "$@"
do
    shortlib=$baselib
    while extn=$(echo $shortlib | sed -n '/\.[0-9][0-9]*$/s/.*\(\.[0-9][0-9]*\)$/\1/p')
          [ -n "$extn" ]
    do
        shortlib=$(basename $shortlib $extn)
        dir=$(dirname $baselib)
        (cd $dir; ln -sf $(basename $baselib) $(basename $shortlib) )
    done
done
