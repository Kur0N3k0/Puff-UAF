#!/usr/bin/perl -w
#
# this script rereferences the time variable in NCEP reanalysis data to 
# smaller values that work with in Puff.  Normally, the values
# have units of "hours since 1-1-1".  This script makes them smaller by 
# referencing them to Jan 1 of the year the data is valid. For example, the
# file uwnd.1999.nc would have the time variable as 'hours since 1-1-1999"
#
# The NCO operators are required to use this scripts, get them from 
# nco.sourceforge.net
#
# There are a few checks before the file is overwritten, but you might want to
# make a copy of the first file you process in case something goes awry.
# USAGE:
# reference.pl <file>
#
# Rorik Peterson
# ffrap1@uaf.edu
#
# modifications
# Aug 22 2007
# less strict on the length of the time value, no longer only 8 digits

use File::Basename;

my $ncks_exe = "/usr/bin/ncks";
my $ncap_exe = "/usr/bin/ncap";
my $ncatted_exe = "/usr/bin/ncatted";

if (length($ARGV[0]) > 0)
{
  $infile = $ARGV[0];
} else {
  print "usage: rereference.pl <file>\n";
}

$outfile = "$infile.$$";

#extract the first time value
$result = `$ncks_exe -H -d time,0,0 -v time $infile`;
($time) = $result=~/time\[0\]=(\d+)/;
unless ($time > 0)
{
  print"failed to extract time value from result \"$result\"\nMaybe the file is already rereferenced.\n";
	exit;
}

# get the year of this data.  Assume name is [uv]wnd.????.nc
basename($infile) =~ /[uv]wnd\.(\d{4})\.nc/;
$y=$1;

# do arithmetic on the file
$result = `$ncap_exe -O -s \"time=time-$time\" $infile $outfile`;
if (length($result) > 0) { die $result; }

# edit the units attribute
$result = `$ncatted_exe -O -a units,time,c,c,\"hours since $y-1-1 00:00:0.0\" -a delta_t,time,c,c,\"0000-00-00 06:00:00" $outfile`;
if (length($result) > 0) { die $result; }

`mv -f $outfile $infile`;
print "done\n";
