#!/bin/sh
error_file="test08.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

thisdir=`pwd`;
PUFFHOME=$thisdir/..
export PUFFHOME

# use a custom-made cloud file to restart a simulation
../src/puff -restartFile example.cloud -eruptDate "2006 07 25 06:00" -saveHours 24 -model gfs -rcfile ../etc/puffrc > /dev/null 2>$error_file

# use a custom-made cloud file with another volcano
../src/puff -restartFile example.cloud -volc spurr -eruptDate "2006 07 25 06:00" -saveHours 24 -model gfs -rcfile ../etc/puffrc > /dev/null 2>$error_file

if test -s $error_file; then
  exit 1
fi
rm $error_file
rm 2006*ash.cdf
exit 0
