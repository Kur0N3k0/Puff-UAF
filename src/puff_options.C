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
#include <iostream> /* std::cout */
#include <string> /* strncpy */
#include <fstream> /* ifstream */
#include <string> /* string class */
#include <cstdio> /* sscanf */
#include <ctime> /* time() */
#include "puff_options.h"

extern void show_volcs();
extern char* time2unistr(time_t);  // from utils.C

void set_defaults(struct Argument *argument);
void parse_file(const char* inFile, const struct option *op);
void option_switch(int opt,  const char *optarg, const struct option *opt_lng);
//////////////////////////////////
// defined the option structure and calls getopt_long_only
//////////////////////////////////
void parse_options(int argc, char **argv) 
{
  extern struct Argument argument;
  extern char *optarg;
  int opt;
  static struct option opt_lng[] = {
    {"argFile",required_argument,0,ARGFILE},
    {"ashLogMean",required_argument,0,ASHLOGMEAN},  
    {"ashLogSdev",required_argument,0,ASHLOGSDEV},
    {"averageOutput",optional_argument,0,AVERAGEOUTPUT},
    {"dem",required_argument,0,DEM},
    {"diffuseH",required_argument,0,DIFFUSEH},
    {"diffuseZ",required_argument,0,DIFFUSEZ},
		{"drag",required_argument,0,DRAG},
    {"dtMins",required_argument,0,DTMINS},
    {"eruptDate",required_argument,0,ERUPTDATE},
    {"eruptHours",required_argument,0,ERUPTHOURS},
    {"eruptMass",required_argument,0,ERUPTMASS},
    {"eruptVolume",required_argument,0,ERUPTVOLUME},
		{"fileAll",required_argument,0,FILEALL},
    {"FileT",required_argument,0,FILET},
    {"fileU",required_argument,0,FILEU},
    {"fileV",required_argument,0,FILEV},
    {"fileZ",required_argument,0,FILEZ},
    {"gridBox",required_argument,0,GRIDBOX},
    {"gridLevels",required_argument,0,GRIDLEVELS},
    {"gridOutput",optional_argument,0,GRIDOUTPUT},
    {"griddedOutput",optional_argument,0,GRIDOUTPUT},
    {"help",no_argument,0,HELP},
    {"latLon",required_argument,0,LATLON},
    {"logFile",required_argument,0,LOGFILE},
    {"lonLat",required_argument,0,LONLAT},
    {"model",required_argument,0,MODEL},
    {"nAsh",required_argument,0,NASH},
    {"newline",optional_argument,0,NEWLINE},
		{"needTemperatureData",no_argument,0,NEEDTEMPERATUREDATA},
    {"nmc",optional_argument,0,NMC},
    {"noFallout",optional_argument,0,NOFALLOUT},
    {"noPatch",optional_argument,0,NOPATCH},
    {"opath",required_argument,0,OPATH},
    {"particleOutput",optional_argument,0,PARTICLEOUTPUT},
    {"path",required_argument,0,PATH},
    {"pickGrid",required_argument,0,PICKGRID},
    {"phiDist",required_argument,0,PHIDIST},
    {"planesFile",required_argument,0,PLANESFILE},
    {"plumeMax",required_argument,0,PLUMEMAX},
    {"plumeMin",required_argument,0,PLUMEMIN},
    {"plumeHwidth",required_argument,0,PLUMEHWIDTH},
    {"plumeZwidth",required_argument,0,PLUMEZWIDTH},
    {"plumeShape",required_argument,0,PLUMESHAPE},
    {"quiet",optional_argument,0,QUIET},
    {"rcfile",required_argument,0,RCFILE},
		{"regionalWinds",required_argument,0,REGIONALWINDS},
    {"repeat",required_argument,0,REPEAT},
    {"restartFile",required_argument,0,RESTARTFILE},
    {"runHours",required_argument,0,RUNHOURS},
    {"runSurface",optional_argument,0,RUNSURFACE},
    {"saveHours",required_argument,0,SAVEHOURS},
    {"saveAshInit",optional_argument,0,SAVEASHINIT},
    {"saveWfile",optional_argument,0,SAVEWFILE},
    {"saveWinds",optional_argument,0,SAVEWFILE},
    {"sedimentation",required_argument,0,SEDIMENTATION},
    {"seed",required_argument,0,SEED},
    {"shiftWest",optional_argument,0,SHIFTWEST},
    {"showVolcs",optional_argument,0,SHOWVOLCS},
    {"silent",optional_argument,0,SILENT},
    {"sorted",required_argument,0,SORTED},
    {"varU",required_argument,0,VARU},
    {"varV",required_argument,0,VARV},
    {"verbose",optional_argument,0,VERBOSE},
    {"version",no_argument,0,PUFF_VERSION},
    {"volc",required_argument,0,VOLC},
    {"volcLon",required_argument,0,VOLCLON},
    {"volcLat",required_argument,0,VOLCLAT},
    {"volcFile",required_argument,0,VOLCFILE},
    {0,0,0,0}
    }; /* end opt_lng */
    int opt_idx=0; /* Index of current long option into opt_lng array */
  /* set default values first */
  set_defaults(&argument);
  // store the command line for the history netCDF attribute
  for (int i = 0; i < argc; i++)
  {
    // add quotes to arguments with spaces
    if (strchr(argv[i],' ')) argument.command_line.append("'");
    argument.command_line.append(argv[i]);
    if (strchr(argv[i],' ')) argument.command_line.append("'");
    argument.command_line.append(" ");
  }
  /* using '+' allows any option, I think */
  while((opt = getopt_long_only(argc,argv,"+",opt_lng,&opt_idx)) != EOF){
    // the switch function is external because the input-file parsing routine
    // uses it also
    option_switch(opt, optarg, opt_lng);
    } /* end while loop */
  /* parse the argument file */
//  if ( argument.argFile ) parse_file(argument.argFile, opt_lng);
  
  // check that the minimum number of arguments has been specified 
  
  // exit if no restartFile or volcano name or lon/lat specified
  if ( !argument.restartFile &&
      strcmp(argument.volc,"none")==0 ) 
  {
    std::cerr << "ERROR: Must specify a volcano name, location, or restart file\n";
    exit(0);
  }
  
  // check for eruption date
  if (!argument.eruptDate)
  {
    std::cerr << "ERROR: Must specify beginning time of the eruption with \"-eruptDate=YYYY MM DD HH:mm\"\n";
    exit(0);
  }
  
  // check for a planes file without phiDist specified
  if (argument.planesFile.size() > 0 && !argument.phiDist )
    std::cout << "WARNING: Flight exposure will be calculated (i.e. -planesFile was specified).  You should also specify a narrow particle distribution (i.e. use -phiDist) to get reliable results.\n";

	// make sure at least one file will be saved
	if (argument.runHours < argument.saveHours)
	{
		std::cerr << "WARNING: adjusting save interval to " << argument.runHours
			        << " hours\n";
		argument.saveHours = argument.runHours;
  }

  return;
}  /* parse_options */

//////////////////////////////////
// contains the options switch only because the command-line and the
// file parser use the same set.
// boolean options retain option=true/false ability for backwards compatibility
//////////////////////////////////
void option_switch(int opt, const char *optarg, const struct option *opt_lng) {
  extern struct Argument argument;
  static bool eruptMass_set = false;
	char east, north;  // used by --lonLat and --latLon options
  switch (opt)
    {
    case ARGFILE: 
			argument.argFile = strdup(optarg);
			parse_file(optarg, opt_lng);
      break;
    case ASHLOGMEAN: 
      if (sscanf(optarg, "%lf", &argument.ashLogMean) == 0)
        std::cerr << "invalid value for option ashLogMean: " << optarg << std::endl;
      break;
    case ASHLOGSDEV: 
      if (sscanf(optarg, "%lf", &argument.ashLogSdev) == 0)
        std::cerr << "invalid value for option ashLogSdev: " << optarg << std::endl;
      break;
    case AVERAGEOUTPUT:
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.averageOutput = false;
 	else if (toupper(optarg[0]) == 84) argument.averageOutput = true; 
 	else 
 	  std::cout << "unrecognized boolean option -averageOutput=" << optarg << std::endl;
         }  // if ( (optarg) && strlen(optarg) > 0 )
      else { argument.averageOutput = true; }
      break;
    case DEM:
      if (strcmp(optarg,"none") == 0) break;
      argument.dem = strdup(optarg);
      if (strchr(argument.dem,':') != NULL) {
        if(sscanf(strchr(argument.dem,':'), ":%i", &argument.dem_lvl) == 0) 
	  std::cerr << "invalid value for dem level: " << optarg << std::endl;
	strtok(argument.dem,":");
      }
      break;
    case DIFFUSEH: 
      if (sscanf(optarg, "%lf", &argument.diffuseH) == 0)
        std::cerr << "invalid value for option diffuseH: " << optarg << std::endl;
      break;
    case DIFFUSEZ: 
      if (sscanf(optarg, "%lf", &argument.diffuseZ) == 0)
        std::cerr << "invalid value for option diffuseZ: " << optarg << std::endl;
      break;
		case DRAG:
				if (sscanf(optarg, "%lf", &argument.drag) == 0)
					std::cerr << "invalid value for option 'drag': "<< optarg << std::endl;
				break;
    case DTMINS: 
      if (sscanf(optarg, "%lf", &argument.dtMins) == 0)
        std::cerr << "invalid value for option dtMins: " << optarg << std::endl;
      break;
    case ERUPTDATE: 
      if (optarg[0] == '+' || optarg[0] == '-')
      {
        time_t tnow = time (NULL);
	double offhrs = strtod(optarg, (char **) NULL);
	tnow += time_t (offhrs * 3600.0);
	argument.eruptDate = strdup(time2unistr(tnow));
      } else if (strlen(optarg) == 16)  {
        argument.eruptDate = strdup(optarg);
      } else if (strlen(optarg) == 12)  {
		  argument.eruptDate = (char*)calloc(17,sizeof(char));
      strncpy(argument.eruptDate, &optarg[0], 4);  // YYYY
	strcat(argument.eruptDate, " ");
	strncat(argument.eruptDate, &optarg[4], 2); // MM
	strcat(argument.eruptDate, " ");
	strncat(argument.eruptDate, &optarg[6], 2); // DD
	strcat(argument.eruptDate, " ");
	strncat(argument.eruptDate, &optarg[8], 2); // HH
	strcat(argument.eruptDate, ":");
	strncat(argument.eruptDate, &optarg[10], 2); // MM
      } else  {
        std::cout << "ERROR: Bad argument: -eruptDate \"" << optarg << "\"" << std::endl;
        std::cout << "  Use one of: " << std::endl;
        std::cout <<
    "  -eruptDate +/-N                 (Relative hours to current time)";
        std::cout << std::endl << "  -eruptDate \"YYYY MM DD HH:MM\" (GMT time)";
        std::cout << std::endl << "FAILED." << std::endl;
	exit(0);
      }
      break;
    case ERUPTHOURS: 
      if (sscanf(optarg, "%lf", &argument.eruptHours) == 0)         
        std::cerr << "invalid value for option diffuseH: " << optarg << std::endl;
      break;
    case ERUPTMASS:
      if (sscanf(optarg, "%lf", &argument.eruptMass) != 1)
      {
        std::cerr << "ERROR: invalid value for -eruptMass: \"" << optarg << "\"\n";
        exit(0);
      }
      if (eruptMass_set)
        std::cerr << "WARNING: Eruption mass was specified more than once, possibly by using both -eruptMass and -eruptVolume.  Puff only allows one specification, and will use the last one, i.e. \"-eruptMass=" << optarg << "\"\n";
      eruptMass_set = true;
      break;
    case ERUPTVOLUME:
      double volume;
      if (sscanf(optarg, "%lf", &volume) != 1)
      {
        std::cerr << "ERROR: invalid value for -eruptVolume: \"" << optarg << "\n";
        exit(0);
      }
      if (strstr(optarg, "km") != NULL)
      {
        // convert it to cubic meters
        volume = 1000*1000*1000*volume;
      }
      // mass = density * volume, assume density is 2000 kg/m^3 = 2 g/cm^3
      // eruption mass is in kg's
      argument.eruptMass = 2000 * volume;
      if (eruptMass_set)
        std::cerr << "WARNING: Eruption mass was specified more than once, possibly by using both -eruptMass and -eruptVolume.  Puff only allows one specification, and will use the last one, i.e. \"-eruptVolume=" << optarg << "\"\n";
      eruptMass_set = true;
      break;
	  case FILEALL:
      argument.fileT = strdup(optarg);
      argument.fileU = strdup(optarg);
      argument.fileV = strdup(optarg);
      argument.fileZ = strdup(optarg);
			break;
    case FILET: 
      argument.fileT = strdup(optarg);
      break;
    case FILEU: 
      argument.fileU = strdup(optarg);
      break;
    case FILEV: 
      argument.fileV = strdup(optarg);
      break;
    case FILEZ: 
      argument.fileZ = strdup(optarg);
      break;
    case GRIDBOX:
      if (optarg) {
        float x1, x2, y1, y2, z1, z2; // dummy variables for sscanf() check
        if (sscanf(optarg, "%f:%f/%f:%f/%f:%f", &x1,&x2,&y1,&y2,&z1,&z2)>1)
	{
	  argument.gridBox = strdup(optarg);
	} else {
	  std::cerr << "WARNING: invalid value for option -gridBox: \"" <<
	    optarg << "\". Should be x1:x2/yl:y2/z1:z2 Using default values.\n";
	}
      } else {
        std::cerr << "ERROR: option -gridBox requires a value.\n";
	exit(0);
      }
      break;
    case GRIDLEVELS:
      if (optarg) 
      {
        if (sscanf(optarg, "%i", &argument.gridLevels) == 1)
	{
	  if (argument.gridLevels < 1)
	  {
	    std::cerr << "ERROR: Grid levels must be >= 1\n";
	    exit(0);
	  }
	} else {
	  std::cerr << "ERROR: Invalid option for -gridLevels: \"" << optarg << "\".  Using default value of " << argument.gridLevels << std::endl;
	}
      }
      break;
    case GRIDOUTPUT:
      argument.gridOutput = true;
      if ( (optarg) && strlen(optarg) > 0 ) {
        float a, b;  // dummy variables for the sscanf() check
	// it is easier to assign these values later
        if (sscanf(optarg, "%fx%f", &a, &b) == 2)
	{
          argument.gridOutputOptions = strdup(optarg);
	} else {
	  std::cerr << "WARNING: invalid value for option -gridOutput: \"" <<
	    optarg << "\".  Using default values.\n";
	}
      }
      break;
    case HELP:
      show_help();
      exit(0);
      break;
    case LATLON:
			// possibilities are 
			// 1) YY/XX with no direction information
			// 2) YYN/XX  or
			// 3) YY/XXE where one has a direction with it
			// 4) YYN/XXE where both have directions.  Test for all possibilities
			if (sscanf(optarg, "%lf%c/%lf%c",   &argument.volcLat, &north, &argument.volcLon, &east) == 4) {
				// do nothing
			} else if (sscanf(optarg, "%lf%c/%lf",   &argument.volcLat, &north, &argument.volcLon ) == 3) {
				east = 'E';
			} else if (sscanf(optarg, "%lf/%lf%c",   &argument.volcLat, &argument.volcLon, &east ) == 3) {
				north = 'N';
		  } else if (sscanf(optarg, "%lf/%lf",   &argument.volcLat, &argument.volcLon ) == 2) {
				east = 'E';
				north = 'N';
			} else { // failure
        std::cout << "bad option --latLon=" << optarg << std::endl;
	exit(0);
      }
			// convert to north and (positive)east values
			if (toupper((int)north) == 'S') argument.volcLat *= -1;
		  if (toupper((int)east) == 'W') argument.volcLon = 360 - argument.volcLon;	
			if (strcmp(argument.volc,"none") == 0) 
			{
				strcpy(argument.volc, "unknown");
			}
      break;
    case LOGFILE:
      if (strncmp(optarg, "-", 1) == 0) {
        std::cerr << "WARNING: invalid log filename: \"" << optarg << "\"" << std::endl;
        }
      else {
        argument.logFile = strdup(optarg);
        argument.quiet = true;
	}
      break;
    case LONLAT:
			// possibilities are 
			// 1) XX/YY with no direction information
			// 2) XXE/YY  or
			// 3) XX/YYN where one has a direction with it
			// 4) XXE/YYN where both have directions.  Test for all possibilities
			if (sscanf(optarg, "%lf%c/%lf%c",   &argument.volcLon, &east, &argument.volcLat, &north) == 4) {
				// do nothing
			} else if (sscanf(optarg, "%lf%c/%lf",   &argument.volcLon, &east, &argument.volcLat ) == 3) {
				north = 'N';
			} else if (sscanf(optarg, "%lf/%lf%c",   &argument.volcLon, &argument.volcLat, &north ) == 3) {
				east = 'E';
		  } else if (sscanf(optarg, "%lf/%lf",   &argument.volcLon, &argument.volcLat ) == 2) {
				east = 'E';
				north = 'N';
			} else { // failure
        std::cout << "bad option --lonLat=" << optarg << std::endl;
	exit(0);
      }
			// convert to north and (positive)east values
			if (toupper((int)north) == 'S') argument.volcLat *= -1;
		  if (toupper((int)east) == 'W') argument.volcLon = 360 - argument.volcLon;	
			if (strcmp(argument.volc,"none") == 0) 
			{
				strcpy(argument.volc, "unknown");
			}
      break;
    case MODEL:
      argument.model = strdup(optarg);
      break;
    case NASH:
      if (sscanf(optarg, "%i", &argument.nAsh) == 0) 
        std::cerr << "WARNING: invalid value for option nAsh: \"" << optarg << std::endl;
      break;
    case NEWLINE:
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.newline = false;
 	else if (toupper(optarg[0]) == 84) argument.newline = true; 
 	else 
 	  std::cout << "unrecognized boolean option -newline=" << optarg << std::endl;
         }  // if ( (optarg) && strlen(optarg) > 0 )
      else { argument.newline = true; }
      break;
    case NMC:
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.nmc = false;
 	else if (toupper(optarg[0]) == 84) argument.nmc = true; 
 	else 
 	  std::cout << "unrecognized boolean option -nmc=" << optarg << std::endl;
         }  // if ( (optarg) && strlen(optarg) > 0 )
      else { argument.nmc = true; }
      break;
    case NOFALLOUT:
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.noFallout = false;
 	else if (toupper(optarg[0]) == 84) argument.noFallout = true; 
 	else 
 	  std::cout << "unrecognized boolean option -noFallout=" << optarg << std::endl;
         }  // if ( (optarg) && strlen(optarg) > 0 )
      else { argument.noFallout = true; }
      break;
    case NOPATCH:
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.noPatch = false;
 	else if (toupper(optarg[0]) == 84) argument.noPatch = true; 
 	else 
 	  std::cout << "unrecognized boolean option -noPatch=" << optarg << std::endl;
         }  // if ( (optarg) && strlen(optarg) > 0 )
      else { argument.noPatch = true; }
      break;
    case OPATH:
      argument.opath  = strdup(optarg);
      if ( argument.opath[strlen(argument.opath)-1] != '/')
        strcat(argument.opath, "/");
      break;
    case PARTICLEOUTPUT:
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.particleOutput = false;
 	else if (toupper(optarg[0]) == 84) argument.particleOutput = true; 
 	else 
 	  std::cout << "unrecognized boolean option -particleOutput=" << optarg << std::endl;
         } else { 
	   argument.particleOutput = true; 
	 }
      break;
      
    case PATH:
      argument.path = strdup(optarg);
      if (argument.path[strlen(argument.path)-1] !=  '/')
        strcat(argument.path, "/");
      break;
    case PICKGRID:
      std::cout << "ERROR: \"--pickGrid option is not available\n";
      exit(0);
      break;
    case PHIDIST:
      argument.phiDist = strdup(optarg);
      break;
    case PLANESFILE:
      argument.planesFile.push_back(optarg);
			argument.gridOutput = true;
      break;
    case PLUMEMAX:
      if (sscanf(optarg, "%lf", &argument.plumeMax) == 0) 
        std::cerr << "invalid value for option plumeMax: " << optarg << std::endl;
      break;
    case PLUMEMIN:
      if (sscanf(optarg, "%lf", &argument.plumeMin) == 0)
        std::cerr << "invalid value for option plumeMin: " << optarg << std::endl;
      break;
    case PLUMEHWIDTH:
      if (sscanf(optarg, "%lf", &argument.plumeHwidth) == 0)
        std::cerr << "invalid value for option plumeHwidth: " << optarg << std::endl;
      break;
    case PLUMEZWIDTH:
      if (sscanf(optarg, "%lf", &argument.plumeZwidth) == 0)
        std::cerr << "invalid value for option plumeZwidth: " << optarg << std::endl;
      break;
    case PLUMESHAPE:
      argument.plumeShape = strdup(optarg);
      break;
    case QUIET:
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.quiet = false;
 	else if (toupper(optarg[0]) == 84) argument.quiet = true; 
 	else 
 	  std::cout << "unrecognized boolean option -quiet=" << optarg << std::endl;
         }  // if ( (optarg) && strlen(optarg) > 0 )
      else { argument.quiet = true; }
      break;
    case RCFILE:
      argument.rcfile = strdup(optarg);
      break;
	  case REGIONALWINDS:
			if (sscanf(optarg, "%lf", &argument.regionalWinds) != 1)
				std::cout << "WARNING: -invalid regionalWinds argument:'" << optarg << "', ignoring\n";
			break;
    case REPEAT:
      if (sscanf(optarg, "%i", &argument.repeat) == 0)
        std::cerr << "invalid value for option repeat: " << optarg << std::endl;
      break;
    case RESTARTFILE:
      if (strcmp(optarg, "none") == 0) break;
      argument.restartFile = strdup(optarg);
      break;
    case RUNHOURS:
      if (sscanf(optarg, "%lf", &argument.runHours) == 0)
        std::cerr << "invalid value for option runHours: " << optarg << std::endl;
      break;
    case RUNSURFACE:
      std::cout << "WARNING: option -runSurface is deprecated and has no effect.\n";
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.runSurface = false;
 	else if (toupper(optarg[0]) == 84) argument.runSurface = true; 
 	else 
 	  std::cout << "unrecognized boolean option -runSurface=" << optarg << std::endl;
         }  // if ( (optarg) && strlen(optarg) > 0 )
      else {argument.runSurface = true; }
      break;
    case SAVEHOURS:
      if (sscanf(optarg, "%lf", &argument.saveHours) == 0)
        std::cerr << "invalid value for option -saveHours: " << optarg << std::endl;
      // check that -saveHours is positive, else die
      if (argument.saveHours <= 0)
      {
        std::cerr << "Invalid value: " << argument.saveHours << ". saveHours must be > 0.\n";
	exit(0);
      }
      if (strstr(optarg, "min") != NULL) 
      {
        argument.saveHours = argument.saveHours/60.0;
      }
      break;
    case SAVEASHINIT:
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.saveAshInit = false;
 	else if (toupper(optarg[0]) == 84) argument.saveAshInit = true; 
 	else 
 	  std::cout << "unrecognized boolean option -saveAshInit=" << optarg << std::endl;
         }  // if ( (optarg) && strlen(optarg) > 0 )
      else { argument.saveAshInit = true; }
      break;
    case SAVEWFILE:
      if ( (optarg) && strlen(optarg) > 0 ) 
      {
        if (toupper(optarg[0]) == 70) 
	{
	  argument.saveWfile = false;
	} else if (toupper(optarg[0]) == 84) {
	  argument.saveWfile = true; 
	} else if (strlen(optarg) == 10) {
	  argument.saveWfilename = optarg;
	  argument.saveWfilename.append("_puff.nc");
	  argument.saveWfile = true;
	}
	  
       else 
 	  std::cout << "unrecognized boolean option -saveWfile=" << optarg << std::endl;
         } else { 
	   argument.saveWfile = true; 
	 }
      break;
    case SEDIMENTATION:
      if ( (strncmp(optarg,"stokes", 6) == 0) ||
           (strncmp(optarg,"Stokes", 6) == 0) ) 
      {
        argument.sedimentation = FALL_STOKES;
      } else if ( (strncmp(optarg,"reynolds",8) == 0) ||
                  (strncmp(optarg,"Reynolds",8) == 0) )
      {
        argument.sedimentation = FALL_REYNOLDS;
				argument.needTemperatureData = true;
      } else if ( (strncmp(optarg,"constant",8) == 0) ||
                  (strncmp(optarg,"Constant",8) == 0) )
      {
        argument.sedimentation = FALL_CONSTANT;
      } else {
        std::cerr << "unknown sedimentation value \"" << optarg << "\"" <<
	std::endl;
      }
      break;
    case SEED:
      if (sscanf(optarg, "%i", &argument.seed) == 0) 
        std::cerr << "invalid value for option seed: " << optarg << std::endl;
      break;

    case SHIFTWEST:
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.shiftWest = false;
 	else if (toupper(optarg[0]) == 84) argument.shiftWest = true; 
 	else 
 	  std::cout << "unrecognized boolean option -shiftWest=" << optarg << std::endl;
         }  // if ( (optarg) && strlen(optarg) > 0 )
      else { argument.shiftWest = true; }
      break;
    case SHOWVOLCS:
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.showVolcs = false;
 	else if (toupper(optarg[0]) == 84) argument.showVolcs = true; 
 	else 
 	  std::cout << "unrecognized boolean option -showVolcs=" << optarg << std::endl;
         }  // if ( (optarg) && strlen(optarg) > 0 )
      else { argument.showVolcs = true; }
      show_volcs();
      exit(0);
      break;
    case SILENT:
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.silent = false;
 	else if (toupper(optarg[0]) == 84) argument.silent = true; 
 	else 
 	  std::cout << "unrecognized boolean option -silent=" << optarg << std::endl;
         }  // if ( (optarg) && strlen(optarg) > 0 )
      else { argument.silent = true; }
      argument.quiet = argument.silent;
      break;
    case SORTED:
      argument.sorted = strdup(optarg);
      break;
    case VARU:
      argument.varU = strdup(optarg);
      break;
    case VARV:
      argument.varV = strdup(optarg);
      break;
    case VERBOSE:
      if ( (optarg) && strlen(optarg) > 0 ) {
        if (toupper(optarg[0]) == 70) argument.verbose = false;
 	else if (toupper(optarg[0]) == 84) argument.verbose = true; 
 	else 
 	  std::cout << "unrecognized boolean option -verbose=" << optarg << std::endl;
         }  // if ( (optarg) && strlen(optarg) > 0 )
      else { argument.verbose = true; }
      break; 
    case PUFF_VERSION:
      std::cout << puff_version_number << std::endl;
      exit(0);
      break;
    case VOLC: 
			// space was allocated in defaults, but copy in the new name
      strcpy(argument.volc,optarg);
      break;
    case VOLCLON:
			if (sscanf(optarg, "%lf%c", &argument.volcLon, &east) == 2)
			{
				// do nothing
			} else if (sscanf(optarg, "%lf", &argument.volcLon) == 1) {
				east = 'E';
		  } else {
        std::cerr << "invalid value for option volcLon: " << optarg << std::endl;
			}
		  if (toupper((int)east) == 'W') argument.volcLon = 360 - argument.volcLon;	
			if (strcmp(argument.volc,"none") == 0) 
			{
				strcpy(argument.volc, "unknown");
			}
      break;
    case VOLCLAT:
			if (sscanf(optarg, "%lf%c", &argument.volcLat, &north) == 2)
			{
				// do nothing
			} else if (sscanf(optarg, "%lf", &argument.volcLat) == 1) {
				north = 'N';
		  } else {
        std::cerr << "invalid value for option volcLat: " << optarg << std::endl;
			}
		  if (toupper((int)east) == 'S') argument.volcLat *= -1;	
			if (strcmp(argument.volc,"none")  == 0) 
			{
				strcpy(argument.volc, "unknown");
			}
      break;
    case VOLCFILE:
      argument.volcFile = strdup(optarg);
      break;  
    } /* end switch */
    
  return;
  } /* end option_switch() */
  
//////////////////////////////////
// set default values for all arguments
//////////////////////////////////
void set_defaults(struct Argument *argument) {
  /* define default for everything for easy reference */
  
	argument->argFile = (char)NULL;
  argument->ashLogMean = -6;
  argument->ashLogSdev = 1;
  argument->averageOutput = false;
  argument->dem = (char)NULL;
  argument->dem_lvl = 2;
  argument->diffuseH = 10000;
  argument->diffuseZ = 10;
	argument->drag = 1.0;
  argument->dtMins = 10;
  argument->eruptDate = (char)NULL;
  argument->eruptMass = 1e9;  // kilograms
  argument->eruptHours = 3;
	argument->fileT = (char)NULL;
  argument->fileU = (char)NULL;
  argument->fileV = (char)NULL;
  argument->fileZ = (char)NULL;
  argument->gridOutput = false;
  argument->gridBox = (char)NULL;
  argument->gridLevels = -1;
  argument->gridOutputOptions = "0.5x2000";
	argument->logFile = (char)NULL;
  argument->model = "puff";
  argument->nAsh = 2000;
  argument->newline = false;
  argument->nmc = false;
  argument->noFallout = false;
  argument->noPatch = false;
  argument->opath = "./";
  argument->particleOutput = true;
  argument->path = "";
  argument->phiDist = (char)NULL;
  argument->planesFile.clear();
  argument->plumeMax = 16000;
  argument->plumeMin = 0;
  argument->plumeHwidth = 0;
  argument->plumeZwidth = 3;
  argument->plumeShape = "linear";
  argument->quiet = false;
  argument->rcfile = (char)NULL;
	argument->regionalWinds = (double)NULL;
  argument->repeat = -1;
  argument->restartFile = (char)NULL;
  argument->runHours = 24;
  argument->runSurface = false;
  argument->saveHours = 6;
  argument->saveWfilename = "wind_puff.nc";
  argument->sedimentation = FALL_CONSTANT;
  argument->seed = 0;
  argument->shiftWest = true;
  argument->showVolcs = false;
  argument->silent = false;
  argument->sorted = "yes";
  argument->varU = (char)NULL;
  argument->varV = (char)NULL;
  argument->verbose = false;
	argument->volc = (char*)calloc(128,sizeof(char));
  strcpy(argument->volc,"none");
  argument->volcLat = (double)NULL;
  argument->volcLon = (double)NULL;
  argument->volcFile = (char)NULL;
  return;
  }

//////////////////////////////////
// show help
//////////////////////////////////
void show_help() {
  std::cout << "Valid options are: (see documentation for futher details)\n";
  std::cout << "  -argFile      filename   (string)\n";
  std::cout << "  -ashLogMean   value      (float)\n";
  std::cout << "  -ashLogSdev   value      (float)\n";
  std::cout << "  -dem          name       (string)\n";
  std::cout << "  -diffuseH     value      (float)\n";
  std::cout << "  -diffuseZ     value      (float)\n";
  std::cout << "  -dtMins       value      (float)\n";
  std::cout << "  -eruptDate    \"YYYY MM DD HH:MM\" (string)\n";
  std::cout << "  -eruptHours   value      (float)\n";
	std::cout << "  -fileAll      filename   (string)\n";
  std::cout << "  -fileT        filename   (string)\n";
  std::cout << "  -fileU        filename   (string)\n";
  std::cout << "  -fileV        filename   (string)\n";
  std::cout << "  -fileZ        filename   (string)\n";
  std::cout << "  -gridBox      x:x/y:y/z:z(string)\n";
  std::cout << "  -gridLevels   value      (integer)\n";
  std::cout << "  -gridOutput   DXxDZ      (string) in degrees x meters\n";
  std::cout << "  -logFile      filename   (string)\n";
  std::cout << "  -lonLat       XX/YY      (string) volcano location\n";
  std::cout << "  -model        value      (string)\n";
  std::cout << "  -nAsh         value      (integer)\n";
  std::cout << "  -newline\n";
  std::cout << "  -nmc [deprecated, does nothing]\n";
  std::cout << "  -noFallout\n";
  std::cout << "  -noPatch\n";
  std::cout << "  -opath        filepath   (string)\n";
  std::cout << "  -path         filepath   (string)\n";
  std::cout << "  -pickGrid     XX/YY      (string)\n";
  std::cout << "  -phiDist      value      (string) i.e. \"1=30;2=70\"\n";
  std::cout << "  -plumeMax     value      (float)\n";
  std::cout << "  -plumeMin     value      (float)\n";
  std::cout << "  -plumeHwidth  value      (float) in km\n";
  std::cout << "  -plumeZwidth  value      (float) in km\n";
  std::cout << "  -plumeShape   shape      (string) e/p/l\n";
  std::cout << "  -quiet\n";
  std::cout << "  -rcfile       filename   (string)\n";
	std::cout << "  -regionalWinds  values   (float)\n";
  std::cout << "  -repeat       value      (integer)\n";
  std::cout << "  -restartFile  filename   (string)\n";
  std::cout << "  -runHours     value      (float)\n";
  std::cout << "  -runSurface [deprecated, does nothing]\n";
  std::cout << "  -saveHours    value      (float)\n";
  std::cout << "  -shiftWest\n";
  std::cout << "  -saveAshInit\n";
  std::cout << "  -saveWinds\n";
  std::cout << "  -showVolcs\n";
  std::cout << "  -sorted       yes/no/never  (string)\n";
  std::cout << "  -varU         name       (string)\n";
  std::cout << "  -varV         name       (string)\n";
  std::cout << "  -verbose\n";
  std::cout << "  -version\n";
  std::cout << "  -volc         name       (string)\n";
  std::cout << "  -volcLat      latitude   (float)\n";
  std::cout << "  -volcLon      longitude  (float)\n";
  std::cout << "  -volcFile     filename   (string)\n";
  
  return;
  }

//////////////////////////////////
// parse the argument file.  It is called after all the command-line
// options have been processed, and attempts to skip commented lines
//////////////////////////////////
void parse_file(const char* inFile, const struct option *opt_lng) {
  int i;  // counter
  int loc = -1; // location with string
  const int MAX_LINE_LENGTH = 256;
  char buffer[MAX_LINE_LENGTH];	// buffer to hold each line read from the file
  std::string textLine, arg, val;
  bool seen_old_format = false;
  
  if (inFile == "") return;
  
  std::ifstream argumentFile (inFile, std::ios::in);
  
  if (!argumentFile) {
    std::cerr << "Failed to open argument file \"" << inFile << std::endl;
    return;
    }
  
  while (! argumentFile.eof() ) {
    argumentFile.getline(buffer, MAX_LINE_LENGTH);
    textLine = buffer;
    // remove trailing comments if they exist
    loc = textLine.find("#");
    if (loc != (int)std::string::npos ) {
      textLine.erase(loc);
      };
    // read next line if there is nothing left
    if (textLine == "") continue;
    
    // remove preceeding '/' for backwards compatibility
    loc = textLine.find("\\");
    if (loc != (int)std::string::npos ) {
      seen_old_format = true;
      if (!seen_old_format ) {
        std::cerr << "Warning: old style input file\n" << textLine << std::endl;
	}
      textLine = textLine.substr(loc+1,textLine.length());
      }
    
    // remove preceeding whitespace from argument
    loc = textLine.find_first_of(" ");
    while (loc == 0) {
      textLine=textLine.substr(1,textLine.length());
      loc = textLine.find_first_of(" ");
      }
      
    // get the first argument
    loc = textLine.find(" ");
    if (loc != (int)std::string::npos ) {
      arg = textLine.substr(0,loc);
      val = textLine.substr(loc+1,textLine.length());      
      }
    else { // options does not require a value
      arg = textLine; 
      val = ""; 
      }
    
    // remove preceeding whitespace from value
    loc = val.find_first_of(" ");
    while (loc == 0) {
      val=val.substr(1,val.length());
      loc = val.find_first_of(" ");
      }
      
    // remove trailing whitespace from value
    loc = val.find_last_of(" ");
    while (val.length() != 0 && loc == (int)(val.length()-1)) {
      val=val.substr(0,val.length()-1);
      loc = val.find_last_of(" ");
      }
    
    // remove quotes from arg and value
    if (arg[0] == '\"') arg=arg.substr(1,arg.length());
    if (val[0] == '\"') val=val.substr(1,val.length());
    if (arg[arg.length()-1] == '\"') arg=arg.substr(0,arg.length()-1);
    if (val[val.length()-1] == '\"') val=val.substr(0,val.length()-1);
    
    // make value NULL if empty because the getopt switch expects that for
    // options without arguments
    
    i = 0;	// reset counter
    bool done = false; // flag to stop looping through options
    // go through all the option in the option structure and look for one that
    // matches 'arg.data()'.  
    while ((opt_lng[i].name != NULL) && (!done)) {
      if (strcmp(opt_lng[i].name, arg.data()) == 0 ) {
	option_switch(opt_lng[i].val,val.data(),opt_lng);
	done = true;
	}
      i++;
      }
    }
    
  return;
  }
