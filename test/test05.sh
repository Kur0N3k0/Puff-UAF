#!/bin/sh
# ashdump usage
error_file="test05.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

# create an ash file to work with
../src/puff -volc spurr -model avn -eruptDate "2002 09 25 00:00" -runHours 24 -saveHours 24 -rcfile ../etc/puffrc > /dev/null 2>$error_file
# do a regular dump
../src/ashdump 200209260000_ash.cdf > /dev/null 2>>$error_file
# do a range-sorted dump
../src/ashdump -range 200:210/50:70 -height 12000:13000 -size 1e-4:1e-5 200209260000_ash.cdf > /dev/null 2>$error_file

# perl-scripted ashdump.  Dump lat and height values, sorting by height in 
# reverse, with a header and height in feet
../src/perl/ashdump.pl --show-params --height-in-feet --reverse --var=lat,hgt --sort=hgt 200209260000_ash.cdf > /dev/null 2>$error_file

if test -s $error_file; then
  exit 1
fi
rm $error_file
rm 200209260000_ash.cdf
exit 0
