#!/usr/bin/perl
use CGI;

my $q=new CGI;

# set sessionID and runDir
my $sessionID = $q->param('sessionID');
my $runDir = $q->param('runDir');

print $q->header;
print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\"
   \"http://www.w3.org/TR/html4/frameset.dtd\">";
print "<HTML>";

print "<head>";
print "<title>WebPuff</title>";
print "<link rel='shortcut icon' href='pix/puff_dragon.ico'>";
print "</head>";
print '<frameset rows="350,*">';
  print "<frame src=\"runParams.pl?sessionID=$sessionID\" name=runParams  frameborder=0>";
  print '<frameset cols="150,480">';
    print "<frame src=\"ashxpOptions.pl\" name=ashxpOptions frameborder=0>";
    print "<frame src=\"$Webpuff::home_web/pix/puff_bg.gif\" name=imageFrame frameborder=0>";
  print "</frameset>";
print "</frameset>";

print "</html>";
