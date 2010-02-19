#!/bin/sh

set -x 
# not really a test, but downloads necessary data files if the do not exist
datfiles="GLOBALeb10colshade.jpg  GLOBALeb3colshade.jpg  GLOBALeb6colshadesmall.jpg"
wget_log="wget.log"
if (test -d ../etc/images); then
  :
else
  mkdir -m 755 ../etc/images
fi
cd ../etc/images
if test -z /usr/bin/wget; then
  echo "cannot find wget" >> $wget_log
else
# if these data files do not exist, use wget to retrieve them, logging errors
  for file in $datfiles; do
    if test -r $file; then
      :
    else
    echo "retrieving image file $file"
    /usr/bin/wget http://puff.images.alaska.edu/download/images/$file -a $wget_log
    fi
  done
fi
missing_files=""
for file in $datfiles; do
  if test -r $file; then
    :
  else
    missing_files="$missing_files $file"
  fi
done
if (test $missing_files); then
	echo "You need to download background images file for ashxp."
  echo "I tried using 'wget' but failed (see 'etc/images/wget.log').  You can"
  echo "manually download these files from"
  echo "http://puff.images.alaska.edu/download/ into the etc/images/ directory"
  echo "and run these tests again.  The missing files are"
  echo "$missing_files"
  exit 1
fi
exit 0
