#!/bin/sh

dest=$1
shift
paths=$*
externs=`ldd $paths |
    awk '/.* => \/usr\/lib\// {print $3;}' |
    sort |
    grep -v libmir |
    uniq`

echo "External libs:"
echo "~~~~~~~~~~~~~~~~~~~~"
ls $externs
echo "~~~~~~~~~~~~~~~~~~~~"

cp $externs $dest
