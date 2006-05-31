/*
 * -- puff : software for ash tracking and simulation
 *    Copyright (C) 1999 Craig Searcy 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 
 * 
 * See also the file license.terms
 * 
 * Craig Searcy   
 * National Weather Service, Forecast Office
 * 6930 Sand Lake Road
 * Anchorage, AK 99502-1845
 *
 * craig.searcy@noaa.gov
 *
 * April 1999
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <fstream>		/* log file */
#include <cstdio>

#ifdef MPI_ENABLED
#include <mpi.h>
#endif // MPI_ENABLED

#include "puff.h"
#include "puff_options.h"	/* puff options */
#include "dem.h"
#include "rcfile.h"
#ifdef HAVE_CMAPF_H
#include "cmapf.h"
#else
#include "libsrc/dmapf-c/cmapf.h"
#endif

#include "atmosphere.h"
// Local prototypes:
std::string concFilename(char *oPath, int nm_files);
int make_puffargs (int argc, char **argv);
//int make_winds ();
int make_atmosphere(Atmosphere *atm);
int make_projection_grid ();
int make_puffparams ();
int make_timevars ();
int make_ash (int repeat_count);
void write_ash (time_t ash_t, int repeat_count);
int read_uni (Grid & uni, std::string *filename);
int wind_create_W (Grid & U, Grid & V, Grid & W);
void make_output ();
char *ashTimeHdr (time_t ash_t);
void repeatRunOutput(int run);

// Time output styles:
void refreshTime2 (time_t &time, bool clear = true);

// struct holding command-line options
Argument argument;

// Initialized in make_ash:
Ash ash;
#ifdef HAVE_SSTREAM
std::stringstream ashFilestrm;
#elif HAVE_STRSTREAM
std::strstream ashFilestrm;
#else
#error Do not have <sstream> or <strstream>
#endif

// Initialized in make_winds:
time_t reftime_t;

Atmosphere *atm = new Atmosphere;

// Initialized in make_projection_grid():
maparam *proj_grid;

// Initialized in make_puffparams:
double puff_lon;
double puff_lat;
time_t eruptDate_t;

// digital elevation model object
Dem dem;
// resources
PuffRC resources;

// Initialized in make_timevars:
time_t runHours_t;
time_t saveHours_t;
time_t eruptHours_t;
time_t dtMins_t;
time_t endDate_t;

inline void meter2cart (double &dx, double &dy, double &y)
{
  // fixme: assumes x,y in kilometers and u, v in m/s, this should be verified.
  dx = 0.001 * dx;
  dy = 0.001 * dy;
}

// moved to puff_utils.C
void (*meter2grid) (double &dx, double &dy, double &y);

//////////////////////////////////////////////////////////////////////////
// 
// puff:
//
//////////////////////////////////////////////////////////////////////////
int main (int argc, char **argv)
{

  // make buffer pointers and create the log file stream in the main scope
  // although they might not be used
  std::streambuf * errbuf;
  std::streambuf * outbuf;
  std::ofstream logFile;

	// non MPI has processor rank zero
	int procRank = 0;

#ifdef MPI_ENABLED
	MPI_Init(&argc, &argv);
	// let each process get its rank
	MPI_Comm_rank(MPI_COMM_WORLD, &procRank);
#endif // MPI_ENABLED

	if (isProcController(procRank) ) {
  parse_options (argc, argv);
  if (!resources.init(argument.rcfile)) {
    std::cerr << "No data resource file found\n";
    exit(1);
    }
  if (resources.loadResources(argument.model, "model=") != 0) exit(1);

  // open log file and redirect output if necessary
  if ( argument.logFile ) {
    logFile.open (argument.logFile, std::ios::out);
    if (!logFile) {
      std::cerr << "WARNING: Failed to open log file \"" << argument.logFile << "\". Using default output.\n"; 
    } else {
    // send std::cerr to the log file
    errbuf = std::cerr.rdbuf (logFile.rdbuf ());
    // send cout to the log file
    outbuf = std::cout.rdbuf (logFile.rdbuf ());
    }
  }

	} // isProcController

  // all working processes run Puff:
  if (run_puff (procRank) == PUFF_ERROR) {
    exit (1);
  }

  // restore the buffers
	if (argument.logFile and isProcController(procRank) )
	{
  	std::cerr.rdbuf (errbuf);
  	std::cout.rdbuf (outbuf);
	}
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// 
// run puff:
//
//////////////////////////////////////////////////////////////////////////
int run_puff (int procRank)
{
#ifdef MPI_ENABLED
	int procSize = 1;
	MPI_Comm_size(MPI_COMM_WORLD, &procSize);
#endif // MPI_ENABLED

  // Start message:
  time_t t = time (NULL);
  std::cout << "Begin:  " << asctime (localtime (&t)) << std::endl << std::flush;

  //
  // Initialize Random Number Seed:
  //
  init_seed (iseed, argument.seed);

  if (argument.verbose) {
    std::cout << "Random number seed = " << iseed << std::endl;
  }

  // Get the volcano site:

  if (make_puffparams () == PUFF_ERROR) {
    std::cerr << "\nERROR: make_puffparams() failed\n";
    return PUFF_ERROR;
  }

  // initialize the DEM
  if ( argument.dem ) {
    resources.loadResources(argument.dem, "dem=");
    dem.setPath(resources.getDemPath());
    if (dem.initialize(resources.demType()) != 0)
      std::cerr << "WARNING: failed to initialize dem model " << argument.dem << std::endl;
    if (dem.setResolution(argument.dem_lvl) != 0) return PUFF_ERROR;
    }
 
  // Create wind objects:


  if (make_atmosphere (atm) == PUFF_ERROR) return PUFF_ERROR;

  // Get time-keeping variables:

  if (make_timevars () == PUFF_ERROR)  return PUFF_ERROR;


  // Prepare a projection grid:

  if (make_projection_grid () == PUFF_ERROR) 
  {
    std::cerr << "\nERROR: make_projection_grid() failed" << std::endl;
    return PUFF_ERROR;
  }

  // initialize 'repeat_count', which counts how many repeat runs to do.  If
  // it is zero. the filename still contains the count (which is zero).
  int repeat_count = ((int) argument.repeat >= 0 ? 0 : -1);
  

    // basic Output:    
    make_output ();

    std::cout << "   RUNNING:\n" << std::flush;
    
    if (argument.newline)
      std::cout << std::endl;

  // now repeat this whole loop at least once, and possibly more if 
  // 'repeat_count' is greater than zero
  do {
    
    // Prepare for time integration
    time_t clock_t;
    float secs2hrs = 1. / 3600.;
    float diffHrs;
    time_t printOut_t = 0;

    // differential movement, only need the x,y,z structure stuff actually
    Particle dr (0, 0, 0);

    // Diffusivity constants:
		// if diffusion is variable, these are neglected later on
    float ch = sqrt (2. * argument.diffuseH / double (dtMins_t));
    float cv = sqrt (2. * argument.diffuseZ / double (dtMins_t));

    // if a restartFile is not specified, and atm is not global, data 
    // is necessarily regional and so check if the specified volcano location 
    // is outside the bounds of the data.  We don't check for restartFiles
		// because there is no volcano lon,lat to check.  
		// FIXME: we should check the restartFile boundaries as well
		while (1)
		{
			if (argument.restartFile) break;
			if (atm->isGlobal()) break;
			if (atm->containsXYPoint(puff_lon, puff_lat)) break;
			if (!atm->isProjectionGrid() &&
					(atm->containsXYPoint(puff_lon+360, puff_lat) ) )
			{
				puff_lon+= 360;
				break;
			}
			if (atm->containsXYPoint(puff_lon-360, puff_lat) )
			{
				 puff_lon -= 360;
				 break;
			}
      // if we get here, we must be out of bounds
      	std::cerr << "\nERROR: Volcano site is outside bounds of wind data\n";
	return PUFF_ERROR;
    }
   
    // Create Ash object:
    if (make_ash (repeat_count) == PUFF_ERROR) 
    {
      std::cerr << "\nERROR: make_ash() failed\n";
      return PUFF_ERROR;
    }
    
    //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    //
    // START MAIN INTEGRATION:
    // 
    //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


    // a boolean flag for premature ending, like when all particles are 
    // out-of-bounds or grounded.
    bool EarlyEndOfSimulation = false;

    for (clock_t = eruptDate_t; 
         clock_t <= endDate_t && !EarlyEndOfSimulation; 
	 clock_t += dtMins_t) 
    {
      // diffHrs is the number of hours between the current clock time and
      // the reference time for the data.  The time dimension for the data is
      // relative to this reference time.
      diffHrs = secs2hrs * (clock_t - reftime_t);

      // Print the time to the console:
      if (!argument.quiet) {
	refreshTime2 (clock_t);
      }

      // loop over all the particles in the cloud
      for ( int i = 0; i < ash.n(); i++) {
	// Active particles must meet all these criteria:
	// (1) particle has been "born"
	if ( (clock_t >= ash.start (i) ) &&
	// (2) particle is not on the ground 
	     (!ash.isGrounded(i) ) &&
	// (3) particle exists within the boundary of the wind data
	     (ash.particleExists(i) ) 
#ifdef MPI_ENABLED
	//	(4) this processor's jobs 
		&&		( i % procSize == procRank ) 
#endif // MPI_ENABLED
				)
	{

	    // variable diffusion:
			// -1 is 'turbulent', there could be other options...
			if (argument.diffuseH == -1)
      	ch = sqrt (2. * (atm->diffuseKh(diffHrs, &ash.r[i])) / double (dtMins_t));

	    dr.x = dtMins_t * ch * gasdev (iseed);
	    dr.y = dtMins_t * ch * gasdev (iseed);
	    dr.z = dtMins_t * cv * gasdev (iseed);

#ifdef PUFF_STATISTICS
	    ash.dif_x[i] += fabs (dr.x);
	    ash.dif_y[i] += fabs (dr.y);
	    ash.dif_z[i] += fabs (dr.z);
#endif
	    // Advection:
            dr.x += argument.drag * dtMins_t * atm->xSpeed(diffHrs, &ash.r[i]);
            dr.y += argument.drag * dtMins_t * atm->ySpeed(diffHrs, &ash.r[i]);
            dr.z += argument.drag * dtMins_t * atm->zSpeed(diffHrs, &ash.r[i]);

	    // Fallout:
	    dr.z += atm->fallVelocity(diffHrs, &ash.r[i]);

#ifdef PUFF_STATISTICS
	    // units for adv_x depend on input data.  If atm velocity is m/s
	    // and dtMins_t is seconds, than adv_x is in meters
            ash.adv_x[i] += fabs(dtMins_t * atm->xSpeed(diffHrs, &ash.r[i]) );
            ash.adv_y[i] += fabs(dtMins_t * atm->ySpeed(diffHrs, &ash.r[i]) );
            ash.adv_z[i] += fabs(dtMins_t * atm->zSpeed(diffHrs, &ash.r[i]) );
#endif
	    // Move to grid:
	    meter2grid (dr.x, dr.y, ash.r[i].y);

	    // Update position:
	    ash.r[i] = ash.r[i] + dr;
	    
	    // if the particle is at/below the ground surface, "ground" it
	    if (ash.r[i].z <= dem.elevation(ash.r[i].y, ash.r[i].x, proj_grid) )
	    {
	      ash.r[i].z = dem.elevation(ash.r[i].y, ash.r[i].x, proj_grid); 
	      if (ash.ground(i) != 0) EarlyEndOfSimulation = true;
	    }

	    // allow ash to go over the pole if lat/lon coordinates and global 
	    if ( atm->isGlobal() ) 
	    {
	      if (ash.r[i].y > 90 || ash.r[i].y < -90) 
	      {
		if (ash.r[i].y > 0 )
		{
		   ash.r[i].y = 180 - ash.r[i].y;
		} else {
		  ash.r[i].y = -180 - ash.r[i].y;
		}
	      (ash.r[i].x > 180 ? ash.r[i].x -= 180.0 : ash.r[i].x += 180);
	      }
	      // allow ash to go around the dateline with global data
              if (ash.r[i].x > atm->xMax() )
		ash.r[i].x -= 360;
	      if (ash.r[i].x < atm->xMin() )
		ash.r[i].x += 360;
	      // now ceiling
	      if (!atm->containsZPoint(ash.r[i].z)) 
	      {
	        // reset particle
		ash.r[i].z = atm->zMax();
		// check that this is not the last one moving
		if (ash.outOfBounds(i) != 0)
		  EarlyEndOfSimulation = true;
        }
            
	    // end of global windfield adjustments
	    } else if (!atm->containsXYZPoint(ash.r[i].x, ash.r[i].y, ash.r[i].z) ) {
	    // reset the ash on the boundary if it carried over
	    if (ash.r[i].x < atm->xMin() ) ash.r[i].x = atm->xMin() ;
	    
	    // set this particle non-existant and bail if this is the last 
	    // particle moving (not on the ground or already out-of-bounds)
            if ( ash.outOfBounds(i) != 0) 
	      EarlyEndOfSimulation = true;
	      
	  }  // end checking boundaries

	}  // end loop over non-grounded particles
      }	 //  *** End of nash loop **    

#ifdef MPI_ENABLED
			// only 1 proc should print time and save files
			if (procRank == 0)
			{
#endif //MPI_ENABLED

      // Dump ash data if requested:
      if (printOut_t >= saveHours_t) 
			{
				refreshTime2 (clock_t);
				write_ash (clock_t, repeat_count);
				printOut_t = 0;
      }

      // Update:
      printOut_t += dtMins_t;
      ash.clock () += dtMins_t;
#ifdef MPI_ENABLED
			}
#endif //MPI_ENABLED

    }				// ***End Main Integration***
    //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    //
    // END MAIN INTEGRATION:
    // 
    //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#ifdef MPI_ENABLED
	MPI_Finalize();
#endif // MPI_ENABLED

    // Always dump the last ash data:
    // commented out because this can cause un-uniform spacing between
    // data files, which may be unexpected.  It is a 'user beware' instance
    // if they want the last file printed, then specify it.
    

    // write gridded data
    if (argument.gridOutput)
    {
      std::string outFile = concFilename(argument.opath, repeat_count);
      ash.writeGriddedData(outFile, 
			   (repeat_count == argument.repeat)
			   );
    }

  // add some sort of progress indicator for multiple runs with repeat_count  
  if (argument.repeat > 0 && argument.averageOutput) 
    std::cout << "." << std::flush;
  
  } while (repeat_count++ < (int) argument.repeat);
    // Flush output:
    std::cout << std::endl ;
  std::cout << "Done.\n";
  return PUFF_OK;
}

//////////////////////////////////////////////////////////////////////////
// build a filename for the gridded concentration data based on the eruption
// data. 
//////////////////////////////////////////////////////////////////////////
std::string concFilename(char *opath, int nm_files)
{
  std::string filename = opath;
  filename.append(argument.eruptDate);

  // delete the spaces in the eruption filename netCDF filename
  std::string::size_type loc = filename.find(" ");
  while (loc != std::string::npos)
  {
    filename.erase(loc,1);
    loc = filename.find(" ");
  }
  
  // erase the colon 
  filename.erase(filename.find(":"),1);
  // should now be <path>/YYYYMMDDHHmm
  filename.append("_conc");

  static const int size = 4;    // number of digits+1 in filename..ashXXX.cdf )
  char *buf = new char[size];	// holds the returned string

  // if this is a repeat run, create suffix XXX
  if (nm_files >= 0) 
  {
    snprintf(buf, size+1, "%3i", nm_files);
    // replace spaces with zeros in suffix
    for (int i = 0; i < size; i++) {
      if (strncmp(&buf[i], " ", 1) == 0) strncpy(&buf[i], "0", 1);
      }
  } else { buf[0] = '\0'; }
  // append the file number
  filename.append(buf);
  delete[] buf;
  
  // add suffix
  filename.append(".nc");
  return filename;
}
  
//////////////////////////////////////////////////////////////////////////
void refreshTime2(time_t &time, bool clear)
{
  if (argument.quiet && clear) return;
  
  if (argument.newline && clear ) 
  {
    std::cout << "time: " << time << std::endl;
    return;
  }
  
  char *str = (char*)malloc(22*sizeof(char));
  struct tm *tmnow;
  
   tmnow = gmtime(&time);
   if (tmnow->tm_isdst) (tmnow->tm_hour)--;
   strftime(str, 21, "%Y %m %d %H:%M", tmnow);
  
  // add the timezone.  Using %Z in the above does not seem to work
  // on Solaris, which reports the local timezone since the 'tm' structure
  // does not contain timezone information in it.
  strcat(str," GMT");
  
  std::cout << str << " " << std::flush;
  
  if (!clear) 
  {
    free(str);
    return;
  }
  
  // go backwards so this can be overwritten
  for (unsigned int i = 0; i < strlen(str) + 1; i++)  std::cout << '\b'; 
  
  free(str);
  return;
  
}
////////////////////////////////////////////////////////////////////////
int make_atmosphere(Atmosphere *atm)
{
  if ( atm->init(&puff_lon, &puff_lat) == PUFF_ERROR) return PUFF_ERROR;
  reftime_t = atm->reference_time();
  return PUFF_OK;
}
  
////////////////////////////////////////////////////////////////////////
//
// Prepare NMC grid object, flags, etc:
//
////////////////////////////////////////////////////////////////////////
int make_projection_grid ()
{

  // set protocol for converting lat/lon to meters in grid
  meter2grid = &meter2sphere;
  if (atm->isProjectionGrid() ) 
  {
		proj_grid = new maparam;
    meter2grid = &meter2cart;
    init_grid(( atm->fileU() )->c_str(),proj_grid);
  
    // Switch the site origin to xy grid:
    double x = -1e30, y = -1e30;
    // convert lat/lon to x/y
    cll2xy (proj_grid, puff_lat, puff_lon, &x, &y);
    if ( (x == -1e30) || (y == -1e30) ) {
      std::cerr << "ERROR: failed to convert valcano lat/lon of " << puff_lat
           << "/" << puff_lon << " to a projection grid\n"; 
      return PUFF_ERROR;
    } else {
      puff_lon = x;
      puff_lat = y;
    }
  } else {
		proj_grid = NULL;
		ash.copyRotatedGrid(&atm->rotGrid);
	}

  return PUFF_OK;
}

////////////////////////////////////////////////////////////////////////
//
// Get the volcano site and eruption date:
//
////////////////////////////////////////////////////////////////////////
int make_puffparams ()
{

	// if the latitude and longitude were set directly, use them
  if ((argument.volcLon != (double)NULL) && (argument.volcLat != (double)NULL)) {
    puff_lon = argument.volcLon;
    puff_lat = argument.volcLat;
  }
  // else if the volcano name was given,use it
  else if (strcmp (argument.volc, "unknown")) {
    get_lon_lat (argument.volc, puff_lon, puff_lat);
	} else if (argument.restartFile) {
		// it is OK if only a restart file is used
  } else {
    std::cout << "ERROR: no volcano specified\n";
    return PUFF_ERROR;
	}

	// add restart files if given
  if ( argument.restartFile ) 
	{
		// if only a restart (not second eruption) set name to 'none'
	  if (!argument.volc) argument.volc = strdup("none");
  }

  if (argument.shiftWest && puff_lon < 0 && puff_lon != puff_undef_dbl) {
    puff_lon += 360;
  }

// only check the volc lon/lat values if a restart file is not specified
  if ( !argument.restartFile ) {
    if (check_lon_lat (argument.volc, puff_lon, puff_lat) == PUFF_ERROR) {
      return PUFF_ERROR;
    }
  }  // end if restartFile

  // Puff works in UTC only, so set/overwrite the TZ environment variable
  // to UTC.  This is necessary for mktime() to return UTC (and not local)
  // time_t values.  Format is "std offset dst offset, rule", so here we set
  // both standard and daylight saving to UTC with a zero offset, and omit
  // any rule to change between standard and daylight 
#ifdef HAVE_SETENV
  setenv ("TZ","UTC0UTC",1);
#elif defined HAVE_PUTENV
	char *tz_string = strdup("TZ=UTC0UTC");
  putenv(tz_string);

  putenv("TZ=UTC0UTC");
# else
#error neither getenv() or putenv() functions are available
#endif

  // Now call tzset(), which will process the TZ env. variable so mktime()
  // works properly
  tzset ();

  // Set eruptDate_t
  eruptDate_t = unistr2time (argument.eruptDate);

  return PUFF_OK;
}

////////////////////////////////////////////////////////////////////////
//
// Make time-keeping variables:
//
////////////////////////////////////////////////////////////////////////
int make_timevars ()
{

  //
  // runHours, eruptHours, saveHours:
  //
  runHours_t = time_t (argument.runHours * 3600.0);
  if (runHours_t < 0) {
    std::cout << "ERROR: negative runHours specified: "
      << argument.runHours << std::endl;
    std::cout << "FAILED." << std::endl;
    return PUFF_ERROR;
  }

  eruptHours_t = time_t (argument.eruptHours * 3600.0);
  if (eruptHours_t < 0) {
    std::cout << "ERROR: negative eruptHours specified: "
      << argument.eruptHours << std::endl;
    std::cout << "FAILED." << std::endl;
    return PUFF_ERROR;
  }

  saveHours_t = time_t (argument.saveHours * 3600.0);
  if (saveHours_t < 0) {
    std::cout << "ERROR: negative saveHours specified: "
      << argument.saveHours << "\nFAILED.\n";
    return PUFF_ERROR;
  }

  // Time step:
  dtMins_t = time_t (argument.dtMins * 60.0);
  if (dtMins_t < 0) {
    std::cout << "ERROR: negative time step dtMins specified: "
      << argument.dtMins << std::endl;
    std::cout << "FAILED." << std::endl;
    return PUFF_ERROR;
  }

  //
  // Use run length if eruptHours > runHours:
  //
  if (eruptHours_t > runHours_t) {
    eruptHours_t = runHours_t;
  }

  //
  // Make sure print or run length don't overstep the integration length:
  //
  if (saveHours_t < dtMins_t) 
  {
    std::cout << "WARNING: adjusting save interval from " << saveHours_t << "sec to " << dtMins_t << "sec to match the time step dtMins of " << dtMins_t << "sec.\n";
    saveHours_t = dtMins_t;
  }
  if (runHours_t < dtMins_t) {
    runHours_t = dtMins_t;
  }

  // Save endDate:
  endDate_t = eruptDate_t + runHours_t;

  return PUFF_OK;
}


////////////////////////////////////////////////////////////////////////
//
// Make Ash object.  There are several options here: a restart file, 
// only a volcano, or both.  If argument.volc is "none", there is only
// a restart.
//
////////////////////////////////////////////////////////////////////////
int make_ash (int repeat_count)
{
  ash.initialize();
  if ( argument.restartFile ) 
	{
    // set a flag whether this is a multiple eruption or not
    int multE = (strcmp (argument.volc, "none")); 

    ash.init_site_custom (multE, proj_grid);
  } // end if arg.set(restartFile)

	// make ash at this volcano, unless there is not one
  if (strcmp (argument.volc, "none") != 0)
	{
    // allocate memory if it was not done with the restart file
    if (ash.n () == 0) {
      if (ash.create (int (argument.nAsh)) == ASH_ERROR)
	  return PUFF_ERROR;
    }
    
    if ((ash.clock () != unistr2time (argument.eruptDate)) &&
	(ash.isAshFile (argument.restartFile))) {
      std::cout << "WARNING: eruption time and file datestamp are not equal. ";
      std::cout << "(" << ash.clock () -
	unistr2time(argument.eruptDate) << " s).\n";
    }
    ash.init_site (puff_lon, puff_lat, argument.volc);

    // Plume shape:
    switch (argument.plumeShape[0]) {
    case ('l'):{
	ash.init_linear_column (argument.plumeMax, argument.plumeMin);
	break;
      }
    case ('p'):{
	ash.init_poisson_column (argument.plumeMax, argument.plumeMin,
				 argument.plumeZwidth);
	break;
      }
    case ('e'):{
	ash.init_expon_column (argument.plumeMax,
			       1000 * argument.plumeZwidth,
			       argument.plumeMin);
	break;
      }
    default:{
	std::cout << "ERROR: Bad argument -plumeShape \""
	  << argument.plumeShape << "\"\n"
	  << "   Use one of \"linear\", \"exponential\", \"poisson\"" << std::endl;
	std::cout << "FAILED." << std::endl;
	return PUFF_ERROR;
      }
    }
  }  // if a volcano was specified 

  // check for empty ash
  if (ash.n () == 0) 
	{
    std::cout << "ERROR: no ash particles.  Specify a volcano or filename\n";
    return PUFF_ERROR;
  }

  // Init horizontal spread:
  if (argument.plumeHwidth > 0) {
    ash.horiz_spread (1000 * argument.plumeHwidth, argument.plumeMax,
		      argument.plumeMin);
  }
  // initialize the size distribution.  Several options may have been
  // set, but we'll sort that out in the member function
  if (ash.init_size (argument.ashLogMean, 
                     argument.ashLogSdev,
		     argument.phiDist) != PUFF_OK) return PUFF_ERROR;
  ash.init_age (eruptDate_t, eruptHours_t);
#ifdef PUFF_STATISTICS
  ash.clearStats ();
#endif
//  strcpy (ash.sorted, argument.sorted);
  ash.setSortingProtocol(argument.sorted);
  ash.quicksort();

  // Save puff paramters in ash object:
  ash.erupt_hours = float (eruptHours_t) / 3600.0;
  ash.plume_height = float (argument.plumeMax);
  ash.plume_min = float (argument.plumeMin);
  strcpy (ash.plume_shape, argument.plumeShape);
  ash.diffuse_h = float (argument.diffuseH);
  ash.diffuse_v = float (argument.diffuseZ);
  ash.plume_width_z = float (argument.plumeZwidth);
  ash.plume_width_h = float (argument.plumeHwidth);
  ash.log_mean = float (argument.ashLogMean);
  ash.log_sdev = float (argument.ashLogSdev);


  // clear the stash of concentration data before writing the initial dump
  ash.clearStash();

  // Dump Initial ash data:  saving initial makes no sense with repeat runs
  // because they are all the same
  if (argument.saveAshInit && argument.repeat <= 0) {
    write_ash (eruptDate_t, repeat_count) ;
  }

  return PUFF_OK;
}

////////////////////////////////////////////////////////////////////////
//
// Write Ash object
//
////////////////////////////////////////////////////////////////////////
void write_ash (time_t ash_t, int nm_files)
{
  
  std::string ashFilename;
  static double xlon, ylat;

  static const int size = 4;    // number of digits+1 in filename..ashXXX.cdf )
  char *buf = new char[size];	// holds the returned string

  // if this is a repeat run, create suffix XXX
  if (nm_files >= 0) 
	{
    snprintf(buf, size+1, "%3i", nm_files);
    // replace spaces with zeros in suffix
    for (int i = 0; i < size; i++) 
		{
			if (buf[i] == ' ') buf[i] = '0';
    }
  } else { buf[0] = '\0'; }
  
  // set global attribute date/time stamp
  strcpy(ash.date_time, ashTimeHdr(ash_t));
  ashFilename = argument.opath;
  ashFilename.append(ashTimeHdr(ash_t) );
  ashFilename.append("_ash");
  ashFilename.append(buf);
  ashFilename.append(".cdf");
  
  delete[] buf;
  
  // Convert to Lon/Lat:
  if (atm->isProjectionGrid() ) 
	{
    cxy2ll (proj_grid, xlon, ylat, &ash.origLat,
	    &ash.origLon);
    ash.origLon = xlon;
    ash.origLat = ylat;
    for (int i = 0; i < ash.n (); i++) {
      cxy2ll (proj_grid, ash.r[i].x, ash.r[i].y, &ylat, &xlon);
      ash.r[i].x = xlon;
      ash.r[i].y = ylat;
    }
    // convert to positive longitude (fixme: should not be necessary)
    for (int i = 0; i < ash.n (); i++) {
      if (ash.r[i].x < 0)
	ash.r[i].x += 360.0;
    }
  }
  // Write:
  // don't bother if we are not writing particle files
  if (argument.averageOutput && nm_files != argument.repeat) 
  {
    // do nothing
  } else {
   ash.write (ashFilename.c_str() );
  }
  
  // write concentration data
  if (argument.gridOutput)
  {
    ash.stashData(ash_t);
  }
  
  // Convert back to xy:
  if (atm->isProjectionGrid() ) 
  {
    cll2xy (proj_grid, ash.origLat, ash.origLon,
	    &xlon, &ylat);
    ash.origLon = xlon;
    ash.origLat = ylat;
    for (int i = 0; i < ash.n (); i++) 
    {
      cll2xy (proj_grid, ash.r[i].y, ash.r[i].x, &xlon, &ylat);
      ash.r[i].x = xlon;
      ash.r[i].y = ylat;
    }
  }

  return;;
}

////////////////////////////////////////////////////////////////////////
//
// Format Ash file header
//
////////////////////////////////////////////////////////////////////////
char *ashTimeHdr (time_t ash_t)
{
  struct tm *tmnow;
//  tmnow = (tm*)malloc(sizeof(tm));
  static char *hdrstr = new char[16];

  for (int i = 0; i < 16; i++) {
    hdrstr[i] = '\0';
  }

  tmnow = gmtime (&ash_t);
  
  if (tmnow->tm_isdst) (tmnow->tm_hour)--;
  
  strftime (hdrstr, 13, "%Y%m%d%H%M", tmnow);

  return hdrstr;
}

//////////////////////////////////////////////////////////////////////
//
// Make basic output
//
///////////////////////////////////////////////////////////////////////
void make_output ()
{

  // Site:
  std::string s = argument.volc;
  std::string::size_type underscore = s.find ("_");
  if (underscore != std::string::npos) {
    s.replace (underscore, 1, " ");
  }
  std::cout << "Volcano:\t" << s << " ( " << puff_lon << " , " << puff_lat << " )"
    << std::endl;

  std::cout << std::endl;
  std::cout << "Start Time:\t";
 refreshTime2 (eruptDate_t, false);
  std::cout << "   (" << eruptDate_t << ")" << std::endl;
  std::cout << "  End Time:\t";
  refreshTime2 (endDate_t, false);
  std::cout << "   (" << endDate_t << ")" << std::endl;
  std::cout << "           \t" << "____________________" << std::endl;

}
///////////////////////////////////////////////////////////////////////
//void repeatRunOutput(int repeat_count)
//{
//  if (repeat_count < 0) return;
//  static int len = (int)logf((float)argument.repeat)*2+4;
//  if (repeat_count > 0)
//  {

						

  
