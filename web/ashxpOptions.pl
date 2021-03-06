#!/usr/bin/perl -I.

use CGI qw(:all);
use Webpuff;
use File::Basename;
use strict;

my $q = new CGI;

# set sessionID and runDir
my $sessionID = $q->param('sessionID');
#my $runDir = $q->param('runDir');
my $previous = $q->param('previous');

# autoflush on
$|=1;

# all scroll options are a hash of arrays, see camel book 275
my %scrollOptions = (
  color				=> ["gray", "rainbow", "red", "isopach"],
	projection	=> ["mercator", "lambert", "polar"],
	format			=> ["GIF", "JPEG" ,"BLOCK", "CONTOUR"]
	);

my $formatLabels = {
	GIF	=> 'particle(GIF)',
	JPEG	=> 'particle(JPG)',
	BLOCK	=> 'conc(blocks)',
	CONTOUR=> 'conc(contours)'
	};

# parse options from previous form
my %ashxpOption;

if ($q->param('runashxp')) {
  show_updating();
  process_form($q);
} elsif ($q->param('refresh')) {
  if (lock_free() ) {
    create_form();
  } else {
    show_updating();
  }
} else {
  create_form();
} 
  
#------------------------
sub create_form {

  my %jpgLabels=();
  my $initial_image = "$Webpuff::home_web/pix/puff_bg.gif";
  my $update_label = "Map";
  
  #set defaults
  set_defaults();

  # load values from a previous run into the form  
  if ($q->param('reset')) {
    reset_form()
  } else {
  parse_previous($q->param('previous'));
  }
  my @jpgFiles = jpg_listing();

  foreach (@jpgFiles) {
    $_ =~ m/(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})*/;
    $jpgLabels{$_} = "$1 $2 $3 $4:$5";
  }

if (scalar @jpgFiles > 0) {
  $initial_image="$Webpuff::home_web/$runDir/$sessionID/$previous/$jpgFiles[0]";
  $update_label="Update";
} 

  print $q->header;
  print $q->start_html(-title=>"Display Options", 
                       -bgcolor=>"lightseagreen",
		       -head=>Link({-rel=>'stylesheet',
		                    -type=>'text/css',
				    -href=>"$Webpuff::home_web/webpuff.css"}),
                       -script=>{-language=>'JAVASCRIPT',
	                          -src=>"$Webpuff::home_web/javascript/webpuff.js"},
		       -onLoad=>"parent.imageFrame.location='$initial_image'",

	             );
		     
if (!$previous) {
  print $q->end_html;
  return;
}

(my $name, my $number) = ($previous =~ m/(\D*)(\d*)/);
$name = $previous unless ($number);
$name .= "($number)" if ($number);
#print '<table cellspacing="0" cellpadding="1">';
#print "<tr><td class=\"largeLabel\" colspan=\"2\">$name</td></tr>";
print "<h1 class=\"largeLabel\">$name</h1>";

# make the image selection a separate form, so when we update the options form
# this will default to no value.

# make this 2-column table entry a table itself with 2 columns, the images
# list on the left taking all rows, and the button images on the right, each
# to one row
  print '<table cellspacing="0" cellpadding="1">';
  print "<tr>";
  print '<td rowspan="2">';
print $q->start_form(-name=>"imageForm",
                     -method=>"GET"
		     );
  print $q->scrolling_list(-name=>'image',
                           -values=>[@jpgFiles],
			   -labels=>\%jpgLabels,
			   -size=>5,
			   -default=>$jpgFiles[0],
        -onChange=>"loadjpeg(parent.ashxpOptions.document.imageForm,parent.imageFrame, '$Webpuff::home_web/$runDir/$sessionID/$previous/')"
			 );
print $q->end_form;
  print "</td>"; # the image scroll list
  print "<td>"; # button 1	
  # only show a movie button for GIF files
  if ($ashxpOption{format} eq "GIF") {
  print "<a href=\"#\" onclick=\"loadmovie(parent.imageFrame, '$Webpuff::home_web/$runDir/$sessionID/$previous/movie.gif')\" ><img border='0' src='$Webpuff::home_web/pix/movie.png' alt=\"show movie\" /></a><br/>";
  }
  print "</td>"; # end of button 1
  print "</tr><tr>";
  print "<td>"; # button 2 since the scroll list takes up column 1
  print "<a href='map.pl'><img border='0' src=\"$Webpuff::home_web/pix/map-button.gif\" alt='map tool'/></a>";
  print "</td>"; # end button 2
  print "</tr>";
  print "</table>"; # end of small table holding image scroll list and buttons
#print "</td>";
#print "</tr>";

print $q->start_form(-name=>"ashxpOptionsForm",
                     -onSubmit=>"resetImage(parent.imageFrame,\"$Webpuff::home_web/pix/puff_bg.gif\")",
		     -method=>"GET"
		     );

#add hidden form values
print $q->hidden(-name=>'sessionID',-value=>$sessionID);
#print $q->hidden(-name=>'runDir',-value=>$runDir);
print $q->hidden(-name=>'previous',-value=>$previous);
print '<table cellspacing="0" cellpadding="1">';

print "<tr>";
print "<td class=\"labelText\">";
print "range[m]:";
print "</td><td>";
print $q->textfield(-name=>'range_low',-size=>5,-maxLength=>5, -default=>"");
print "/";
print $q->textfield(-name=>'range_hi',-size=>5,-maxLength=>5, -default=>"");
print "</td>";
print "</tr>";

print "<tr><td class=\"labelText\">";

# for checkboxes, don't send a hash to the method, because sending '0' to 
# -checked causes the box to be checked.   Format for this method is:
# checkbox(name, checked, value, label)
# we don't use value since it is boolean.
print $q->checkbox("fallout",$ashxpOption{fallout},"fallout","fallout");
print "</td>";

print "<td class=\"labelText\">";

# see above comment about not using the hash-style of method call for checkboxes
print $q->checkbox("grayscale",$ashxpOption{grayscale},"grayscale","grayscale");
print "</td></tr>";

print '<tr><td class="labelText">';
# see above comment about not using the hash-style of method call for checkboxes
print $q->checkbox("airborne",$ashxpOption{airborne},"airborne","airborne");

print "</td><td class=\"labelText\">";
print $q->checkbox("no-stamp",$ashxpOption{"no-stamp"},"no-stamp","no stamp");
print "</td></tr>";

print "<tr><td>";
print "<b><a href=\"#\" onclick=\"openHelp('$Webpuff::home_web/help/gridlines.html')\" class=\"helpLink\" title=\"click for option explanation\"> gridlines</a></b>";
print "</td><td>";
print $q->textfield(-name=>"gridlines",
                    -size=>8,
		    -class=>"tableEntry",
		    -default=>$ashxpOption{gridlines}
		    );
print "</td></tr>";

print "<tr><td>";
print "<b><a href=\"#\" onclick=\"openHelp('$Webpuff::home_web/help/latlon.html')\" class=\"helpLink\" title=\"click for option explanation\"> bounds</a></b>";
print "</td><td>";
print $q->textfield(-name=>"latlon",
                    -size=>16,
		    -class=>"tableEntry",
		    );
print "</td></tr>";
		    
print "<tr>";
print "<td class=\"labelText\">";
print "color:";
print "</td><td>";
print $q->scrolling_list(-name=>'colorProtocol',
			-value=>$scrollOptions{color},
			 -size=>1,
			 -class=>"tableEntry",
			-default=>$scrollOptions{color}[0],
			 );
print "</td></tr>";

print "<tr>";
print "<td class=\"labelText\">";
print "projection:";
print "</td><td>";
print $q->scrolling_list(-name=>'projection',
				-value=>$scrollOptions{projection},
			 -size=>1,
			 -class=>"tableEntry",
			 -default=>$ashxpOption{projection},
			 );
			 
print "</td>";
print "</tr>";

print "<tr>";
print "<td class=\"labelText\">";
print "particle size:";
print "</td><td>";
print $q->textfield(-name=>'pixels',
                    -size=>2,
		    -maxLength=>1, 
		    -class=>"tableEntry",
		    -default=>$ashxpOption{pixels});
print "</td>";
print "</tr>";


print "<tr>";
print "<td class=\"labelText\">";
print "magnify:";
print "</td><td>";
print $q->textfield(-name=>'magnify',
                    -size=>2,
		    -maxLength=>2, 
		    -class=>"tableEntry",
		    -default=>$ashxpOption{magnify});
print "</td>";
print "</tr>";


print "<tr>";
print "<td>";
print $q->submit(-name=>'runashxp', -value=>$update_label);
print "</td><td>";
#print $q->defaults('RESET');
print $q->submit(-name=>'reset', -value=>"RESET");
print "</td></tr>";

print "<tr>";
#print "<td colspan='2'>";
#print $q->radio_group(-name=>'imageFormat',
#                      -values=>['JPEG','GIF','conc'],
#		      -default=>$ashxpOption{format},
#		      );

print "<td class='labelText'>";
print "<b><a href=\"#\" onclick=\"openHelp('$Webpuff::home_web/help/format.html')\" class=\"helpLink\" title=\"click for option explanation\"> format</a></b>";
print "</td><td>";
print $q->scrolling_list(-name=>'imageFormat',
			-value=>$scrollOptions{format},
			-labels=>$formatLabels,
			-size=>1,
			-class=>"tableEntry",
			-default=>$ashxpOption{format},
			);
print "</td></tr>";
		 
print "</table>";

print $q->end_form;

print "<a href=\"#\" onclick=\"openHelp('$Webpuff::home_web/$runDir/$sessionID/$previous/ashxp.log')\" class=\"helpLink\" title=\"click for log\">log</a><br/>";
print $q->end_html;
}

#------------------------
sub lock_free {
  return 0 if (-e "$Webpuff::home_abs/$runDir/$sessionID/ashxp.lock");
  return 1;
}
#------------------------
sub show_updating {
  print $q->header;
  print $q->start_html(-title=>"Display Options",
                       -bgcolor=>"lightseagreen",
		       -head=>$q->meta({-http_equiv=>'refresh',
#		                        -content=>"5;URL=\"ashxpOptions.pl?refresh=1&runDir=$runDir&sessionID=$sessionID&previous=$previous\""})
		                        -content=>"5;URL=\"ashxpOptions.pl?refresh=1&sessionID=$sessionID&previous=$previous\""})
		  ); 
 print "<img src=\"$Webpuff::home_web/pix/updating.gif\">";
 print $q->end_html();
 return;
}

#------------------------
sub init_images {
  my @ncFiles = @_;

  my $command = $Webpuff::ashxp_exe;
  foreach (@ncFiles)
  {
    $command .= "$_ ";
  }

  # run the command only if there is not logfile, otherwise a new process
  # will get kicked in every time this script is reloaded by the http_equiv
  # statement in the header above
  # the parent will create a reloading page with "updating" information.
  # the child will execute the command, but only the first time
  if (-e "ashxp.log") {
    # there is a log file, but not the correct number of jpg files.  An error
    # probably occurred, so report that with a non-reloading page
    print $q->header;
    print $q->start_html(-title=>"Display Options - Errors",
                         -bgcolor=>"lightseagreen"
			 );

    print "Errors occurred while processing this request.  They were:<UL>";		 
    open LOG, "<ashxp.log";
    while (<LOG>) {
      if ($_ =~ /ERROR/) {
        print "<LI>$_</LI>";
        }
      }
    print "</UL>";
    print $q->end_html();
  } else {
    if (my $pid = fork) {
      # parent here
      print $q->header;
      print $q->start_html(-title=>"Display Options",
                           -bgcolor=>"lightseagreen",
		           -head=>$q->meta({-http_equiv=>'refresh',
		                            -content=>"5;ashxpOptions.pl?sessionID=$sessionID&previous=$previous"})
		  	   ); 
      print "<img src=\"$Webpuff::home_web/pix/updating.gif\">";
      print $q->end_html();
      } elsif (defined $pid) {
      #child here
      run_command($command) ;
      }
    }
    
  return;
}

#------------------------
sub run_command {
  my $command = shift;
  
  # go to the directory since ashxp dumps to the current directory
  chdir "$Webpuff::home_abs/$runDir/$sessionID/$previous" or die "Failed to chdir to $Webpuff::home_abs/$runDir/$sessionID:$!\n";
  
  # open a log, clobber an existing one so that we can parse it easily when
  # reloading the last used values
  open LOG, ">ashxp.log";
  # put an lock on the file while we are running.  The Fcntl module did not 
  # work, LOCK_EX did not appear available on Solaris.
  open LOCK, ">>ashxp.lock";
  # dump command so we can reparse it for updating the last set of images
  print LOG "$command\n";
    
  open COMM, "$command 2>&1|";
  while (<COMM>) {
    print LOG $_;
  }
  close COMM;
  close LOG;
  close LOCK;
  system("rm -f ashxp.lock");
  # return back 
  chdir "../../.." or die "Failed to go back home: $!\n";
  return;
}
#------------------------
sub process_form {
  my $q = shift;

  # get a listing of any available netCDF files
  my @ncFiles=();
	
	my $suf = ".cdf";
	if ($q->param('imageFormat') =~ m/BLOCK|CONTOUR/) {$suf=".nc";}

  foreach (glob("$Webpuff::home_abs/$runDir/$sessionID/$previous/*${suf}")) { push @ncFiles, $_; }

  my $filelist="";
  foreach(@ncFiles) { $filelist.=" " . basename($_); }

	# conc. clouds use the .nc file
#	if ($q->param('imageFormat') =~ m/BLOCK|CONTOUR/) {
#		$filelist = basename(glob("$Webpuff::home_abs/$runDir/$sessionID/$previous/*.nc"));
#	}

  my $command;
  if ($q->param('imageFormat') eq "JPEG") {
    $command = $Webpuff::ashxp_exe;
  } elsif ($q->param('imageFormat') eq "GIF") {
    $command = "$Webpuff::ashgmt_exe";
  } elsif ($q->param('imageFormat') eq "BLOCK") {
		$command = "$Webpuff::ashgmt_exe";
	} elsif ($q->param('imageFormat') eq "CONTOUR") {
		$command = "$Webpuff::ashgmt_exe --contours ";
	}
  
  # hash of possible options
  my %opts;
  
  # set a large background file unless one is already defined
  $command .="--bgfile=$Webpuff::home_exe/etc/images/GLOBALeb3colshade.jpg " unless (defined $ENV{ASHXP_BG_FILE_NAME});

	# set the font file
	$command .= "--fontfile=$Webpuff::home_exe/etc/fonts/FreeMonoBold.ttf ";

  $opts{color} = $q->param('colorProtocol');
  $command .= "--color=$opts{color} ";
  
   $opts{pixels} = $q->param('pixels');
   $command .= "--pixels=$opts{pixels} ";  
  
  $opts{projection} = $q->param('projection');
  $command .= "--projection=$opts{projection} ";
  
  $opts{fallout} = $q->param('fallout');
  $command .= "--fallout " if ($opts{fallout});

  $opts{airborne} = $q->param('airborne');
  $command .= "--airborne " if ($opts{airborne});
  
  $opts{grayscale} = $q->param('grayscale');
  $command .= "--grayscale " if ($opts{grayscale});
  
  $opts{latlon} = $q->param('latlon');
  $command .= "--latlon $opts{latlon} " if (length $opts{latlon} > 0);
  
   $opts{magnify} = $q->param('magnify');
   $command .= "--magnify=$opts{magnify} ";
  
  $opts{hgtrange} = get_hgt_range($q);
  $command .= "--hgt-range=$opts{hgtrange} " if (length $opts{hgtrange} > 0);
  
  $opts{gridlines} = $q->param('gridlines');
  $command .= "--gridlines=$opts{gridlines} " if (length $opts{gridlines}>0);
  
  $command .= "--no-stamp " if $q->param('no-stamp');
  $command .= $filelist;
  
  run_command($command);
    
  return;
}
#------------------------
sub get_hgt_range {
  my $q = shift;
  my $low = $q->param('range_low');
  my $high = $q->param('range_hi');
  
  return "" if ($low >= $high);

  return "$low/$high";
 
}  
#------------------------
sub jpg_listing {
  my @jpgFiles = ();
  my $ext;
  if ($ashxpOption{format} eq "GIF") {
    $ext = "ash*.png";
  } elsif ($ashxpOption{format} eq "JPEG") {
    $ext = "ash*.jpg";
  } elsif ($ashxpOption{format} =~ m/BLOCK|CONTOUR/) {
		$ext = "conc*.png";
	}
  
  if (not $ext) {
    return @jpgFiles;
  }
  
  foreach (glob("$Webpuff::home_abs/$runDir/$sessionID/$previous/*$ext"))
  {
    $_ = basename($_);
    push @jpgFiles, $_;
  }
  return @jpgFiles;
}
#---------------------
sub set_defaults {
  my $q=shift;
  foreach (keys %ashxpOptionDefault) {
    $ashxpOption{$_} = $ashxpOptionDefault{$_};
    }
  return;
}
#---------------------
sub parse_previous {
  my $dir = shift;
  
  return if (!$dir);
  
  my $command = "";
  
  open LOG, "<$Webpuff::home_abs/$runDir/$sessionID/$dir/ashxp.log" or return;
#   while (<LOG>) {
#     $command = $_ if ($_=~/$Webpuff::ashxp_exe/);
#   }
  # first line in the log is the command, the rest is output like warnings
  $command = <LOG>;
  close LOG;
  
  my @tokens = split "--", $command;
  # get the executable name
  my $executable = shift @tokens;
  # set the format type
  if ($Webpuff::ashxp_exe =~  $executable) {
    $ashxpOption{format} = "JPEG";
  } elsif ($Webpuff::ashgmt_exe =~ $executable) {
    $ashxpOption{format} = "GIF";
  } 

	# ashgmt may have processed conc. files, so check for that
	$ashxpOption{format} = "BLOCK" if ($command =~ m/conc\d*\.nc$/);
	# adjust once again if a contour plot was made
	$ashxpOption{format} = "CONTOUR" if ($command =~ m/--contours/);
  
  # the last token has a list of files after the last switch, remove them
  # do not do this, however, if no log exists and, thus, no tokens.
  $tokens[scalar @tokens-1] = (split " ", @tokens[scalar @tokens-1])[0] if (scalar @tokens > 0);
  
  foreach (@tokens) {
    # clean whitespace
    $_ =~ s/^\s+//g;
    $_ =~ s/\s+$//g;
    my ($option, $value) = split "=";
    # scan through the possible text options
    foreach (keys %ashxpOption) {
      if ("$_" eq $option) {
        # some options have not value (i.e. --fallout) so make it '1'
        $value = 1 unless (defined($value));
	# set the default value to this one
        $ashxpOption{$_} = $value;
      }
    }
  }
  return;
}
#---------------------
sub reset_form {
  my $sID = $q->param('sessionID');
  my $previous = $q->param('previous');
  $q->delete_all();
  $q->param(-name=>'sessionID',-value=>$sID);
  $q->param(-name=>'previous',-value=>$previous);
  return;
}  
