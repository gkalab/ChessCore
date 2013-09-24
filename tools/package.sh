#!/bin/bash
#
# Package up ccore and associated support files for distribution.
# (assumes the binary has just been built and src/Version.ver is accurate).
#

dir=`(cd $(dirname $0); pwd)`
parent=$(dirname $dir)

if [ ! -x $parent/build/ccore ]; then
    echo $0: $parent/build/ccore does not exist
    exit 1
fi

version=$(grep version $parent/src/Version.ver | cut -f2 -d' ')

tarfile=~/tmp/ccore${version}.tar.gz
rm -f $tarfile

files="doc/ccore.md build/ccore"
if [ -f build/ccore.bin ]; then
    files="$files build/ccore.bin build/libChessCore.*"
fi

(cd $parent; tar cfz $tarfile $files test) || exit 2
echo Files written successfully to $tarfile
exit 0

