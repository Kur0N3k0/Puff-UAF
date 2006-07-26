#!/bin/sh
error_file="test07.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

thisdir=`pwd`;
PUFFHOME=$thisdir/..
export PUFFHOME

# made a gridded concentration file of a 3-run average, only save the last
# files and made the grid size of the concentration file 
# 0.5 deg x 0.5 deg x 2000 meters
../src/puff -volc cleveland -eruptDate "2006 07 25 06:00" -model gfs -gridOutput=0.5x2000 -repeat 2 -averageOutput=true -rcfile ../etc/puffrc > /dev/null 2>$error_file
if test -s $error_file; then
  exit 1 
fi
rm 2006*ash*.cdf
rm 2006*conc*.nc
rm $error_file
exit 0
