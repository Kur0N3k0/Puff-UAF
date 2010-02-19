#!/usr/bin/perl -I.


use CGI qw(:all);
use Webpuff;
use strict;

my $q = new CGI;

# get the process ID file from the main application
my $pidfile = $Webpuff::pidfile;
# the webpuff process id
my $pid = `cat $pidfile` if (-e $pidfile);
# only try to kill it if it makes some sense
if ($pid =~ /\d/) {
   `kill $pid`;
   `rm -f $pidfile`;
  } 

# a single good-bye page that closes all the frames
print $q->header;
print $q->start_html(-title=>"Exit Webpuff",
                     -head=>$q->meta({-http_equiv => 'refresh',
		        -content => "1,http://puff.images.alaska.edu"}),
 			);
print $q->h1("goodbye");

  print $q->end_html(); 
  exit(0);
 
