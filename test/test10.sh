#!/bin/sh
error_file="test10.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

# use variable fall dynamics, which require a temperature field, CMC has it
# the avn set doesn't so one is created using a standard atmosphere
../src/puff -volc spurr -model cmc -eruptDate "2003 04 23 00:00" -sedimentation reynolds -saveHours 24 -rcfile ../etc/puffrc > /dev/null 2>$error_file
../src/puff -volc spurr -model avn -eruptDate "2002 09 25 00:00" -sedimentation reynolds -saveHours 24 -rcfile ../etc/puffrc > /dev/null 2>>$error_file

if test -s $error_file; then
  exit 1
fi
rm $error_file
rm 200304240000_ash.cdf
rm 200209260000_ash.cdf
exit 0
