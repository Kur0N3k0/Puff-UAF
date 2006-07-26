#!/bin/sh
# test CMC regional-gridded data
error_file="test06.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

thisdir=`pwd`;
PUFFHOME=$thisdir/..
export PUFFHOME

# run it with regional wind and convert geopotential height on the fly
../src/puff -model nam216 -eruptDate "200607140000" -volc spurr  -runHours 6 -rcfile ../etc/puffrc > /dev/null 2>$error_file

if test -s $error_file; then
  exit 1
fi
rm 2006*ash.cdf
rm $error_file
exit 0
