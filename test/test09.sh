#!/bin/sh
error_file="test09.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

thisdir=`pwd`;
PUFFHOME=$thisdir/..
export PUFFHOME

# make a regional data set 30x30 degrees from a global one
# things run faster when not using the entire global data set, although
# this in not obvious with this test since our original data is already sparse
../src/puff -lonLat 50/50 -model gfs -eruptDate "2006 07 25 06:00" -regionalWinds 30 -saveHours 24 -rcfile ../etc/puffrc > /dev/null 2>$error_file

if test -s $error_file; then
  exit 1
fi
rm $error_file
rm 2006*ash.cdf
exit 0
