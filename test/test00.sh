#!/bin/sh
# not really a test, but downloads necessary data files if the do not exist
datfiles="2006072506_gfs.nc 2006071400_nam216.nc"
wget_log="wget.log"
if (test -d ../data/); then
  :
else
  mkdir -m 755 ../data
fi
cd ../data/
if test -z /usr/bin/wget; then
  echo "cannot find wget" >> $wget_log
else
# if these data files do not exist, use wget to retrieve them, logging errors
  for file in $datfiles; do
    if test -r $file; then
      :
    else
    echo "retrieving data file $file"
    /usr/bin/wget http://puff.images.alaska.edu/download/$file -a $wget_log
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
  echo "You need to download data files to run these tests."
  echo "I tried using 'wget' but failed (see 'data/wget.log').  You can"
  echo "manually download these files from"
  echo "http://puff.images.alaska.edu/download/ into the data/ directory"
  echo "and run these tests again.  The missing files are"
  echo "$missing_files"
  exit 1
fi
exit 0
