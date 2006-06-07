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

#ifndef PUFF_OPTIONS_H
#define PUFF_OPTIONS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <string>
#include <vector>

enum Sedimentation {FALL_STOKES, FALL_REYNOLDS, FALL_CONSTANT};

struct Argument {
  std::string command_line,
							opath,
  						saveWfilename;
  char *argFile, 
       *dem, 
       *eruptDate, 
       *fileT, 
       *fileU, 
       *fileV,
       *fileZ, 
       *gridBox, 
       *gridOutputOptions, 
       *logFile, 
       *model, 
      // *opath, 
       *path, 
       *phiDist,
       *plumeShape,
       *rcfile, 
       *restartFile, 
       *sorted, 
       *varU, 
       *varV, 
       *volc,
       *volcFile;
  double ashLogMean, 
         ashLogSdev, 
	 diffuseH, 
	 diffuseZ, 
	 drag,
	 dtMins, 
	 eruptHours,
	 eruptMass,
	 plumeMax, 
	 plumeMin, 
	 plumeHwidth, 
	 plumeZwidth, 
	 regionalWinds,
	 runHours, 
	 saveHours, 
	 volcLon, 
	 volcLat;
  int dem_lvl, 
      gridLevels,
      nAsh, 
      repeat, 
      seed;
  bool averageOutput,
       gridOutput,
			 needTemperatureData,
       newline, 
       nmc, 
       noFallout, 
       noPatch, 
       quiet, 
       particleOutput,
       runSurface, 
       saveAshInit, 
       saveWfile, 
       shiftWest, 
       showVolcs, 
       silent, 
       verbose;
  Sedimentation  sedimentation ;
  std::vector<std::string> planesFile;
  };

void parse_options(int argc, char **argv);

/* get the version number via autoconf and config.h */
static const char puff_version_number[] = VERSION;

enum keyWords { ARGFILE, ASHLOGMEAN, ASHLOGSDEV, AVERAGEOUTPUT, DEM, DIFFUSEH, DIFFUSEZ,
DRAG, DTMINS, ERUPTDATE, ERUPTHOURS, ERUPTMASS, ERUPTVOLUME, FILEALL, FILET, FILEU, FILEV, FILEZ, GRIDBOX, GRIDLEVELS, GRIDOUTPUT, HELP, LATLON, LOGFILE, LONLAT,
MODEL, NASH, NEEDTEMPERATUREDATA, NEWLINE, NMC, NOFALLOUT, NOPATCH, OPATH, PARTICLEOUTPUT, PATH, PICKGRID, PHIDIST, PLANESFILE, PLUMEMAX, PLUMEMIN, PLUMEHWIDTH, PLUMEZWIDTH, PLUMESHAPE, QUIET, RCFILE, REGIONALWINDS, REPEAT, RESTARTFILE, RUNHOURS, RUNSURFACE, SAVEHOURS, SAVEASHINIT, SAVEWFILE, SEDIMENTATION, SEED, SHIFTWEST, SHOWVOLCS, SILENT, SORTED, VARU, VARV, VERBOSE, PUFF_VERSION, VOLC, VOLCLAT, VOLCLON, VOLCFILE };

void show_help();

#endif /* PUFF_OPTIONS_H */
