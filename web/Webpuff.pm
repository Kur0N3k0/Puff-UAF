package Webpuff;
require Exporter;

use File::Basename;

@ISA = qw(Exporter);
@EXPORT = qw(%ashxpOptionDefault %puffOptionDefault %puffScrollOptionDefault %puffOptionStatic $runDir);

# assume the top install directory is one above the one we are in and
# no trailing slash
$home="/home/josh/puff";

# MODIFY HERE FOR WEB SERVER INSTALLATIONS -----------
# there are three possible directory roots that may need to be modified if webpuff
# is installed on an active webserver and not being run locally
# this is where all the executables, documentation, etc are.  Often 
# in some users
# directory, for example the PREFIX in ./configure --prefix=PREFIX
$home_exe=$home;
# For a web server install, this is were the webpages are, does not 
# include apache's DocumentRoot, but needs an initial slash, so
# something like '/webpuff' would make sense.  No trailing slash.
# For a localhost install, use simply "." since minihttpd is run from 
# within the 'web' directory.  No leading or trailing slashes or else
# unexpected behavior can result; like loading of random web pages
$home_web=".";
# the cgi's need to know the absolute location of the webpages so things like
# 'glob()' work
$home_abs="$home/web";
# For binary installations, the udunits.dat file location needs to be
# specified.  Any copy at any location will work.  If you built from source,
# you probably don't need to specify this.
#$ENV{UDUNITS_PATH}="/home/rorik/unidata/etc/udunits.dat";

# END OF WEB SERVER INSTALLATION CHANGES ----------

$pidfile="$home_exe/etc/webpuff.pid";
$puff_exe="$home_exe/bin/puff ";
$ashxp_exe="$home_exe/bin/ashxp ";
$ashgmt_exe="$home_exe/bin/ashgmt --verbose --movie=movie.gif ";

# This is the directory in which all sessions are stored, relative to this dir.
# for a localhost install, you probably want simply
$runDir="run";
# 20060615 I'm not sure we need the following directive ever.
# for a web-server install, add the web_home
#$runDir="$home_web/run";

# these are all default values exported to the various frames
# these are ONLY used for options which can be changed within the gui
# or added as 'hidden' input fields.  Use static values below otherwise
$puffOptionDefault{ashLogMean}=-6;
$puffOptionDefault{ashLogSdev}=1;
$puffOptionDefault{diffuseH}=10000;
$puffOptionDefault{diffuseZ}=10;
$puffOptionDefault{eruptHours}=3;
$puffOptionDefault{eruptDate}=get_today();
#$puffOptionDefault{eruptDate}="2002 09 25 00:00";
$puffOptionDefault{nAsh}=2000;
$puffOptionDefault{phiDist}="";
$puffOptionDefault{plumeMin}=0;
$puffOptionDefault{plumeMax}=16000;
$puffOptionDefault{plumeHwidth}=0;
$puffOptionDefault{plumeZwidth}=3;
$puffOptionDefault{quiet}="true";
$puffOptionDefault{regionalWinds}=30;
$puffOptionDefault{restartFile}="none";
$puffOptionDefault{runHours}=24;
$puffOptionDefault{saveHours}=6;
# setting the default scroll options to "" will default to the first value
# in the list.  Lists are populated from within runParams by reading puffrc
# file. (plumeShape is currently static)
$puffOptionDefault{model}="";
$puffOptionDefault{dem}="";
$puffOptionDefault{plumeShape}="";

#static options are part of the command line no matter what changes are made
# within the gui.
$puffOptionStatic{quiet}="true";
$puffOptionStatic{averageOutput}=true;
$puffOptionStatic{gridOutput}=true;
$puffOptionStatic{gridSize}="0.5x2000";
$puffOptionStatic{repeat}="5";

$ashxpOptionDefault{airborne}=0;
$ashxpOptionDefault{fallout}=0;
$ashxpOptionDefault{grayscale}=0;
$ashxpOptionDefault{gridlines}=10;
$ashxpOptionDefault{"no-stamp"}=0;
$ashxpOptionDefault{magnify}=1;
$ashxpOptionDefault{pixels}=3;
$ashxpOptionDefault{projection}="mercator";
$ashxpOptionDefault{color}="rainbow";
# format options are GIF JPEG BLOCK CONTOUR
$ashxpOptionDefault{format}="GIF";

#---------------------
sub get_today {
  my ($mday, $mon, $year);
  (undef,undef,undef,$mday,$mon,$year,undef,undef,undef)=gmtime;
  $year += 1900;
  $mon +=1;
  $mon = "0$mon" if (length $mon == 1);
  $mday = "0$mday" if (length $mday == 1);
  
  return "$year $mon $mday 00:00";
}


1;
