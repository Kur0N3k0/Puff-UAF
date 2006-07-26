#!/bin/sh
error_file="test10.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

thisdir=`pwd`;
PUFFHOME=$thisdir/..
export PUFFHOME

# use variable fall dynamics, which require a temperature field
# nam model has T field
# gfs does not
../src/puff -volc spurr -model nam216 -eruptDate "2006 07 14 00:00" -sedimentation reynolds -saveHours 24 -rcfile ../etc/puffrc > /dev/null 2>$error_file
../src/puff -volc spurr -model gfs -eruptDate "2006 07 25 06:00" -sedimentation reynolds -saveHours 24 -rcfile ../etc/puffrc > /dev/null 2>>$error_file

if test -s $error_file; then
  exit 1
fi
rm $error_file
rm 200*_ash.cdf
exit 0
