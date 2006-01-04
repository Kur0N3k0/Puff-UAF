#!/bin/sh
error_file="test09.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

# make a regional data set 30x30 degrees from a global one
# things run faster when not using the entire global data set, although
# this in not obvious with this test since our original data is already sparse
../src/puff -lonLat 50/50 -model avn -eruptDate "2002 09 25 00:00" -regionalWinds 30 -saveHours 24 -rcfile ../etc/puffrc > /dev/null 2>$error_file

if test -s $error_file; then
  exit 1
fi
rm $error_file
rm 200209260000_ash.cdf
exit 0
