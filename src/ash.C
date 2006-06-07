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
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>  // sscanf
#include <string>
#include <cmath> // M_PI definition, pow()
#include <vector>
#include <algorithm>
#include <unistd.h> // access system call
#include <sys/types.h> // open()/close()
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_NETCDFCPP_H
#include <netcdfcpp.h>
#else
#include "netcdfcpp.h"
#endif // HAVE_NETCDFCPP_H

#include "ash.h"
#include "ran_utils.h"
#include "cloud.h"   
#include "atmosphere.h"
#include "puff_options.h" // argument structure
#include "puff.h"

// if this is not already defined
#ifndef HAVE_LOGF
#define logf(x) log(reinterpret_cast<float>(x))
#endif

const int ASH_ERROR = 1;
const int ASH_OK = 0;

// VARIABLES DECLARED EXTERNALLY:
int iseed;

// Local prototypes
extern void Tokenize(const std::string&, 
                     std::vector<std::string>&, 
		     const std::string&);

extern Atmosphere atm;
extern Argument argument;

///////////////////////////////////////////////////////////////////////
//
// CONSTRUCTORS:
//
//////////////////////////////////////////////////////////////////////
Ash::Ash() {
    ashN = 0;
    initialize();
}

Ash::Ash(long n) {
    ashN = n;
    initialize();
    allocate();
}

int Ash::create(long n) {
    ashN = n;
    return allocate();
}

Ash::~Ash() {
    if ( particle ) {
	delete[] particle;
    }
}

///////////////////////////////////////////////////////////////////////
//
// INITIALIZE:
//
//////////////////////////////////////////////////////////////////////
void Ash::initialize() {

  // POINTERS:
//  r = NULL;
    
  // VARIABLES:
  origLon = 0;
  origLat = 0;
  origTime = 0;
  clockTime = 0;
    
  minlat = 90;
  maxlat = -90;
  minlon = 1e30;
  maxlon = -1e30;
  minhgt = 1e30;
  maxhgt = 0;
    
  numGrounded = 0;
  numOutOfBounds = 0;

  // assume all particles initially exist and are ungrounded
  // do this here since -repeat does not recreate the ash object but does
  // initialize() it again
  for (int i = 0; i<ashN; i++) 
  {
    particle[i].grounded = false;
    particle[i].exists = true;
  }
  
  return;    
}

///////////////////////////////////////////////////////////////////////
//
// ALLOCATE:
//
//////////////////////////////////////////////////////////////////////
int Ash::allocate() 
{
  // with repeat runs, this space is already allocated, so don't repeat
	if (particle) return ASH_OK;

  particle = new Particle[ashN];
  if ( !particle ) {
    std::cerr << "\nash ERROR: new failed.\n";
    return ASH_ERROR;
  }
    
  r = particle;
  
    
#ifdef PUFF_STATISTICS    
  dif_x = new double[ashN];
  dif_y = new double[ashN];
  dif_z = new double[ashN];
  adv_x = new double[ashN];
  adv_y = new double[ashN];
  adv_z = new double[ashN];
  if (!dif_x || !dif_y || !dif_z || !adv_x || !adv_y || !adv_z) {
    std::cerr << "\nash ERROR: new dif or adv failed.\n";
    return ASH_ERROR;
  }
#endif    
   return ASH_OK; 
}
///////////////////////////////////////////////////////////////////////
// initialize a simulation using a restart file.  There are four possible
// scenarios when getting here:
// * using a puff-generated ash file with/without another eruption
// * using a user-specified cloud discription file w or w/o another eruption
//  The number of particles in the new simulation for user-specified with
// another eruptions is the value 'n' passed the -nAsh argument evenly split
// between the two ('multE_to_cloud_frac=0.5').  Otherwise, particle number
// is read in from the ash file for the existing cloud and -nAsh argument used
// for the new eruption.
///////////////////////////////////////////////////////////////////////

void Ash::init_site_custom(int multE, maparam* proj_grid) {
  double *tempLLValues;
  int error;
  static const float multE_to_cloud_frac=0.5;
	char *restartFile = argument.restartFile;
  
  if ( (ashNpart = isAshFile(restartFile) ) > 0) 
	{
    if (strcmp(argument.volc, "none") == 0)
		{
			create(ashNpart);
			ashN=(long)(ashNpart);
		} else {
			create(ashNpart+argument.nAsh);
			ashN=(long)(ashNpart+argument.nAsh);
		}
//    ashN=0;	// reset to zero to allow for reading
    std::cout << "Reading " << restartFile << " ... " << std::flush;
    error = read(restartFile);
    if (error) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Read failed for " << restartFile << std::endl;
      exit(1);
      }
    std::cout << "done." << std::endl;

  } else {
		// The restart file is a cloud specification, so create the 
		// cloud and put the particle locations into the ash object
    ashN=argument.nAsh;
    create(ashN);
    Cloud cloud(ashN, restartFile);
       
    strcpy(origName, "Restart");
 
    if ( multE ) ashNpart=(long)(multE_to_cloud_frac*ashN);
    else         ashNpart=ashN; 
    tempLLValues = cloud.customCloud(ashNpart );

// load cloud values into the ash object    
    for (int i=0; i<(ashNpart); i++) 
		{
      particle[i].x = tempLLValues[i];
      particle[i].y = tempLLValues[ashNpart+i];
      particle[i].z = tempLLValues[2*ashNpart+i];
      particle[i].size = cloud.size();
      particle[i].startTime = -1;
    }
  }

	// If the atmosphere is a projection grid, convert the lat/lon
	// values to this grid
	if (proj_grid)
	{
		double x, y;
		for (int i = 0; i < ashN; i++)
		{
			// convert lat,lon to x,y
			cll2xy(proj_grid, particle[i].y, particle[i].x, &x, &y);
			particle[i].x = x;
			particle[i].y = y;
		}
	}	
  return;
}

///////////////////////////////////////////////////////////////////////
//
// INITIALIZE SITE
//
//////////////////////////////////////////////////////////////////////
void Ash::init_site(float lon, float lat, char *name) {

  // max name length is 120 including null-termination    
    strncpy(origName, name, 119);
    origName[119] = '\0';
    
    origLon = static_cast<double>(lon);
    origLat = static_cast<double>(lat);
          
    for (int i=ashNpart; i<ashN; i++) {
	particle[i].x = origLon;
	particle[i].y = origLat;
    }
  return;
}

///////////////////////////////////////////////////////////////////////
//
// INITIALIZE LINEAR COLUMN
//
//////////////////////////////////////////////////////////////////////
void Ash::init_linear_column(float height, float bottom) {
    
    // Linear column distribution
    for (int i=ashNpart; i<ashN; i++) {
	particle[i].z = double(bottom) + double((height-bottom)*ran1(iseed));
    }    
  return;
}

///////////////////////////////////////////////////////////////////////
//
// INITIALIZE EXPONENTIAL COLUMN
//
//////////////////////////////////////////////////////////////////////
void Ash::init_expon_column(float height, float width, float bottom) {
    static float spread;
    
    // Linear column height distribution
    for (int i=ashNpart; i<ashN; i++) {
	
	
	// Make sure it is positive
	particle[i].z = -1.0;
	while( particle[i].z < bottom ) {
	    spread = 0.25*width*ran1(iseed);
	    particle[i].z = height - double(width*expdev(iseed)) + spread;
	}
    }    
  return;
}

///////////////////////////////////////////////////////////////////////
//
// INITIALIZE "POISSON" COLUMN
//
//////////////////////////////////////////////////////////////////////
void Ash::init_poisson_column(float height, float bottom, float width) {
  static float random_number;

  for (int i=ashNpart; i<ashN; i++) {
	
    // Make sure it is positive
    particle[i].z = -1.0;
    while( particle[i].z < bottom || particle[i].z > height) {
      random_number=poi_dist(width, iseed);
      particle[i].z = height - random_number*1000;
      }
  }
  return;
}

///////////////////////////////////////////////////////////////////////
//
// HORIZONTAL SPREAD:
// ASSUMES HEIGHT AND SITE HAS BEEN INITIALIZED:
//
//////////////////////////////////////////////////////////////////////
void Ash::horiz_spread(float width, float height, float bottom) {
  if ( width <= 0.0 ) {
    return;
  }
    
  static float theta;
  static Particle dr;
  static float zfrac;
    
  static double Re = PUFF_EARTH_RADIUS;
  static double Deg2Rad = M_PI/180.0;
  static double Rad2Deg = 180.0/M_PI;
    
  for (int i=ashNpart; i<ashN; i++) {
    theta = 2*PI*ran1(iseed);
	
    dr.x = width*cos(theta);
    dr.y = width*sin(theta);
    dr.z = 0.0;
	
    zfrac = (particle[i].z-bottom)/(height-bottom)*ran1(iseed);
    dr.x = zfrac*Rad2Deg*dr.x/(Re*cos(Deg2Rad*particle[i].y));
    dr.y = zfrac*Rad2Deg*dr.y/Re;
	
    // near the pole, dr.x can be ridiculously large
    while (dr.x > 360) { dr.x -=360;}
    while (dr.x < -360  ) { dr.x +=360;}
    particle[i] = particle[i] + dr;
  }
  return;
}

///////////////////////////////////////////////////////////////////////
//
// Initialize the age of the ash particles.  This is a distribution over
// 'eruptHours", or all the same if they are from a restart file.
//
//////////////////////////////////////////////////////////////////////
void Ash::init_age(long erupt_time, long lengthSecs) {
    
    origTime = erupt_time;
    clockTime = erupt_time;
    
    // loop over all particles, 'age'ing them two ways:
    // if -1: from restart file, so all ages are eruption time
    // otherwise: linear distribution over lengthSecs
    for (int i=ashNpart; i<ashN; i++) {
      if (particle[i].startTime == -1 ) {
        particle[i].startTime = origTime;
	}
      else {
	particle[i].startTime = origTime + long(ran1(iseed)*float(lengthSecs));
	}
    }
    
}

///////////////////////////////////////////////////////////////////////
//
// initialize the size of the particles.
// phiDist is a string describing a size distribution where 
// phi = -log_base_2 of the particle size in mm, so d[mm] = 2^(-phi).
// The string is a list of "phi=percent" and should be something like 
// "1=30;0=40;-1=30".  If the percentages do not add to 100, normalize.
// If the other options "ashLogMean" and/or "ashLogSdev" are specified,
// "phiDist" will override the others.
//
//////////////////////////////////////////////////////////////////////
int Ash::init_size(double logMean, double logSdev, char* phiDist)
{  
  // if -phiDist is not specified, use a gaussian distribution for size
  if ( !phiDist || (strlen(phiDist) == 0) ) {
    double logSize;  // temporary holder of a value
    for (int i=ashNpart; i<ashN; i++) {
      logSize = logMean + logSdev*gasdev(iseed);
      particle[i].size = pow(10., logSize);
      }
  } else {
    // use the specified phi distribution
    std::string phiString = phiDist;  // create a string for easier parsing
    std::vector<std::string> valuePairs;   // the pairs, i.e. 2=23.2
    std::vector<double> phi, percent; // the individual values
    Tokenize(phiString, valuePairs, ";");
    std::vector<std::string>::const_iterator pVal;  // pointer to value pairs
    // loop through all the value pairs, parsing the phi value from
    // the percent value.
    for (pVal=valuePairs.begin(); pVal!=valuePairs.end(); pVal++) {
      std::string::size_type loc = 0;  // location in each string
      std::string phiStr, percentStr;  // temporarily hold these values
      double phiDouble, percentDouble;
      loc = (*pVal).find("="); // find the '=' delimiter
      if (loc == std::string::npos) {
        std::cerr << "ERROR: invalid use of option -phiDist=\"" << phiDist <<"\". ";
	std::cerr << "\"" << *pVal << "\" is not a valid value pair. ";
	std::cerr << "It should be a list of phi=percent, i.e. \"1=30;2=70\"\n";
	return ASH_ERROR;
	}
      // assign phi and percent
      phiStr = (*pVal).substr(0,loc);
      percentStr = (*pVal).substr(loc+1,(*pVal).length());
      // attempt to put the value pairs into doubles
      if (sscanf(phiStr.c_str(), "%lf", &phiDouble) != 1) {
        std::cerr << "ERROR: \"" <<phiStr << "\" should be a numeric value ";
	std::cerr << "value in -phiDist=\"" << phiDist << "\"\n";
	return ASH_ERROR;
	}

      if (sscanf(percentStr.c_str(), "%lf", &percentDouble) != 1) {
        std::cerr << "ERROR: \"" <<percentStr << "\" should be a numeric value ";
	std::cerr << "value in -phiDist=\"" << phiDist << "\"\n";
	return ASH_ERROR;
	}
      
      if (percentDouble < 0 ) {
        std::cerr << "ERROR: negative percentage value in -phiDist=\"";
	std::cerr << phiDist << "\" makes no sense.\n";
	return ASH_ERROR;
	}
      // add phi and percent to their respective vectors
      phi.push_back(phiDouble);
      percent.push_back(percentDouble);
      }  // end loop over value pairs
    
    // create iterators for phi and percentage
    std::vector<double>::iterator pPhi, pPercent;
    
    // convert mass percentages to number percentages, because we work with
    // individual particles.  Get this from mass percentages by dividing each
    // mass percentage by the cube of the corresponding diameter since mass is
    // proportional to the cube of the diameter.
    
    // normalize percentages
    double total = 0.0;
    for (pPercent=percent.begin(); 
         pPercent != percent.end();
	 pPercent++) {
      total+=(*pPercent);
      }
    // normalize the vector from 0 -> 1
    for (pPercent=percent.begin();
         pPercent != percent.end();
	 pPercent++) {
      *pPercent /= total;
      }	 
    
    // make percentages by mass
    pPhi=phi.begin();
    for (pPercent=percent.begin();
         pPercent!=percent.end();
	 pPhi++,pPercent++)
      {
        (*pPercent)=(*pPercent)/pow(pow(2,(-1)*(*pPhi)),3);
      } 

    // re-normalize percentages
    total = 0.0;
    for (pPercent=percent.begin(); 
         pPercent != percent.end();
	 pPercent++) {
      total+=(*pPercent);
      }
    // normalize the vector from 0 -> 1
    for (pPercent=percent.begin();
         pPercent != percent.end();
	 pPercent++) {
      *pPercent /= total;
      }	 
    
    pPhi=phi.begin();  // start pointer at the first phi value
    int first = ashNpart; // the first size value to assign
    int next;             // the next value to iterate to on each step
		double thisSize;      // local copy of the current size
    int totalSize = (ashN-ashNpart);  // total size values to assign
    for (pPercent=percent.begin(); 
         pPercent != percent.end(); 
	 ( pPercent++ , pPhi++) ) {
      next = (int)floor(first+(*pPercent)*totalSize);  
			// don't assign beyond totalSize
			while (next >= totalSize) next--;
      // assign the specified size to this group of particles
      for (int i = first; i <= next; i++) {
        thisSize = pow(2, (-1)*(*pPhi)) / 1000; // size is in meters
        particle[i].size = thisSize; // size is in meters
	}
      // assign the next value to count to
      first = next;
      }
		// some particles might have gotten missed due to round-off error, assign
		// those here
		while(next < totalSize) particle[next++].size = thisSize;
       
    } // end -phiDist option used
    
  // determine the percent of total mass each particle represents
  // do this for all particles, not just ashNpart now that we know what 
  // all particles are
  double total_mass = 0.0;
  double density = 2e6;  // milligrams per meter^3
  for (int i = 0; i < ashN; i++)
  {
    total_mass += 4/3*M_PI*pow(particle[i].size,3)*density;
  }
  
  for (int i =0; i < ashN; i++)
  {
    particle[i].mass_fraction = 4/3*M_PI*pow(particle[i].size,3) *
                               density / total_mass;
  }
    
  return ASH_OK;   
}
//////////////////////////////////////////////////////////////////////
//
//  calculate fall velocity
//
//////////////////////////////////////////////////////////////////////
double Ash::fallVelocity(int idx) {
  if (argument.sedimentation == FALL_STOKES)
  {
    static const double GravConst =  (2./9.)*(1.08e9);
    return -GravConst*particle[idx].size*particle[idx].size;
  } else if (argument.sedimentation == FALL_REYNOLDS) {
    static const double GravConst =  (2./9.)*(1.08e9);
    return -GravConst*particle[idx].size*particle[idx].size;
  }
  else {
  // shouldn't get here
  std::cerr << "ERROR: sedimentation type is undefined\n";
  exit(0);
  }
}   
  
///////////////////////////////////////////////////////////////////////
//
// write netCDF file using C++ interface
//
//////////////////////////////////////////////////////////////////////
#define PUFF_FILE_PERMISSIONS S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH
#define PUFF_DIR_PERMISSIONS S_IRWXU|S_IRWXG|S_IRWXO
void Ash::write(const char *filename) {

  std::cout << "Saving " << filename << std::endl;

  if (sorting_protocol == ASH_SORT_YES) quicksort();
  findLimits();

	// open/create file
	int fd = open(filename, O_CREAT|O_APPEND,PUFF_FILE_PERMISSIONS);
	// if failed, maybe directory doesn't exist
	if (fd < 0) mkdir(argument.opath.c_str(),PUFF_DIR_PERMISSIONS);
	// try again
	if (fd < 0) fd = open(filename, O_CREAT,PUFF_FILE_PERMISSIONS);
	if (fd > 0) { (void)close(fd);}
	else {
		std::cout << "ERROR: Failed to create output file " << filename << "\n";
		exit(0);
		}
  NcFile ncfile(filename, NcFile::Replace); // create/clobber a file
	if (!ncfile.is_valid()) {
		std::cerr << "ERROR: failed to create output file\n";
		exit(0);
		}
  // create a dimension
  NcDim *dp = ncfile.add_dim((NcToken)"nash", ashN);
  NcVar *vp;  // create a pointer to a NcVar object

  // create a temporary array for writing data, since we want to use the 
  // order array which is sorted
  double *loc = new double[ashN];
  ncbyte *gnd = new ncbyte[ashN];  // hold 'grounded' flag, ncbyte is simply
  				   // a char, which is also the size of a bool

  // add location, age and size information
  for (int i = 0; i<ashN; i++) { loc[i]=particle[particle[i].order].startTime; }
  vp = ncfile.add_var((NcToken)"age", ncDouble, dp);
  vp->put(loc, ashN);
  vp->add_att((NcToken)"units","seconds");

  for (int i = 0; i<ashN; i++) { loc[i]=particle[particle[i].order].size; }
  vp = ncfile.add_var((NcToken)"size", ncDouble, dp);
  vp->put(loc, ashN);
  vp->add_att((NcToken)"units","meters");
  
  for (int i = 0; i<ashN; i++) { loc[i]=particle[particle[i].order].x; }
  vp = ncfile.add_var((NcToken)"lon", ncDouble, dp);
	if ( isRotatedGrid() ) rotateGrid(loc, rotGrid.lon, LON);
  vp->put(loc, ashN);
  vp->add_att((NcToken)"units","degrees_E");
	if ( isRotatedGrid() ) rotateGridPoint(&maxlon, rotGrid.lon, LON);
  vp->add_att((NcToken)"max_value",maxlon);
	if ( isRotatedGrid() ) rotateGridPoint(&minlon, rotGrid.lon, LON);
  vp->add_att((NcToken)"min_value",minlon);
  
  for (int i = 0; i<ashN; i++) { loc[i]=particle[particle[i].order].y; }
  vp = ncfile.add_var((NcToken)"lat", ncDouble, dp);
	if ( isRotatedGrid() ) rotateGrid(loc, rotGrid.lat, LAT);
  vp->add_att((NcToken)"units","degrees_N");
	if ( isRotatedGrid() ) rotateGridPoint(&maxlat, rotGrid.lat, LAT);
  vp->add_att((NcToken)"max_value",maxlat);
	if ( isRotatedGrid() ) rotateGridPoint(&minlat, rotGrid.lat, LAT);
  vp->add_att((NcToken)"min_value",minlat);
  vp->put(loc, ashN);

  for (int i = 0; i<ashN; i++) { loc[i]=particle[particle[i].order].z; }
  vp = ncfile.add_var((NcToken)"hgt", ncDouble, dp);
  vp->add_att((NcToken)"units","meters");
  vp->add_att((NcToken)"max_value",maxhgt);
  vp->add_att((NcToken)"min_value",minhgt);
  vp->put(loc, ashN);

  // added 'grounded' boolean variable
  for (int i = 0; i<ashN; i++) {
    gnd[i]=(ncbyte)particle[particle[i].order].grounded; 
    }
  vp = ncfile.add_var((NcToken)"grounded", ncByte, dp);
  vp->add_att((NcToken)"units","none");
	vp->add_att((NcToken)"total",numGrounded);
  vp->put(gnd, ashN);

  // added 'exists' boolean variable, reusing *gnd array
  for (int i = 0; i<ashN; i++) {
    gnd[i]=(ncbyte)particle[particle[i].order].exists; 
    }
  vp = ncfile.add_var((NcToken)"exists", ncByte, dp);
  vp->add_att((NcToken)"units","none");
	vp->add_att((NcToken)"total",ashN-numOutOfBounds);
  vp->put(gnd, ashN);
  
#ifdef PUFF_STATISTICS
  for (int i = 0; i<ashN; i++) { loc[i]=dif_x[particle[i].order]; }
  vp = ncfile_add_var((NcToken)"dif_x", ncDouble, dp);
  vp->add_att((NcToken)"units","meters");
  vp->add_att((NcToken)"description","net diffusion component");  
  vp->put(loc, ashN);   

  for (int i = 0; i<ashN; i++) { loc[i]=dif_y[particle[i].order]; }
  vp = ncfile_add_var((NcToken)"dif_y", ncDouble, dp);
  vp->add_att((NcToken)"units","meters");
  vp->add_att((NcToken)"description","net diffusion component");    
  vp->put(loc, ashN);   

  for (int i = 0; i<ashN; i++) { loc[i]=dif_z[particle[i].order]; }
  vp = ncfile_add_var((NcToken)"dif_z", ncDouble, dp);
  vp->add_att((NcToken)"units","meters");
  vp->add_att((NcToken)"description","net diffusion component");    
  vp->put(loc, ashN);   

  for (int i = 0; i<ashN; i++) { loc[i]=adv_x[particle[i].order]; }
  vp = ncfile_add_var((NcToken)"adv_x", ncDouble, dp);
  vp->add_att((NcToken)"units","meters");
  vp->add_att((NcToken)"description","net advection component");    
  vp->put(loc, ashN);   

  for (int i = 0; i<ashN; i++) { loc[i]=adv_y[particle[i].order]; }
  vp = ncfile_add_var((NcToken)"adv_y", ncDouble, dp);
  vp->add_att((NcToken)"units","meters");
  vp->add_att((NcToken)"description","net advection component");  
  vp->put(loc, ashN);   

  for (int i = 0; i<ashN; i++) { loc[i]=adv_z[particle[i].order]; }
  vp = ncfile_add_var((NcToken)"adv_z", ncDouble, dp);
  vp->add_att((NcToken)"units","meters");
  vp->add_att((NcToken)"description","net advection component");    
  vp->put(loc, ashN);   
#endif // PUFF_STATISTICS

  // add eruption specifications
  (ncfile.add_var((NcToken)"clock_time", ncInt,0))->put(&clockTime,1);
  (ncfile.add_var((NcToken)"origin_time", ncInt,0))->put(&origTime,1);
  (ncfile.add_var((NcToken)"origin_lon", ncDouble,0))->put(&origLon,1);
  (ncfile.add_var((NcToken)"origin_lat", ncDouble,0))->put(&origLat,1);
  (ncfile.add_var((NcToken)"erupt_hours", ncFloat,0))->put(&erupt_hours,1);
  (ncfile.add_var((NcToken)"plume_height", ncFloat,0))->put(&plume_height,1);
  (ncfile.add_var((NcToken)"plume_width_z", ncFloat,0))->put(&plume_width_z,1);
  (ncfile.add_var((NcToken)"plume_width_h", ncFloat,0))->put(&plume_width_h,1);
  (ncfile.add_var((NcToken)"diffuse_h", ncFloat,0))->put(&diffuse_h,1);
  (ncfile.add_var((NcToken)"diffuse_v", ncFloat,0))->put(&diffuse_v,1);
  (ncfile.add_var((NcToken)"log_mean", ncFloat,0))->put(&log_mean,1);
  (ncfile.add_var((NcToken)"log_sdev", ncFloat,0))->put(&log_sdev,1);
  
  // put in global attributes
  ncfile.add_att((NcToken)"title","Puff Ash Data");
  ncfile.add_att((NcToken)"volcano",origName);
  ncfile.add_att((NcToken)"date_time",date_time);
  ncfile.add_att((NcToken)"plume_shape",plume_shape);
  ncfile.add_att((NcToken)"history",argument.command_line.c_str());
  
  delete[] loc;
  delete[] gnd;
  
  return;
  }
///////////////////////////////////////////////////////////////////////
//
// read netCDF using C++ interface
//
//////////////////////////////////////////////////////////////////////
int Ash::read(char *filename) {
  bool need_allocation = false;  // flag where space needs to be allocated
  NcVar *vp;  // pointer to NcVar object

  // if there is no ash initially, we'll need to allocate space for it later
  if (ashN == 0) need_allocation = true;
    
  // create netCDF file object
  NcFile ncfile(filename, NcFile::ReadOnly);
  if (! ncfile.is_valid() ) {
    std::cerr << "failed to open netCDF file " << filename <<" as a c++ object\n";
    return ASH_ERROR;
    }

  // allocate space if ash object is empty
  if (need_allocation) 
	{ 
    ashN = (ncfile.get_dim((NcToken)"nash"))->size();
  	allocate(); 
	}
    
  // get eruption specifications
  vp = ncfile.get_var((NcToken)"clock_time");
  vp->get(&clockTime,vp->edges());
  vp = ncfile.get_var((NcToken)"origin_time");
  vp->get(&origTime,vp->edges());
  vp = ncfile.get_var((NcToken)"origin_lon");
  vp->get(&origLon,vp->edges());
  vp = ncfile.get_var((NcToken)"origin_lat");
  vp->get(&origLat,vp->edges());
  // read simulation parameters
  vp = ncfile.get_var((NcToken)"erupt_hours");
  vp->get(&erupt_hours,vp->edges());
  vp = ncfile.get_var((NcToken)"plume_height");
  vp->get(&plume_height,vp->edges());
  vp = ncfile.get_var((NcToken)"plume_width_z");
  vp->get(&plume_width_z,vp->edges());
  vp = ncfile.get_var((NcToken)"plume_width_h");
  vp->get(&plume_width_h,vp->edges());
  vp = ncfile.get_var((NcToken)"diffuse_h");
  vp->get(&diffuse_h,vp->edges());
  vp = ncfile.get_var((NcToken)"diffuse_v");
  vp->get(&diffuse_v,vp->edges());
  vp = ncfile.get_var((NcToken)"log_mean");
  vp->get(&log_mean,vp->edges());
  vp = ncfile.get_var((NcToken)"log_sdev");
  vp->get(&log_sdev,vp->edges());

  // get ash location
  // we need a temporary array 'loc' to retrieve particle attributes because
  // they are stored in the an object and cannot be directly accessed as an
  // array

  double *loc = new double[ashN];
  
  vp = ncfile.get_var((NcToken)"lon");
  vp->get(loc,vp->edges());
  for (int i = 0; i<ashN; i++) { particle[i].x=loc[i]; }
  vp = ncfile.get_var((NcToken)"lat");
  vp->get(loc,vp->edges());
  for (int i = 0; i<ashN; i++) { particle[i].y=loc[i]; }
  vp = ncfile.get_var((NcToken)"hgt");
  vp->get(loc,vp->edges());
  for (int i = 0; i<ashN; i++) { particle[i].z=loc[i]; }
  vp = ncfile.get_var((NcToken)"size");
  vp->get(loc,vp->edges());
  for (int i = 0; i<ashN; i++) { particle[i].size=loc[i]; }
  vp = ncfile.get_var((NcToken)"age");
  vp->get(loc,vp->edges());
  for (int i = 0; i<ashN; i++) { particle[i].startTime=loc[i]; }
  
	// done with this array
	if (loc) delete[] loc;

  // get global attributes
  strcpy(origName,(ncfile.get_att((NcToken)"volcano"))->as_string(0) );
  strcpy(plume_shape,(ncfile.get_att((NcToken)"plume_shape"))->as_string(0) );
  strcpy(date_time,(ncfile.get_att((NcToken)"date_time"))->as_string(0) );
//  strncpy(date_time,(ncfile.get_att((NcToken)"date_time"))->as_string(0), 12 );
//	date_time[12] = '\0';
  
   return ASH_OK; 
  }
////////////////////////////////////////////////////////////////////////
//
// Determine if filename is a puff-generated ash file
//
////////////////////////////////////////////////////////////////////////
int Ash::isAshFile(char *filename) {

  int cdfid, nashid;
  size_t nashsize;  

	// if there is no restart file, avoid segfault
	if (!filename) return 0;

  // determine if file exists and is readable
  if (access(filename, R_OK) != 0) return 0;
  // determine if file is netCDF
  if ( (nc_open(filename, NC_NOWRITE, &cdfid)) != NC_NOERR) return 0;
  // get dimension ID
  if (nc_inq_dimid(cdfid, "nash", &nashid) != NC_NOERR) {
    std::cerr << "file " << filename << "does not contain a \"nash\" dimension\n";
    return 0;
    }
  // get dimension length
  if (nc_inq_dimlen(cdfid, nashid, &nashsize) != NC_NOERR) {
    std::cerr << "failed to obtain a length for dimension \"nash\" in file " << filename << std::endl;
    return 0;
    }
      
  return (int)nashsize;
  
  }
////////////////////////////////////////////////////////////////////////
//
// sort particles
// the array 'order' is the sorted ordering, since there are many attributes
// for each particle and the individual particles are not objects themselves,
// it is easiest and most efficient to change 'order' when sorting.
////////////////////////////////////////////////////////////////////////
void Ash::quicksort() {

#define SWAP(a,b) temp=(a); (a)=(b); (b)=temp;
#define ISWAP(a,b) itmp=(a); (a)=(b); (b)=itmp;

  long M;
  int NSTACK;
  long i, ir, j,k,l,*istack;
  int jstack=0, o, itmp;
  float a,temp;
  double *arr;

//  loc = ashPoint;
  ir = ashN-1;
  l = 0;
  
  M=(long)( ashN > 100 ? (ashN/20) : ashN);
  NSTACK=( ashN > 20 ? ashN : 20 );
  // allocate space for temporary array that is being sorted
  arr = new double[ashN];
  
  for (i=0; i<ashN; i++) 
  {
		switch ( sorting_variable)
		{
			case ASH_SORT_T:
				arr[i] = particle[i].startTime;  break;
			case ASH_SORT_X:
				arr[i] = particle[i].x;  break;
			case ASH_SORT_Y:
				arr[i] = particle[i].y;  break;
			case ASH_SORT_Z:
			default:
				arr[i] = particle[i].z;  break;
		}

    particle[i].order = i;
  }
  if (sorting_protocol == ASH_SORT_NEVER) return;
  istack= new long[NSTACK];
  for(;;) {
    if (ir-l < M) {
      for (j=l+1; j<=ir;j++) {
        a=arr[j];
        o=particle[j].order;
	for (i=j-1; i>=l; i--) {
	  if (arr[i] <= a) break;
	  arr[i+1]=arr[i];
	  particle[i+1].order=particle[i].order;
	  }
	  arr[i+1]=a;
	  particle[i+1].order=o;
	}
	if (jstack == 0) break;
	ir=istack[jstack--];
	l=istack[jstack--];
	}
      else {
        k=(l+ir) >> 1;
	SWAP(arr[k],arr[l+1])
	ISWAP(particle[k].order,particle[l+1].order)
	if (arr[l] > arr[ir]) {
	  SWAP(arr[l],arr[ir])
	  ISWAP(particle[l].order,particle[ir].order)
	  }
	if (arr[l+1] > arr[ir]) {
	  SWAP(arr[l+1],arr[ir])
	  ISWAP(particle[l+1].order,particle[ir].order)
	  }
	if (arr[l] > arr[l]+1) {
	  SWAP(arr[l],arr[l+1])
	  ISWAP(particle[l].order,particle[l+1].order)
	  }
	i=l+1;
	j=ir;
	a=arr[l+1];
	o=particle[l+1].order;
	for (;;) {
	  do i++; while (arr[i] < a);
	  do j--; while (arr[j] > a);
	  if (j < i) break;
	  SWAP (arr[i],arr[j]);
	  ISWAP (particle[i].order,particle[j].order);
	  }
	arr[l+1]=arr[j];
	particle[l+1].order=particle[j].order;
	arr[j]=a;
	particle[j].order=o;
	jstack += 2;
	
	if (jstack > NSTACK) { 
	  std::cout << "NSTACK too small in sort.\n";
	  exit(1);
	  }
	if (ir-i+1 >= j-1) {
	  istack[jstack]=ir;
	  istack[jstack-1]=i;
	  ir=j-1;
	}  else {
	  istack[jstack]=j-1;
	  istack[jstack-1]=l;
	  l=i;
	  }
	}
      }
    delete[] istack;
    delete[] arr;
    
    return;
    }

////////////////////////////////////////////////////////////////////////
//
// find lat/lon limits
//
////////////////////////////////////////////////////////////////////////
void Ash::findLimits () 
{
  std::vector<double> lon, lat, hgt;
  
  for (int i=0; i<ashN; i++) {
    lon.push_back(particle[i].x);
    lat.push_back(particle[i].y);
    hgt.push_back(particle[i].z);
    }
  // find max/min lon values
  maxlon=*(max_element(lon.begin(),lon.end()));
  minlon=*min_element(lon.begin(),lon.end());
  // if ash is near meridian, min/max is confusing.  Puff keeps all 'lon'
  // values in the range 0 <= lon <= 360.
  // So, if ash falls within +/- 10 degrees of the meridian, assume it
  // crosses it and use negative lon values for the minimum
  if (maxlon > 350 && minlon < 10) 
  {
    for(std::vector<double>::iterator p = lon.begin();
        p != lon.end(); p++) {
      if (*p > 180) *p = (*p) - 360;
      }
   // now redo the min/max lon values
    maxlon=*(max_element(lon.begin(),lon.end()) );
    minlon=*(min_element(lon.begin(),lon.end()) );
  }

  // find max/min lat values
  maxlat=*max_element(lat.begin(),lat.end());
  minlat=*min_element(lat.begin(),lat.end());
  // find min/max height
  maxhgt=*max_element(hgt.begin(),hgt.end());
  minhgt=*min_element(hgt.begin(),hgt.end());
  return;
}
////////////////////////////////////////////////////////////////////////
//
// clear statistical information
//
////////////////////////////////////////////////////////////////////////
#ifdef PUFF_STATISTICS
void Ash::clearStats() {
  int i;
  for (i=0; i<ashN; i++) {
    dif_x[i] = dif_y[i] = dif_z[i] = 0;
    adv_x[i] = adv_y[i] = adv_z[i] = 0;
    }
  return;
  }
#endif
////////////////////////////////////////////////////////////////////////
// it would probably be more efficient to do this when the options are 
// first parsed so that argument.sorted is not carrying around a char string
void Ash::setSortingProtocol(char* arg) 
{
	std::string prot, var;
	prot = arg;
	std::string::size_type loc = prot.find(":");
	if (loc != std::string::npos)
	{
		var = prot.substr(loc+1);
		prot.erase(loc);
  } else {
		var = "z";
	}

  if (prot == "yes" || prot == "YES" ) {
    sorting_protocol = ASH_SORT_YES;
  } else if ( prot == "no" || prot == "NO" ) {
	        sorting_protocol = ASH_SORT_NO;
  } else if ( prot == "never" || prot == "NEVER" ) {
	       sorting_protocol = ASH_SORT_NEVER;
	} else {
		std::cerr << "WARNING: unknown sorting protocol '" << prot
			        << "'. Defaulting to 'yes'";
		sorting_protocol = ASH_SORT_YES;
	}

	if (var == "age" || var == "t" || var == "time") sorting_variable = ASH_SORT_T;
	else if (var == "x" || var == "lon") sorting_variable = ASH_SORT_X;
	else if (var == "y" || var == "lat") sorting_variable = ASH_SORT_Y;
	else if (var == "z" || var == "hgt" || var == "height") sorting_variable = ASH_SORT_Z;
	else sorting_variable = ASH_SORT_Z;
  return;
}
////////////////////////////////////////////////////////////////////////
int Ash::ground(long idx)
{
  particle[idx].grounded = true;
	if (!particle[idx].exists) {
		std::cout << "grounding non-existant particle\n";
		}
  numGrounded++;
  if ( (numOutOfBounds + numGrounded) >= ashN)
  {
    std::cerr << "\nAll ash particles are either grounded or outside the bounds of wind data.\nGrounded: " << numGrounded << "\nOut of Bounds: " << numOutOfBounds << std::endl;
    return 1;
  } else {  
    return 0;
  }
}
  
////////////////////////////////////////////////////////////////////////
// mark this particle out-of-bounds, increment the global counter, and 
// return a flag if this was the last particle left moving
////////////////////////////////////////////////////////////////////////
int Ash::outOfBounds(int idx)
{
  particle[idx].exists = false;
	if (particle[idx].grounded) {
		std::cout << "particle already grounded\n";
		}
  numOutOfBounds++;
  if ( (numOutOfBounds + numGrounded) >= ashN)
  {
    std::cerr << "\nAll ash particles are either grounded our outside the bounds of wind data.\nGrounded: " << numGrounded << "\nOut of Bounds: " << numOutOfBounds << std::endl;
    return 1;
  } else {  
    return 0;
  }
}
////////////////////////////////////////////////////////////////////////
void Ash::clearStash()
{
  recParticle.clear();
  recAshN=0;
  recTime.clear();
  return;
}

////////////////////////////////////////////////////////////////////////
// stash the data for this time in a record.
////////////////////////////////////////////////////////////////////////
void Ash::stashData(time_t now)
{
  
  for (int i=0;i<ashN;i++)
  {
		// only stash particles that exists (not out of bounds)
		// and have been "born"
		if (particle[i].exists and particle[i].startTime <= now)
		{
    	recParticle.push_back(particle[i]);
		}
  }
  // advance the record counter
  recAshN++;
  // store the time of this data
  recTime.push_back(clockTime);
  return;
}
    
////////////////////////////////////////////////////////////////////////
// write a single file of gridded data.  Things get a little confusing when
// looping over all the particles and populating the concentration grids because
// there are both 2D and 3D grids, depending on whether it is airborne particles
// or fallout.  Thus, there are lots of 'if particle is grounded' statements
// littered about.  The reason it is not broken into two sections instead, is
// that the concentration calculations are not really set in stone, and I didn't
// want to have two versions floating around to keep up-to-date.
// Both absolute and relative concentrations are written.
// The structure 'cc' is a ;concentration cloud' and hold the necessary info
// to describe the cloud.  It is also used by the 'Planes' class.
////////////////////////////////////////////////////////////////////////
void Ash::writeGriddedData(
  std::string filename,	// where the output will be written
  bool last_in_running_average	// if this is a repeat run, is this last?
  				// true for non-repeat runs, 'cause it is last!
  )
{
  // a static boolean indicating whether we are calculating absolute
  // concentrations.  Large grid spaces end up with excessively large
  // values, so this can get turned off.
  static bool write_abs_conc = true;
    
  // say that we're creating/writing this file
  if (last_in_running_average)
  {
    std::cout << "Writing concentration file \"" << filename << "\" ... " <<
    std::flush;
  }
  
  float dHorz = 1,      // grid horizontal size
        dVert = 2000;  // grid vertical size

  // parse the options ( format checked earlier in puff_options.C )
  char force = '\000';	// don't be smart and check dHorz vs. dVert sizes
  if ( argument.gridOutputOptions) 
    sscanf(argument.gridOutputOptions,"%fx%f%c",&dHorz, &dVert, &force);

  // this dHorz and dVert could be backwards, so swap them (and warn) if we
  // think so.  dHorz is degrees and dVert is meters.  User can override this
  // by appending a '!', which 'force' checks for
  if ((dHorz > dVert) && !(force)) 
  {
    std::cerr << "\nWARNING: swapping -gridOutput options to DX=" << dVert <<
      " and DY=" << dHorz << ".  Use -gridOutput=DXxDY! to override\n";
    // swap values
    float t = dHorz; dHorz = dVert; dVert = t;
  }
    
  // do not calculate absolute concentrations for large grid spaces since
  // the volume will exceed our use of 'double'.
  if (dHorz > 0.5)
  {
    // only warn once by making 'write_abs_conc' static
    if (write_abs_conc) std::cerr << "\nWARNING: not calculating absolute concentration due to excessively large grid cell size.  Use -gridOutput with smaller values.\n";
    write_abs_conc = false;
  }
  
  // find the limits
  float minX = (*min_element(recParticle.begin(), recParticle.end(), cmpX) ).x;
  float maxX = (*max_element(recParticle.begin(), recParticle.end(), cmpX) ).x;
  float minY = (*min_element(recParticle.begin(), recParticle.end(), cmpY) ).y;
  float maxY = (*max_element(recParticle.begin(), recParticle.end(), cmpY) ).y;
  float minZ = (*min_element(recParticle.begin(), recParticle.end(), cmpZ) ).z;
  float maxZ = (*max_element(recParticle.begin(), recParticle.end(), cmpZ) ).z;
  
  // if ash is near meridian, min/max is confusing.  Puff keeps all 'lon'
  // values in the range 0 <= lon <= 360.
  // So, if ash falls within +/- 10 degrees of the meridian, assume it
  // crosses it and use negative lon values for the minimum
  if (maxX > 350 && minX < 10) 
  {
    for(std::vector<Particle>::iterator p = recParticle.begin();
        p != recParticle.end(); 
	p++) 
    {
      if ((*p).x > 180) (*p).x = (*p).x - 360;
    }
    // now redo the min/max lon values
    minX = (*min_element(recParticle.begin(), recParticle.end(), cmpX) ).x;
    maxX = (*max_element(recParticle.begin(), recParticle.end(), cmpX) ).x;
  }

  // if a gridBox was specifed, re-adjust to that.  If not, set gridBox so
  // the next time (if this is a repeat run) it will be used as well
  if ( argument.gridBox )
  {
    sscanf(argument.gridBox, "%f:%f/%f:%f/%f:%f", &minX, &maxX, &minY, &maxY, &minZ, &maxZ);
  } else {
		// allocate space, hopefully enough
		argument.gridBox = (char*)calloc(255,sizeof(char));
    snprintf(argument.gridBox, 255, "%f:%f/%f:%f/%f:%f", minX, maxX, minY, maxY, minZ, maxZ);
  }
  
  // round with dHorz and dVert
  minX = (floorf(minX/dHorz))*dHorz;
  maxX = (ceilf(maxX/dHorz))*dHorz;
  minY = (floorf(minY/dHorz))*dHorz;
  maxY = (ceilf(maxY/dHorz))*dHorz;
  minZ = (floorf(minZ/dVert))*dVert;
  maxZ = (ceilf(maxZ/dVert))*dVert;
  
  // set the size of the grids, both 2D and 3D
  cc.xSize = (int)rint( ((maxX-minX)/dHorz) );
  cc.ySize = (int)rint( ((maxY-minY)/dHorz) );
  cc.zSize = (int)rint( ((maxZ-minZ)/dVert) );
  cc.tSize = (int)recTime.size();
  cc.d3size = cc.tSize*cc.xSize*cc.ySize*cc.zSize;
  cc.d2size = cc.tSize*cc.xSize*cc.ySize;
			  
  // create concentration arrays, 2D for fallout and 3D for airborne data
  // airborne data
  float *rel_air_conc = new float[cc.d3size];
  float *abs_air_conc = new float[cc.d3size];
  // fallout data
  float *rel_fo_conc = new float[cc.d2size];
  float *abs_fo_conc = new float[cc.d2size];
  // average particle size array
  float *abs_air_size = new float[cc.d3size];
  float *abs_fo_size = new float[cc.d2size];
  // create max values
  float max_abs_air_conc = 0;
  float max_abs_fo_conc = 0;
  float max_rel_air_conc = 0;
  float max_rel_fo_conc = 0;
  
  // zero these values
  for (int i = 0; i < cc.d3size; i++) rel_air_conc[i] = abs_air_conc[i] = abs_air_size[i] = 0.0;
  
  for (int i = 0; i < cc.d2size; i++) rel_fo_conc[i] = abs_fo_conc[i] = abs_fo_size[i] = 0.0;
    
  // create index values for convenient referencing
  int cIdx, tIdx, xIdx, yIdx, zIdx;
  
  // populate the concentration grid
  for (unsigned int pIdx = 0; 
       pIdx < recParticle.size(); 
       pIdx++)
  {
      // rint() rounds to the nearest integer, return a double, so typecast to 
      // an int.  It is defined in the cmath header
      xIdx = (int)floor((recParticle[pIdx].x-minX)/dHorz);
      yIdx = (int)floor((recParticle[pIdx].y-minY)/dHorz);
      zIdx = (int)floor((recParticle[pIdx].z-minZ)/dVert);
      // size of recParticle is nAsh * cc.tSize, so we can get tIdx by taking the
      // floor value of the 'i' index.  Typecasting as an int would probably
      // be sufficient, but why count on it?
      tIdx = (int)(floor(pIdx/ashN));
      // 2D grids for fallout, 3D grids for airborne
      if (recParticle[pIdx].grounded)
        cIdx = xIdx + yIdx*cc.xSize + tIdx*cc.xSize*cc.ySize;
      else
        cIdx = xIdx + yIdx*cc.xSize + zIdx*cc.xSize*cc.ySize + tIdx*cc.xSize*cc.ySize*cc.zSize;
	
      // sanity check that cIdx is valid	  
      if (xIdx >= 0 && xIdx < cc.xSize &&
          yIdx >= 0 && yIdx < cc.ySize &&
  	  zIdx >= 0 && zIdx < cc.zSize &&
          cIdx >= 0 && cIdx < cc.d3size &&
	  (!recParticle[pIdx].grounded || cIdx < cc.d2size)
	  )
      {
        // populate relative concentration indexes
	if (recParticle[pIdx].grounded) 
	{
		rel_fo_conc[cIdx]++;
		if (rel_fo_conc[cIdx] > max_rel_fo_conc)
			{ max_rel_fo_conc = rel_fo_conc[cIdx]; }
	} else {
		rel_air_conc[cIdx]++;
		if (rel_air_conc[cIdx] > max_rel_air_conc)
			{ max_rel_air_conc = rel_air_conc[cIdx]; }
	}
	
        // weighted average of the particle size for both fallout and airborne
	if (recParticle[pIdx].grounded)
          abs_fo_size[cIdx] = (1/rel_fo_conc[cIdx])*recParticle[pIdx].size + 
              ((rel_fo_conc[cIdx]-1)/rel_fo_conc[cIdx])*abs_fo_size[cIdx];
	else abs_air_size[cIdx] = (1/rel_air_conc[cIdx])*recParticle[pIdx].size
	 + ((rel_air_conc[cIdx]-1)/rel_air_conc[cIdx])*abs_air_size[cIdx];	      
        // absolute concentration
	if (write_abs_conc)
	{
          // get the average latitude of this grid space
          double av_lat = minY + dHorz*((float)yIdx+0.5);
      
          // get the approximate volume of this grid space as a cube, but use
          // the average latitude since high latitude grids are trapazoidal
         // neglect the effect of elevation and use the earth radius
          double vol = dVert * dlat2meter(dHorz) * dlon2meter(dHorz, av_lat);
      
          // concentration is mass per volume. The mass fraction was calculated
          // when size was initialized during make_ash().  See that for
          // specifics but currently spherical particles were assumed. 
	  // argument.eruptMass is in kilograms
          double mass = recParticle[pIdx].mass_fraction * argument.eruptMass;
      
          // convert to milligrams because we'll use milligrams/m^3 as our
          // concentration unit.
          mass = mass * 1e3;
      
          // assign the absolute concentration
          if (recParticle[pIdx].grounded) abs_fo_conc[cIdx] += mass/vol;
	  else abs_air_conc[cIdx] += mass/vol;
	  
	  // adjust maximum value for airborne particles if necessary
          if (!recParticle[pIdx].grounded && 
	      abs_air_conc[cIdx] > max_abs_air_conc) 
	       { max_abs_air_conc = abs_air_conc[cIdx]; }
	       
	  // adjust maximum value for fallout particles if necessary
          if (recParticle[pIdx].grounded && 
	       abs_fo_conc[cIdx] > max_abs_fo_conc) 
	         { max_abs_fo_conc = abs_fo_conc[cIdx]; }
	
          // sanity check for airborne or fallout particles
	  bool invalid_cIdx = false;
          if (!recParticle[pIdx].grounded && abs_air_conc[cIdx] < 0)
	    invalid_cIdx = true;
          if (recParticle[pIdx].grounded && abs_fo_conc[cIdx] < 0)
	    invalid_cIdx = true;
	  if (invalid_cIdx)
          {
            std::cerr << "ERROR: bad absolute concentration value\n";
	    exit(ASH_ERROR);
          } 
	  
        } // end if writing abs_conc
      } // end if valid index number
      
  } // end loop over all members of recParticle vector
  
  // if -gridLevels were specified, rebin to reflect that
  if (argument.gridLevels > 0)
  {
    // normalize by exp(gridLevels).  Then take the log of each value to 
    // determine its relative concentration value
    
    // find the min/max
//    float conc_min = -1;
//    float conc_max = -1;
//    for (int i=0; i < cc.d3size; i++)
//    {
//      if (rel_air_conc[i] > conc_max) conc_max = rel_air_conc[i];
//      if (rel_air_conc[i] < conc_min) conc_min = rel_air_conc[i];
//    }
    
    // normalize by exp(gridLevels)
    for (int i=0; i < cc.d3size; i++)
    {
      rel_air_conc[i] = rel_air_conc[i]/max_rel_air_conc*exp(argument.gridLevels);
      // assign a level between zero and gridLevels
      if (rel_air_conc[i] >= 1)
      {
        rel_air_conc[i] = logf(rel_air_conc[i]);
      } else {
        rel_air_conc[i] = 0;
      }
    }  // end loop over entire 3D grid

  // now do the same for relative fallout concentration
    // find the min/max
//    conc_min = -1;
//    conc_max = -1;
//    for (int i = 0; i < cc.d2size; i++)
//    {
//      if (rel_fo_conc[i] > conc_max) conc_max = rel_fo_conc[i];
//      if (rel_fo_conc[i] < conc_min) conc_min = rel_fo_conc[i];
//    }
    
    // normalize by exp(gridLevels)
    for (int i=0; i < cc.d2size; i++)
    {
      rel_fo_conc[i] = rel_fo_conc[i]/max_rel_fo_conc*exp(argument.gridLevels);
      // assign a level between zero and gridLevels
      if (rel_fo_conc[i] >= 1)
      {
        rel_fo_conc[i] = logf(rel_fo_conc[i]);
      } else {
        rel_fo_conc[i] = 0;
      }
    }  // end loop over entire 3D grid
	// adjust maximum values
	max_rel_air_conc = argument.gridLevels;
	max_rel_fo_conc = argument.gridLevels;
  
  } // end if -gridLevels was specified
    
  averageGriddedData(abs_air_conc,
                     rel_air_conc, 
		     abs_fo_conc,
		     rel_fo_conc);
  
  if (last_in_running_average)
  {
  
  // create/clobber a netCDF file
  NcFile ncfile(filename.c_str(), NcFile::Replace);
  
  // create dimension objects
  NcDim *d_time = ncfile.add_dim((NcToken)"time"); // record dimension
  NcDim *d_lon = ncfile.add_dim((NcToken)"lon",cc.xSize);
  NcDim *d_lat = ncfile.add_dim((NcToken)"lat",cc.ySize);
  NcDim *d_lev = ncfile.add_dim((NcToken)"level",cc.zSize);
  
  // make arrays for the dimensions that will be written to the netCDF file
  cc.xValues = new float[cc.xSize];
  cc.yValues = new float[cc.ySize];
  cc.zValues = new float[cc.zSize];
  cc.tValues = new long int[cc.tSize];
  
  // populate the arrays with regular grid values
  cc.xValues[0] = minX;
  for (int i = 1; i < cc.xSize; i++) cc.xValues[i]=cc.xValues[i-1]+dHorz;
  cc.yValues[0] = minY;
  for (int i = 1; i < cc.ySize; i++) cc.yValues[i]=cc.yValues[i-1]+dHorz;
  cc.zValues[0] = minZ;
  for (int i = 1; i < cc.zSize; i++) cc.zValues[i]=cc.zValues[i-1]+dVert;
  for (int i = 0; i < cc.tSize; i++)
         cc.tValues[i]=(long int)recTime[i];
    
  // a netCDF variable object, everything will use it
  NcVar *vp;
  
  // a record index for writing one record at a time
  int recIdx = 0;
  
  // define the dimensions as variables also
  vp = ncfile.add_var((NcToken)"time", ncInt, d_time);
  // time values must be added one record at a time because the dimension
  // slice addition does not appear to work (undefined reference error)
  // vp->put_rec(d_time, cc.tValues);
  recIdx = 0;
  for (int i = 0; i < cc.tSize; i++)
  {
    vp->put_rec(&cc.tValues[recIdx], i);
    // advance the pointer, this may cause a (small) memory leak when deleting
    recIdx++;
  }
  vp->add_att((NcToken)"units","seconds since 1970-1-1");
  vp->add_att((NcToken)"long_name","time");
  vp = ncfile.add_var((NcToken)"level", ncFloat, d_lev);
	long int *edges = vp->edges(); // be sure to delete every time
  vp->put(cc.zValues,edges);
  vp->add_att((NcToken)"units","meters");
  vp->add_att((NcToken)"long_name","level");
  vp->add_att((NcToken)"min_value",cc.zValues[0]);
  vp->add_att((NcToken)"max_value",cc.zValues[cc.zSize-1]);
  vp = ncfile.add_var((NcToken)"lat", ncFloat, d_lat);
	delete[] edges; edges = vp->edges();
  vp->put(cc.yValues,edges);
  vp->add_att((NcToken)"units","degrees_north");
  vp->add_att((NcToken)"long_name","latitude");
  vp->add_att((NcToken)"min_value",cc.yValues[0]);
  vp->add_att((NcToken)"max_value",cc.yValues[cc.ySize-1]);
  vp = ncfile.add_var((NcToken)"lon", ncFloat, d_lon);
	delete[] edges; edges = vp->edges();
  vp->put(cc.xValues,edges);
  vp->add_att((NcToken)"units","degrees_east");
  vp->add_att((NcToken)"long_name","longitude");
  vp->add_att((NcToken)"min_value",cc.xValues[0]);
  vp->add_att((NcToken)"max_value",cc.xValues[cc.xSize-1]);

	delete[] edges;

  // add the relative airborne concentration data
  vp = ncfile.add_var((NcToken)"rel_air_conc", ncFloat, d_time, d_lev, d_lat, d_lon);
  // add one record at a time
  recIdx = 0;
  for (int i = 0; i < cc.tSize; i++)
  {
    vp->put_rec(&cc.rel_air_conc_avg[recIdx], i);
    recIdx += cc.xSize*cc.ySize*cc.zSize;
  }
  vp->add_att((NcToken)"units","none");
  vp->add_att((NcToken)"long_name","relative airborne concentration");
  vp->add_att((NcToken)"max_value",max_rel_air_conc);
//  vp->add_att((NcToken)"missing_value",0.f);

  // add the relative fallout concentration data
  vp = ncfile.add_var((NcToken)"rel_fallout_conc", ncFloat, d_time, d_lat, d_lon);
  // add one record at a time
  recIdx = 0;
  for (int i = 0; i < cc.tSize; i++)
  {
    vp->put_rec(&cc.rel_fo_conc_avg[recIdx], i);
    recIdx += cc.xSize*cc.ySize;
  }
  vp->add_att((NcToken)"units","none");
  vp->add_att((NcToken)"long_name","relative fallout concentration");
  vp->add_att((NcToken)"max_value",max_rel_fo_conc);
  vp->add_att((NcToken)"missing_value",0.f);

  // add the absolute airborne concentration data
  vp = ncfile.add_var((NcToken)"abs_air_conc", ncFloat, d_time, d_lev, d_lat, d_lon);
  // add one record at a time
  recIdx = 0;
  for (int i = 0; i < cc.tSize; i++)
  {
    vp->put_rec(&cc.abs_air_conc_avg[recIdx], i);
     recIdx += cc.xSize*cc.ySize*cc.zSize;
  }
  vp->add_att((NcToken)"units","milligrams/m^3");
  vp->add_att((NcToken)"long_name","absolute airborne concentration");
  vp->add_att((NcToken)"max_value",max_abs_air_conc);
//  vp->add_att((NcToken)"missing_value",0.f);

  // add the absolute fallout concentration data
  vp = ncfile.add_var((NcToken)"abs_fallout_conc", ncFloat, d_time, d_lat, d_lon);
  // add one record at a time
  recIdx = 0;
  for (int i = 0; i < cc.tSize; i++)
  {
    vp->put_rec(&cc.abs_fo_conc_avg[recIdx], i);
    recIdx += cc.xSize*cc.ySize;
  }
  vp->add_att((NcToken)"units","milligrams/m^2");
  vp->add_att((NcToken)"long_name","absolute fallout concentration");
  vp->add_att((NcToken)"max_value",max_abs_fo_conc);
  vp->add_att((NcToken)"missing_value",0.f);

  // add the average size data
  // microns are easier to deal with
  for (int i = 0; i < cc.d3size; i++) abs_air_size[i] = abs_air_size[i]*1e6;
  for (int i = 0; i < cc.d2size; i++) abs_fo_size[i] = abs_fo_size[i]*1e6;
  
  vp = ncfile.add_var((NcToken)"air_size", ncFloat, d_time, d_lev, d_lat, d_lon);
  // add one record at a time
  recIdx = 0;
  for (int i = 0; i < cc.tSize; i++)
  {
    vp->put_rec(&abs_air_size[recIdx], i);
    recIdx += cc.xSize*cc.ySize*cc.zSize;
  }
  vp->add_att((NcToken)"units","microns");
  vp->add_att((NcToken)"long_name","average airborne particle diameter");
  vp->add_att((NcToken)"missing_value",0.f);

  vp = ncfile.add_var((NcToken)"fallout_size", ncFloat, d_time, d_lat, d_lon);
  // add one record at a time
  recIdx = 0;
  for (int i = 0; i < cc.tSize; i++)
  {
    vp->put_rec(&abs_fo_size[recIdx], i);
    recIdx += cc.xSize*cc.ySize;
  }
  vp->add_att((NcToken)"units","microns");
  vp->add_att((NcToken)"long_name","average fallout particle diameter");
  vp->add_att((NcToken)"missing_value",0.f);

  
  // add global attributes here
  ncfile.add_att((NcToken)"title","Puff-generated Ash Data on a Regular Grid");
  ncfile.add_att((NcToken)"history",argument.command_line.c_str());
	ncfile.add_att((NcToken)"volcano",origName);
  
  // add done
  std::cout << "done.\n" << std::flush;

  Planes p(argument.planesFile);
  if (p.size() > 0) p.calculateExposure(&cc);
  
  delete[] cc.xValues;
  delete[] cc.yValues;
  delete[] cc.zValues;
  delete[] cc.tValues;
  
  } // end if last_in_running_average
  
   delete[] rel_air_conc;
   delete[] abs_air_conc;
   delete[] rel_fo_conc;
   delete[] abs_fo_conc;
   delete[] abs_air_size;
   delete[] abs_fo_size;
  
  
  return;
}
//  end of Ash::writeGriddedData()
////////////////////////////////////////////////////////////////////////
// when multiple runs are done '-repeat', average the concentration grids.
// Each variable has a corresponding <var>_avg that is incrementally 
// averaged on each turn.  When there is only one run, the average values are
// the same as the input ones. 
////////////////////////////////////////////////////////////////////////
void Ash::averageGriddedData (
  float *abs_air_conc,
  float *rel_air_conc, 
  float *abs_fo_conc,
  float *rel_fo_conc
  )
{
  static bool init_avg_data = true;
  
  // used to weight the existing average values 
  static int wgt = 0;
  
  // allocate space (and zero) if necessary
  if (init_avg_data)
  {
    cc.abs_air_conc_avg = new float[cc.d3size];
    cc.rel_air_conc_avg = new float[cc.d3size];
    cc.abs_fo_conc_avg  = new float[cc.d2size];
    cc.rel_fo_conc_avg  = new float[cc.d2size];
    for (int i = 0; i < cc.d2size; i++) 
    {
      cc.abs_fo_conc_avg[i] = 0.0;
      cc.rel_fo_conc_avg[i] = 0.0;
    }
    for (int i = 0; i < cc.d3size; i++)
    {
      cc.abs_air_conc_avg[i] = 0.0;
      cc.rel_air_conc_avg[i] = 0.0;
    }
    init_avg_data = false;
  }

 // average the 3D grids  
  for (int i = 0; i < cc.d3size; i++)
  {
    // weighted average
    cc.rel_air_conc_avg[i] = (float)wgt/((float)wgt+1)*cc.rel_air_conc_avg[i] + 
      1/((float)wgt+1)*rel_air_conc[i];
    cc.abs_air_conc_avg[i] = (float)wgt/((float)wgt+1)*cc.abs_air_conc_avg[i] + 
      1/((float)wgt+1)*abs_air_conc[i];
  }

  // average the 2D grids  
  for (int i = 0; i < cc.d2size; i++)
  {
    cc.rel_fo_conc_avg[i] = (float)wgt/((float)wgt+1)*cc.rel_fo_conc_avg[i] + 
      1/((float)wgt+1)*rel_fo_conc[i];
    cc.abs_fo_conc_avg[i] = (float)wgt/((float)wgt+1)*cc.abs_fo_conc_avg[i] + 
      1/((float)wgt+1)*abs_fo_conc[i];
  }
  
  // increment the weighting factor
  wgt++;
  return;

}  
////////////////////////////////////////////////////////////////////////
// compare X
bool cmpX(Particle a, Particle b)
{
  if (a.x < b.x) return true;
  return false;
}
////////////////////////////////////////////////////////////////////////
// compare Y
bool cmpY(Particle a, Particle b)
{
  if (a.y < b.y) return true;
  return false;
}
////////////////////////////////////////////////////////////////////////
// compare Z
bool cmpZ(Particle a, Particle b)
{
  if (a.z < b.z) return true;
  return false;
}
/////////////////////////////////////////////////////////////////////////
// convert an arc of degrees latitude to meters, assuming constant longitude
/////////////////////////////////////////////////////////////////////////
double dlat2meter (double dlat)
{
  double Re = PUFF_EARTH_RADIUS;
  double dy = 2*M_PI*Re*dlat/360.0;
  return dy;
}
/////////////////////////////////////////////////////////////////////////
// convert an arc of degrees longitude to meters
/////////////////////////////////////////////////////////////////////////
double dlon2meter (double dlon, double lat)
{
  double Re = PUFF_EARTH_RADIUS;
  double dx = 2*M_PI*Re*dlon/360.0*cos(M_PI/180.0*lat);
  return dx;
}
/////////////////////////////////////////////////////////////////////////
void Ash::copyRotatedGrid(struct GridRotation *r)
{
	rotGrid.lat = r->lat;
	rotGrid.lon = r->lon;
	rotGrid.angle = r->angle;
	return;
}
/////////////////////////////////////////////////////////////////////////
bool Ash::isRotatedGrid()
{
	if ((rotGrid.lat != 0) ||
			(rotGrid.lon != 0) ||
			(rotGrid.angle != 0) ) return true;
	return false;
}
/////////////////////////////////////////////////////////////////////////
//  move the particles to the rotated grid location.  We assume that 
//  no particles will be moved over the pole.  This may give screwy
//  results for rotations around the dateline
/////////////////////////////////////////////////////////////////////////
void Ash::rotateGrid(double *loc, float val, ID l)
{
	// values in the rotated grid are the location of the south pole when
	// the grid is rotated.
	for (int i=0; i<ashN; i++)
	{
		if (l == LAT)
			loc[i] += double(val+90.0);
		if (l == LON)
			loc[i] += (double)val;
	}
	return;
}
/////////////////////////////////////////////////////////////////////////
//  move single point to the rotated grid location.  We assume that 
//  no points will be moved over the pole.  This may give screwy
//  results for rotations around the dateline
/////////////////////////////////////////////////////////////////////////
void Ash::rotateGridPoint(double *loc, float val, ID l)
{
	// values in the rotated grid are the location of the south pole when
	// the grid is rotated.
	if (l == LAT)
		loc[0] += double(val+90.0);
	if (l == LON)
		loc[0] += (double)val;
	return;
}
