#!/bin/sh
# multiple eruptions from the same volcano
error_file="test03.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

../src/puff -volc cleveland -eruptDate "2002 09 25 00:00" -eruptHours 0.5 -saveHours 6 -runHours 6 -model avn -rcfile ../etc/puffrc > /dev/null 2>$error_file
../src/puff -volc cleveland -eruptDate "2002 09 25 06:00" -eruptHours 0.5 -saveHours 6 -runHours 6 -restartFile "200209250600_ash.cdf" -model avn -rcfile ../etc/puffrc > /dev/null 2>>$error_file
../src/puff -volc cleveland -eruptDate "2002 09 25 12:00" -eruptHours 0.5 -saveHours 6 -runHours 6 -restartFile "200209251200_ash.cdf" -model avn -rcfile ../etc/puffrc > /dev/null 2>>$error_file
../src/puff -volc cleveland -eruptDate "2002 09 25 18:00" -eruptHours 0.5 -saveHours 6 -runHours 6 -restartFile "200209251800_ash.cdf"  -model avn -rcfile ../etc/puffrc > /dev/null 2>>$error_file
if test -s $error_file; then
  exit 1
fi
rm 2002092[56]????_ash.cdf
rm $error_file
exit 0
