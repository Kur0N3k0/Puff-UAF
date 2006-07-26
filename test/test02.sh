#!/bin/sh
# basic puff usage
error_file="test02.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

thisdir=`pwd`;
PUFFHOME=$thisdir/..
export PUFFHOME

../src/puff -volc spurr -eruptDate "2006 07 25 06:00" -model gfs -rcfile ../etc/puffrc -saveAshInit -runHours 6 -saveHours 6 > /dev/null 2>$error_file
if test -s $error_file; then
  exit 1
fi
rm 2006*_ash.cdf
rm $error_file
exit 0
