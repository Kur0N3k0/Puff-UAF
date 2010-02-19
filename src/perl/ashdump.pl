#!/usr/bin/perl -w

use NetCDF;
use Getopt::Long;
use Pod::Usage;
use strict;

# if the variable list changes, add it to this array and add a format
# for printing it to the hash below
my @ashvariable = qw(lon lat hgt size age grounded exists);
my @concvariable = qw(rel_air_conc abs_air_conc);

my %format;
$format{lon}="%7.3f  ";
$format{lat}="%6.3f  ";
$format{hgt}="%5.0f  ";
$format{age}="%7.0f  ";
$format{size}="%7.0f  ";
$format{grounded}="%1.0f  ";
$format{exists}="%1.0f  ";
$format{level}="%5.0f   ";
$format{rel_air_conc}="%3.0f  ";
$format{abs_air_conc}="%4.0f   ";

# these are all the long options with default values. 
my %opt; 
my %range;
GetOptions(
	   "header!" => \$opt{header},
	   "header-only" => \$opt{header_only},
	   "help|?"  => \$opt{help},
	   "height-in-feet" => \$opt{height_in_feet},
     "range=s" => \&range,
	   "reverse" => \$opt{reverse},
	   "quiet"   => \$opt{quiet},
	   "shift-west"     => \$opt{shift_west},
	   "show-params"    => \$opt{show_params},
	   "showparams"     => \$opt{show_params},
	   "size-in-meters" => \$opt{size_in_meters},
	   "sort=s"  => \$opt{sort},
	   "var=s"   => \$opt{printvarstr},
	   "verbose" => \$opt{verbose},
	   "version" => sub { print "2.2.1\n"; exit(0);}
	   );
my $pathname = $ARGV[0];

# if given --help, print pod and exit	 
if ( $opt{help} or scalar @ARGV == 0 ) { pod2usage(1); }

# first thing is to determine whether this is a particle or 
# concentration file
my $ncid = NetCDF::open($pathname, NOWRITE);
die "$!\n" unless ($ncid);
my $filetype = ash_filetype($ncid);
@ashvariable = @concvariable if ($filetype eq "conc");

# set default values when options are not given
defaults(\%opt);

# default to print everything in @ashvariable 
my @printvar=@ashvariable;
# if variable string given, parse it  
if ($opt{printvarstr}) {@printvar = split(/,/,$opt{printvarstr}); }

my ($status, $name, $nd, $nv, $na, $dimid);
#$ncid = NetCDF::open($pathname, NOWRITE);
$status = NetCDF::inquire($ncid, $nd, $nv, $na, $dimid);
if (scalar @ashvariable != $nv and $opt{header} ) {
  my $number = scalar @ashvariable;
  # warn if the ashfile has more variables than this script knows about.
  print "$pathname contains $nv variables, but only $number will be retrieved\n" unless ($opt{quiet});
  if ($opt{verbose}) { 
    print "variables retrieved will be: ";
    foreach (@ashvariable) { print "$_ "; }
    print "\n";
    }
  }

 if ($opt{verbose}) {
   print "ncid: $ncid numdim: $nd numvar:";
   print "$nv num_att: $na dim_id: $dimid\n"; }
 
# retrieve and print the global attributes
if ($opt{header}) {
  my @attributes=qw(title volcano date_time plume_shape);
	@attributes = () if ($filetype eq "conc");
  # create an empty string so its location can be sent to NetCDF::attget()
  my $attval="";
  foreach(@attributes) {
    NetCDF::attget($ncid, NetCDF::GLOBAL, $_, \$attval) == 0 ||
      die "Couldn't get attribute \"$_\"\n"; 
    print "$_:\t$attval\n";
    }
 }

# get and display simulation parameters
if ($opt{show_params} or $opt{verbose}) { show_params(); }

# exit now if only the header and parameters above were needed, don't go through
# the process of getting all the values
exit if ($opt{header_only});

#get all values as a hash of arrays
%main::values = get_values() if ($filetype eq "part");
%main::values = get_conc_values() if ($filetype eq "conc");

if (exists $main::values{age} ) { 
  determine_age($main::values{age}); }

if (! $opt{size_in_meters} and exists $main::values{size} ) { 
  convert_size($main::values{size}); } 

if ( $opt{shift_west} and exists $main::values{lon} ) { 
  shift_west($main::values{lon}); }

if ( $opt{height_in_feet} and exists $main::values{hgt}) { 
  height_in_feet($main::values{hgt}); }
  
if ($opt{sort}) { $main::values{index} = sort_values($main::values{index}); }

# make and print the header
if ($opt{header}) { printheader(@printvar); }


# write the values to the screen, doing some filtering if neccesary
VALUES: for (0..$main::length-1) { 
  my $i=$main::values{index}[$_];
  # make sure value is within range (if range was requested)
  foreach my $var (keys %range) {
    if ( $main::values{$var}[$i] < $range{$var}[0] or
         $main::values{$var}[$i] > $range{$var}[1] ) {next VALUES; }
    }
  foreach my $var (@printvar) { printf $format{$var},$main::values{$var}[$i]; }
  print "\n";
  }
  
#done
#---------------------------------------------------  
# called directly from Getopt::Long, @_ is [option, value], so we want value
# since only --range calls this function.  'value' is a comma-delimited list
# of filters of 'var=lower/upper'.  So split up the list, make sure everything
# is numeric, and add the filter to the hash %range.  Filters are additive, so
# --range hgt=1000/3000,hgt=1500/4000 results in lower=1500 and upper=3000.
sub range {

  foreach (split ",",$_[1]) {
    my $bad_spec = 0;
    (my $var, my $rangeval) = split("=",$_);
    if (!defined $var or !defined $rangeval) { 
      warn "bad --range specification \"$_\", ignoring.\n";
      next; 
      }
    # pattern match with any delimiter
    $rangeval =~ m/(\d+\.?\d*).(\d+\.?\d*)/;
    my ($low, $up);
    if (defined($1) and defined($2)) {
      $low = $1;
      $up = $2;
    }
#    (my $low, my $up) = split("/",$rangeval);
    if (!defined($low) or !defined($up)) { 
      warn "bad --range specification \"$_\", ignoring.\n";
      next; 
      }
    foreach ($low, $up) {
      if ($_ =~ /\D/) {
        print "WARNING: \"$_\" is not numeric in range specification for $var, ignoring this option.\n";
	$bad_spec = 1;
        }
      }
    # allow multiple filter that are addative
    if (defined($range{$var})) {
      $low = $range{$var}[0] if ($range{$var}[0] > $low);
      $up = $range{$var}[1] if ($range{$var}[1] < $up);
    }
    
    $range{$var}=[ $low, $up ] unless ($bad_spec); 
    }
  }  
#---------------------------------------------------  

sub sort_values {
  my $index_ref=$_[0];
  my @index = sort bykey @$index_ref;
  if ($opt{reverse}) { @index = reverse @index; }
  $main::values{index}=\@index;
  }
  
sub bykey {
  $main::values{$opt{sort}}[$a] <=> $main::values{$opt{sort}}[$b]
  } 
#---------------------------------------------------  
sub determine_age {
  my $age_ref = $_[0];
  my $clock_time;
  my $clock_id = NetCDF::varid($ncid, "clock_time");
  $status = NetCDF::varget1($ncid, $clock_id, 0, $clock_time);
  if ($status) { warn "could not get \"clock_time\" for determining age.\n"; }
  else {
    foreach (@$age_ref) { $_ = $clock_time - $_; }
    }
  }
  
#---------------------------------------------------  
# get id's of dimensions and values (because we're using netcdf2). Then get
# values as a hash of arrays 'values'. The units are taken from the attributes
# and also stored as the hash 'units'.  
sub get_values {
  my %values;
  my %varid;
  %main::units=();
  my $ashid = NetCDF::dimid($ncid, "nash");
  NetCDF::diminq($ncid, $ashid, $name, $main::length) == 0 ||
    die "Couldn't get information on ash dimension\n";
  foreach my $var (@ashvariable) {
    $varid{$var} = NetCDF::varid($ncid, $var);
    my @var_units=();
    my @var_val=();
    NetCDF::varget($ncid, $varid{$var}, 0, $main::length, \@var_val) == 0 or
      die "failed to get \"$var\" values: $!\n";
    $values{$var} = [ @var_val ];
    NetCDF::attget($ncid, $varid{$var}, "units", \@var_units) == 0 or
      die "failed to get units for $_: $!\n";
    foreach(@var_units) { $main::units{$var} .= chr $_; }
    $values{index} = [0..$main::length-1];
    }
  return %values;
  }
    
#---------------------------------------------------  
sub get_conc_values {
	my %values;
	my %varid;
	my @dim=qw(lon lat level);
	my @size = ();
	# get all the dimension sizes
	my @record;
	my ($nrecvars, @recvarids, @recsizes);
	NetCDF::recinq($ncid, $nrecvars, \@recvarids, \@recsizes);
	NetCDF::recget($ncid, 0, \@record);
	foreach my $var (@concvariable) {
	  my @var_units = ();
		$varid{$var} = NetCDF::varid($ncid, $var);
	  $values{$var} = $record[$varid{$var}];
		NetCDF::attget($ncid, $varid{$var}, "units", \@var_units);
    foreach(@var_units) { $main::units{$var} .= chr $_; }
		}
		$values{index} = [0 .. scalar @{$values{$concvariable[0]}}-1];

# get the dimensions as variables, they are numbered 1..3
	for (1..3) {
	  my $dim = $dim[$_-1];
		my $length;
		NetCDF::diminq($ncid, $_, $dim, $length);
		$varid{$dim} = NetCDF::varid($ncid, $dim);
		my @var_units=(); my @dval = ();
		NetCDF::varget($ncid, $varid{$dim}, 0, $length, \@dval);
		$values{$dim} = [@dval ];
		NetCDF::attget($ncid, $varid{$dim}, "units", \@var_units);
    foreach(@var_units) { $main::units{$dim} .= chr $_; }
		}

	# expand these array values
	my (@lln, @llt, @llv);
	foreach my $lv (@{$values{level}}) {
	  foreach my $lt (@{$values{lat}}) {
			foreach my $ln (@{$values{lon}}) {
				push @lln, $ln;
				push @llt, $lt;
				push @llv, $lv;
				}}}
	$values{lon}=\@lln;
	$values{lat}=\@llt;
	$values{level}=\@llv;

	# set the total array length
	$main::length = scalar @{$values{lon}};
	
	return %values;
}
#---------------------------------------------------  
sub show_params {
  my @params = qw(clock_time origin_time origin_lon origin_lat 
  erupt_hours plume_height plume_width_z plume_width_h diffuse_h 
  diffuse_v log_mean log_sdev );
  foreach (@params) {
    my $var_val;
    my $var_id = NetCDF::varid($ncid, $_);
    $status = NetCDF::varget1($ncid, $var_id, 0, $var_val);
    if ($status) { warn "failed to get \"$_\".\n"; }
    else { print "$_:\t$var_val\n"; }
    }
    
  # print the lat/lon bounding box
  foreach ( qw(lon lat) ) {
    my $attval = "";
    my $var_id = NetCDF::varid($ncid, $_);
    NetCDF::attget($ncid, $var_id, "min_value", \$attval) == 0 ||
      die "Couldn't get min attribute for \"$_\"\n";
    printf "$_ min/max:\t%5.2f",$attval; 
    NetCDF::attget($ncid, $var_id, "max_value", \$attval) == 0 ||
      die "Couldn't get max attribute for \"$_\"\n";
    printf " /%5.2f\n",$attval;
  }
    
  print "\n";
  }

#---------------------------------------------------  
sub convert_size {
  my $size_ref=$_[0];
  if ( $main::units{size} =~ m/^meter/ ) {
    foreach (@$size_ref) { $_ = $_*1000000; }
    $main::units{size} = "microns";
    }
  return;
  }
#---------------------------------------------------  
sub shift_west {
  my $lon_ref=$_[0];
  if ($main::units{lon} =~/deg/) {
    foreach(@$lon_ref) { if ( $_ < 0 ) { $_ += 180.00; } }
    }
  else { warn "\"lon\" variable does not appear to be in units of degrees.";
    }
  }
#---------------------------------------------------  
sub height_in_feet {
  my $hgt_ref = $_[0];
  if ($main::units{hgt} =~ m/^meter/ ) {
    foreach (@$hgt_ref) { $_ *= 3.2808; }
    $main::units{hgt} = "feet";
    }
  }
#---------------------------------------------------  
# 
sub printheader {
  my @variable = @_;
  print "  ";
  foreach (@variable) { print "$_\t"; }
  print "\n";
  foreach (@variable) {
    print "[ $main::units{$_} ]"; 
    }
  print "\n";
  my $width = 10*(scalar @variable);
  print "-" x $width;
  print "\n";
  }
#---------------------------------------------------  
# determine the type of ash file, either particle or concentration
sub ash_filetype {
  my $ncid = shift;
	my ($nd, $nv, $na, $dimid);
  NetCDF::inquire($ncid, $nd, $nv, $na, $dimid);
  if ($nd == 4) {return "conc";}
	else {return "part"};
	}
#---------------------------------------------------  
# set default values when options are not given
sub defaults {
  my $opt = pop;
  
  $opt->{header} = 1 unless (defined ($opt->{header}) );
  $opt->{header_only} = 0 unless (defined ($opt->{header_only}) );
  $opt->{help} = 0 unless (defined ($opt->{help}) );
  $opt->{height_in_feet} = 0 unless (defined ($opt->{height_in_feet}) );
  $opt->{quiet} = 0 unless (defined ($opt->{quiet}) );
  $opt->{reverse} = 0 unless (defined ($opt->{reverse}) );
  $opt->{shift_west} = 0 unless (defined ($opt->{shift_west}) );
  $opt->{show_params} = 0 unless (defined ($opt->{show_params}) );
  $opt->{size_in_meters} = 0 unless (defined ($opt->{size_in_meters}) );
  $opt->{sort} = "" unless (defined ($opt->{sort}) );
  $opt->{verbose} = 0 unless (defined ($opt->{verbose}) );
  $opt->{printvarstr} = "" unless (defined ($opt->{printvarstr}) );

  return;
}
=head1 NAME

ashdump - sorts, filters, and prints data from a Puff-generated ash file

=head1 SYNOPSIS

B<ashdump>
S<[B<--(no)header> ]>
S<[B<--height-in-feet> ]>
S<[B<--range> I<var>=I<lower>/I<upper> ]>
S<[B<--reverse> ]>
S<[B<--shift-west> ]>
S<[B<--show-params> ]>
S<[B<--size-in-meters> ]>
S<[B<--sort> I<var> ]>
S<[B<--var> I<var> ]>
S<[B<--verbose> ]>
filename

=head1 DESCRIPTION

I<ashdump> is a Perl program that prints the contents of a netCDF
Puff-generated ash file to STDOUT.  Filtering and sorting are options.

=head1 OPTIONS

=over 4

=item B<--(no)header>

print header information, which is mosting global attributes of the ash file.
Prepending 'no' surpresses the header, which is good for dumping to data files that
are to be read by another application.

=item B<--header-only>

print only header information.

=item B<--height-in-feet>

Convert the height values to feet from meters.  If the original units are not
meters, this does nothing.

=item B<--range> I<var>=I<lower>/I<upper> 

Filter the data and output only data for which I<var> is between the values 
I<lower> and I<upper>.  A zero or negative range will result in no output. 
There are no lower or upper limits on the values.  I<--range> can be specified
several times or a comma-delimited list can be used, and the filter operate in
series.

=item B<--reverse>

Reverse the order when sorting (i.e. largest -> smallest).  This option only
makes sense when used with B<--sort>.

=item B<--shift-west>

Shift the longitude values from -180/180 to 0/360.  This option only has an
effect if there are originally negative longitude values.

=item B<--show-params>

List the parameters that were specified for the simulation that created this
ash file.

=item B<--size-in-meters>

List the particle size in meters instead of the default size of microns.

=item B<--sort> I<var>

Sort the values using I<var> from smallest to largest.  Use B<--reverse> to 
reverse the order.  If multiple instances are given, the last one has precident.

=item B<--var> I<var>

Only output the variable I<var>.  This option can be specified several times
or as a comma-delimited list.  The B<--range> and B<--sort> setting can 
still be used for other variables not specified here and still 
perform their functions.

=item B<--verbose>

Verbose output that also includes B<--show-params>
  
=head1 LIMITATIONS

Limited error checking for options.

=head1 SEE ALSO

The netCDF libraries and perl extenstion are required, see www.unidata.ucar.edu.
This utility was written as part of the PUFF volcanic ash tracking project, see
the puff homepage at puff.images.alaska.edu.

=head1 COPYRIGHT

(c) 2002-2004 by Rorik Peterson, Geophysical Institute, University of Alaska,
Fairbanks.

=head1 AUTHOR

Rorik Peterson, rorik@gi.alaska.edu

=cut
