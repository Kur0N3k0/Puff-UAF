#!/bin/sh
# basic puff usage
error_file="test02.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

../src/puff -volc spurr -eruptDate "2002 09 25 00:00" -model avn -rcfile ../etc/puffrc -saveAshInit -runHours 6 -saveHours 6 > /dev/null 2>$error_file
if test -s $error_file; then
  exit 1
fi
rm 2002092[56]????_ash.cdf
rm $error_file
exit 0
