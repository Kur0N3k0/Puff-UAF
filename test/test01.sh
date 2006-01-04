#!/bin/sh
# use a digital elevation model 
error_file="test01.err"
PUFF_VOLCANO_LIST="../etc/volcanos.txt"
export PUFF_VOLCANO_LIST

# see if we have the DEM data file
dem_file="../data/gtopo30/W180N90.DEM"
dem_hdr="../data/gtopo30/W180N90.HDR"
if (test -r "$dem_file" && test -r "$dem_hdr"); then
  :
else
  echo "you need the GTOPO30 test files \"$dem_file\" and \"$dem_hdr\" to run this test.  You can get them at http://puff.images.alaska.edu/download/"
exit 0
fi

# do a short simulation that requires a DEM tile, longer run times would
# require more tiles.  Use GTOTO30 and decrement the DEM resolution by a factor
# of 4 to speed up the test
../src/puff -volc spurr -eruptDate "2002 09 25 00:00" -dem gtopo30:4 -runHours 6 -rcfile ../etc/puffrc > /dev/null 2>$error_file 

if test -s $error_file; then
  exit 1
fi

rm ./200209250600_ash.cdf
rm $error_file
exit 0
