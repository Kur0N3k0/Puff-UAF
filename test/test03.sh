#!/bin/sh
# multiple eruptions from the same volcano
error_file="test03.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

thisdir=`pwd`;
PUFFHOME=$thisdir/..
export PUFFHOME

../src/puff -volc cleveland -eruptDate "2006 07 25 06:00" -model gfs -eruptHours 0.5 -saveHours 6 -runHours 6 -model gfs -rcfile ../etc/puffrc > /dev/null 2>$error_file
../src/puff -volc cleveland -eruptDate "2006 07 25 12:00" -model gfs -eruptHours 0.5 -saveHours 6 -runHours 6 -restartFile "200607251200_ash.cdf" -model gfs -rcfile ../etc/puffrc > /dev/null 2>>$error_file
../src/puff -volc cleveland -eruptDate "2006 07 25 18:00" -model gfs -eruptHours 0.5 -saveHours 6 -runHours 6 -restartFile "200607251800_ash.cdf" -model gfs -rcfile ../etc/puffrc > /dev/null 2>>$error_file
../src/puff -volc cleveland -eruptDate "2006 07 26 00:00" -model gfs -eruptHours 0.5 -saveHours 6 -runHours 6 -restartFile "200607260000_ash.cdf"  -model gfs -rcfile ../etc/puffrc > /dev/null 2>>$error_file
if test -s $error_file; then
  exit 1
fi
rm 2006*_ash.cdf
rm $error_file
exit 0
