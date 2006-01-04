/****************************************************************************
    puff - a volcanic ash tracking model
    Copyright (C) 2001-2003 Rorik Peterson <rorik@gi.alaska.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "my_getopt.h"
#endif
#include <iostream> /* cout */
#include <string> /* strcpy, strdup */
#include <cstdio> /* sscanf */
#include "ashdump_options.h"

void parse_options(int argc, char **argv) 
{
  extern struct Argument argument;
  extern char *optarg;
  extern int optind;
  char *opt_sng;
  int opt;
  static struct option opt_lng[] = {
    {"age",optional_argument,0,ASHDUMP_AGE},
    {"active",optional_argument,0,ACTIVE},  
    {"infile",required_argument,0,INFILE},
    {"feet",optional_argument,0,FEET},
    {"hdr",optional_argument,0,HDR},
    {"header",optional_argument,0,HDR},
    {"height",optional_argument,0,HEIGHT},
		{"height-in-feet",optional_argument,0,FEET},
    {"help",no_argument,0,HELP},
    {"lat",optional_argument,0,ASHDUMP_LAT},
    {"lon",optional_argument,0,ASHDUMP_LON},
    {"precision",required_argument,0,PRECISION},
    {"range",required_argument,0,RANGE},
    {"size",optional_argument,0,SIZE},
    {"stats",optional_argument,0,STATS},
    {"shiftWest",optional_argument,0,SHIFTWEST},
    {"showParams",optional_argument,0,SHOWPARAMS},
    {"showparams",optional_argument,0,SHOWPARAMS},
    {"show-params",optional_argument,0,SHOWPARAMS},
    {"sz",no_argument,0,ASHDUMP_SZ},
    {"usage",no_argument,0,USAGE},
		{"variables",required_argument,0,VARIABLE_LIST},
		{"version",no_argument,0,VERSION_ASHDUMP},
    {"width",required_argument,0,WIDTH},
    {"z",optional_argument,0,ASHDUMP_Z},
    {0,0,0,0}
    }; /* end opt_lng */
    int opt_idx=0; /* Index of current long option into opt_lng array */
  opt_sng = "+";
  /* set default values first */
  set_defaults(&argument);
  while((opt = getopt_long_only(argc,argv,opt_sng,opt_lng,&opt_idx)) != EOF){
  switch (opt)
    {
    case ACTIVE: 
      if (optarg) {
        if (toupper(optarg[0]) == 70) argument.active = false;
 	else if (toupper(optarg[0]) == 84) argument.active = true; 
 	else 
 	  std::cout << "unrecognized boolean option -active=" << optarg << std::endl;
         }  // if (optarg)
      else { argument.active = true; }
      break;
     case ASHDUMP_AGE: 
      if (optarg && strlen(optarg) > 0 ) 
			{
       if (toupper(optarg[0]) == 70) argument.age = false;
 	else if (toupper(optarg[0]) == 84) argument.age = true; 
 	else 
 	  std::cout << "unrecognized boolean option -age=" << optarg << std::endl;
         }  // if (optarg)
      else { argument.age = true; }
      break;
    case FEET: 
      if (optarg) {
        if (toupper(optarg[0]) == 70) argument.feet = false;
 	else if (toupper(optarg[0]) == 84) argument.feet = true; 
 	else 
 	  std::cout << "unrecognized boolean option -feet=" << optarg << std::endl;
         }  // if (optarg)
      else { argument.feet = true; }
      break;
  case HDR: 
      if (optarg) {
        if (toupper(optarg[0]) == 70) argument.hdr = false;
 	else if (toupper(optarg[0]) == 84) argument.hdr = true; 
 	else 
 	  std::cout << "unrecognized boolean option -hdr=" << optarg << std::endl;
         }  // if (optarg)
      else { argument.hdr = true; }
      break;
    case HEIGHT: 
      if (optarg) {
        if (toupper(optarg[0]) == 70) argument.showHeight = false;
 	else if (toupper(optarg[0]) == 84) argument.showHeight = true; 
 	else 
		argument.height = strdup(optarg);
         }  // if (optarg)
      else { argument.showHeight = true; }
      break;
    case HELP: 
      show_help();
      exit(0);
      break;
    case ASHDUMP_LAT: 
      if (optarg) {
        if (toupper(optarg[0]) == 70) argument.lat = false;
 	else if (toupper(optarg[0]) == 84) argument.lat = true; 
 	else 
 	  std::cout << "unrecognized boolean option -lat=" << optarg << std::endl;
         }  // if (optarg)
      else { argument.lat = true; }
      break;
    case ASHDUMP_LON: 
      if (optarg) {
        if (toupper(optarg[0]) == 70) argument.lon = false;
 	else if (toupper(optarg[0]) == 84) argument.lon = true; 
 	else 
 	  std::cout << "unrecognized boolean option -lon=" << optarg << std::endl;
         }  // if (optarg)
      else { argument.lon = true; }
      break;
    case INFILE: 
			argument.infile = strdup(optarg);
      break;
     case PRECISION: 
      if (sscanf(optarg, "%i", &argument.precision) == 0)
        std::cerr << "invalid value for option precision: " << optarg << std::endl;
      break;
    case RANGE: 
			argument.range = strdup(optarg);
      break;
    case SIZE:
      if (optarg) {
        if (toupper(optarg[0]) == 70) argument.showSize = false;
 	else if (toupper(optarg[0]) == 84) argument.showSize = true; 
 	else 
		argument.size = strdup(optarg);
         }  // if (optarg)
      else { argument.showSize = true; }
      break;
   case SHIFTWEST:
      if (optarg) {
        if (toupper(optarg[0]) == 70) argument.shiftWest = false;
 	else if (toupper(optarg[0]) == 84) argument.shiftWest = true; 
 	else 
 	  std::cout << "unrecognized boolean option -shiftWest=" << optarg << std::endl;
         }  // if (optarg)
      else { argument.shiftWest = true; }
      break;
		case VARIABLE_LIST:
			if ( strstr(optarg, "age") ) argument.age = true;
			if ( strstr(optarg, "height") ) argument.showHeight = true;
			if ( strstr(optarg, "hgt") ) argument.showHeight = true;
			if ( strstr(optarg, "lat") ) argument.lat = true;
			if ( strstr(optarg, "lon") ) argument.lon = true;
			if ( strstr(optarg, "size") ) argument.showSize = true;
			if ( strstr(optarg, "sz") ) argument.showSize = true;

			break;
    case SHOWPARAMS:
      if (optarg) {
        if (toupper(optarg[0]) == 70) argument.showParams = false;
 	else if (toupper(optarg[0]) == 84) argument.showParams = true; 
 	else 
 	  std::cout << "unrecognized boolean option -noPatch=" << optarg << std::endl;
         }  // if (optarg)
      else { argument.showParams = true; }
      break;
    case STATS:
      if (optarg) {
        if (toupper(optarg[0]) == 70) argument.stats = false;
 	else if (toupper(optarg[0]) == 84) argument.stats = true; 
 	else 
 	  std::cout << "unrecognized boolean option -stats=" << optarg << std::endl;
         }  // if (optarg)
      else { argument.stats = true; }
      break;
     case ASHDUMP_SZ:
      // deprecated option -sz as a boolean option to display size info
      if (optarg) {
        if (toupper(optarg[0]) == 70) argument.showSize = false;
 	else if (toupper(optarg[0]) == 84) argument.showSize = true; 
 	else 
 	  std::cout << "unrecognized boolean option -sz=" << optarg << std::endl;
         }  // if (optarg)
      else { argument.showSize = true; }
      break;
    case USAGE:
      ashdump_usage();
      exit(0);
      break;
		case VERSION_ASHDUMP:
			static const char ver[] = VERSION;
			std::cout << ver << std::endl;
			exit(0);
			break;
    case WIDTH:
      if (sscanf(optarg, "%i", &argument.width) == 0)
        std::cerr << "invalid value for option precision: " << optarg << std::endl;
      break;
    case ASHDUMP_Z:
      // deprecated option -z as a boolean option to display height info
      if (optarg) {
        if (toupper(optarg[0]) == 70) argument.showHeight = false;
 	else if (toupper(optarg[0]) == 84) argument.showHeight = true; 
 	else 
 	  std::cout << "unrecognized boolean option -z=" << optarg << std::endl;
         }  // if (optarg)
      else { argument.showHeight = true; }
      break;
    } /* end switch */
    } /* end while loop */

  /* filename should remain */
  if (optind >= argc) {
    std::cerr << "ERROR: no file specified\n";
    ashdump_usage();
    exit(0);
    }
  else {
		argument.infile = strdup(argv[optind]);
    optind++;
    }
  if (optind < argc) 
	{
    std::cerr << "ERROR: more than one file cannot be processed at a time.\n";
    exit(0);
	}
     
  return;
}  /* parse_options */
  
//////////////////////
// set default values for everything
//////////////////////
void set_defaults(struct Argument *argument) 
{

  argument->age = false;
  argument->active = false;
	argument->infile = (char)NULL;
  argument->feet = false;
  argument->hdr = false;
  argument->height = (char)NULL;
  argument->lat = false;
  argument->lon = false;
  argument->precision = 2;
  argument->range = (char)NULL;
  argument->size = (char)NULL;
  argument->stats = false;
  argument->shiftWest = true;
  argument->showParams = false;
  argument->showSize = false;
  argument->width = 14;
  argument->showHeight = false; 
  
  return;
}

//////////////////////
// show help message
//////////////////////
void show_help() 
{
  std::cout << "Valid options are: (see documentation for futher details)\n";
  std::cout << "\t-age\n";
  std::cout << "\t-active\n";
  std::cout << "\t-infile     filename   (string)\n";
  std::cout << "\t-feet\n";
  std::cout << "\t-hdr\n";
  std::cout << "\t-height     [Z1/Z2]    (optional string)\n";
  std::cout << "\t-lat\n";
  std::cout << "\t-lon\n";
  std::cout << "\t-precision   value       (integer)\n";
  std::cout << "\t-range       Y1/Y2/X1/X2 (string)\n";
  std::cout << "\t-size       [S1/S2]     (optional string)\n";
  std::cout << "\t-stats\n";
  std::cout << "\t-shiftWest\n";
  std::cout << "\t-showParams\n";
  std::cout << "\t-sz                     (deprecated)\n";
  std::cout << "\t-usage\n";
  std::cout << "\t-width      value       (integer)\n";
  std::cout << "\t-z                      (deprecated)\n";
  
  return;
  }

//////////////////////
// print simple usage message
//////////////////////
void ashdump_usage() 
{
  std::cout << "Usage:\n\tashdump [options] filename\n";
  std::cout << "Use \"-help\" to see a listing of options\n";
  exit(0);
  return;
}
