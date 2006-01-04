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
#include <string>

#ifdef HAVE_SSTREAM
#include <sstream>
#elif HAVE_STRSTREAM
#include <strstream>
#define ostringstream ostrstream
#endif

#include <ctime>
#include <cmath>
#include <climits> // define FLT_MAX
#include <cfloat>

#include "ash.h"
#include "ashdump_options.h"

// file prototypes
//time_t unistr2time(char *unistr);  
void badrange(char *rangeStr);
void conaz(double pe, double re, double ps, double rs, double &del);
void get_stats();
void parse_flags();
void parse_ranges();
extern char *time2unistr(time_t time);

// global variables used for printing different ash fields
enum {F_LAT, F_LON, F_LEVEL, F_SIZE, F_AGE, F_ALL, F_NONE, F_STATS, F_HEADER, F_ACTIVE, F_ARGUMENTS};

bool flag[12];

Argument argument;
Ash ash;
float xRange[2], yRange[2], zRange[2], sRange[2];

int main(int argc, char **argv) 
{
	int iWidth;

	// default printing
	flag[F_ALL] = true;
	flag[F_LON] = false;
	flag[F_LAT] = false;
 	flag[F_LEVEL] = false;
	flag[F_SIZE] = false;
	flag[F_AGE] = false;
	flag[F_STATS] = false;
	flag[F_HEADER] = false;
	flag[F_ACTIVE] = false;
	flag[F_ARGUMENTS] = false;
    
    xRange[0] = -float(FLT_MAX);
    xRange[1] = float(FLT_MAX);
    yRange[0] = -float(FLT_MAX);
    yRange[1] = float(FLT_MAX);
    zRange[0] = -float(FLT_MAX);
    zRange[1] = float(FLT_MAX);
    sRange[0] = -float(FLT_MAX);
    sRange[1] = float(FLT_MAX);
    
		// parse options
    parse_options(argc, argv);

	if ( !argument.infile ) 
	{
		std::cerr << "\nashdump ERROR: File not found." << std::endl;
		exit(1);
	}
	
		// set which variable to print based on options set
    parse_flags();
		// set the ranges based on options set
    parse_ranges();

		// read in the ash object, die if it fails
	if ( ash.read(argument.infile) == ASH_ERROR ) exit(1);

    // Convert to feet:
    if ( argument.feet ) 
		{
      for (int i=0; i<ash.n(); i++) 
			{
				ash.r[i].z = ash.r[i].z * 3.28084;
      }
    }
    
    int fieldWidth = argument.width;
   
		// print header information
		if (flag[F_ALL] || flag[F_HEADER] || flag[F_STATS] || flag[F_ARGUMENTS])
		{
	std::cout << "Ash Object: " << argument.infile << std::endl;
	std::cout << std::endl;
	
	// uppercase the name
	if (strlen(ash.origname()) > 0) {
	    ash.origname()[0] = ash.origname()[0] & 223;
	}
	
	iWidth = 12; 
	std::cout.setf(std::ios::left, std::ios::adjustfield);
	
	std::cout << '\t';
	std::cout.width(iWidth);
	std::cout << "Origin:";
	std::cout << ash.origname() << " ( " << ash.origlon() << " , "
	          << ash.origlat() << " )";
	std::cout << std::endl;
	     
	std::cout << '\t';	
	std::cout.width(iWidth);
	std::cout << "Eruption:";
	std::cout << time2unistr(ash.origtime()) << " GMT";
	std::cout << std::endl;
	
	std::cout << '\t';
	std::cout.width(iWidth);
	std::cout << "Reference:";
	std::cout << time2unistr(ash.clock()) << " GMT";
	//time_t csecs = ash.clock() - unistr2time(time2unistr(ash.clock()));
	//cout << ':';
	//if (csecs < 10) std::cout << '0';
	//cout << csecs << " UTC";
	std::cout << std::endl;
	
	long diffTime = ash.clock() - ash.origtime();
	long diffHrs = diffTime/3600;
	long diffMins = (diffTime - int(float(diffHrs)*3600.))/60;
	//long diffSecs = diffTime -int(float(diffHrs)*3600.) 
	//               - int(float(diffMins)*60.);

	std::cout << '\t';
	std::cout.width(iWidth);
	std::cout << "Difference:";

	std::ostringstream diffStrm;
	diffStrm << diffHrs;
	diffStrm << ':';
	if (diffMins < 10) diffStrm << '0';
	diffStrm << diffMins;

//	cout.setf(ios::right, ios::adjustfield);
	std::cout.width(16);
	std::cout << diffStrm.str();
	std::cout << std::endl;
	
	// count number that are active, in range, and both active & in range 
	int nactive = 0;
	int nactiverange = 0;
	int nrange = 0;
	for (int i=0; i<ash.n(); i++) 
	{

    if ( ash.age(i) > 0 ) nactive++;
	    
    if ( ash.r[i].x >= xRange[0] && ash.r[i].x <= xRange[1] &&
    		 ash.r[i].y >= yRange[0] && ash.r[i].y <= yRange[1] &&
	    	 ash.r[i].z >= zRange[0] && ash.r[i].z <= zRange[1] ) 
		 {
		   nrange++;
		   if ( ash.age(i) > 0 ) nactiverange++;
	   }
	}
	
	iWidth = int(log10((double)ash.n())) + 1;
	std::cout.unsetf(std::ios::left);
	std::cout.setf(std::ios::right, std::ios::adjustfield);
	
	std::cout << std::endl;
	
	std::cout << '\t';
	std::cout.width(iWidth);
	std::cout << ash.n();
	std::cout << " Total particles.";
	std::cout << std::endl;
	
	std::cout << '\t';
	std::cout.width(iWidth);
	std::cout << nactive;
	std::cout << " Active particles.";
	std::cout << std::endl;
	
	std::cout << std::endl;
	
	std::cout << '\t';
	std::cout << "Requested Range [ lon, lat, height, size ]:" << std::endl;
	std::cout << std::endl;
	
	std::cout << "\t\t";
	std::cout << "[ (";
	if ( xRange[0] == -float(FLT_MAX) ) {
	    std::cout << '*';
	}
	else {
	    //cout.precision(1);
	    std::cout << xRange[0];
	}
	std::cout << '/';
	if ( xRange[1] == float(FLT_MAX) ) {
	    std::cout << '*';
	}
	else {
	    //cout.precision(1);
	    std::cout << xRange[1];
	}
	std::cout << "), (";
	if ( yRange[0] == -float(FLT_MAX) ) {
	    std::cout << '*';
	}
	else {
	    //cout.precision(1);
	    std::cout << yRange[0];
	}
	std::cout << '/';
	if ( yRange[1] == float(FLT_MAX) ) {
	    std::cout << '*';
	}
	else {
	    //cout.precision(1);
	    std::cout << yRange[1];
	}
	std::cout << "), (";
	if ( zRange[0] == -float(FLT_MAX) ) {
	    std::cout << '*';
	}
	else {
	    //cout.precision(1);
	    std::cout << zRange[0];
	}
	std::cout << '/';
	if ( zRange[1] == float(FLT_MAX) ) {
	    std::cout << '*';
	}
	else {
	    //cout.precision(1);
	    std::cout << zRange[1];
	}
	std::cout << "), (";
	if ( sRange[0] == -float(FLT_MAX) ) {
	    std::cout << '*';
	}
	else {
	    //cout.precision(1);
	    std::cout << sRange[0];
	}
	std::cout << '/';
	if ( sRange[1] == float(FLT_MAX) ) {
	    std::cout << '*';
	}
	else {
	    //cout.precision(1);
	    std::cout << sRange[1];
	}
	std::cout << ") ]";
	std::cout << std::endl;
	std::cout << std::endl;
	
	std::cout << '\t';
	std::cout.width(iWidth);
	std::cout << nrange;
	std::cout << " Particles in specified range.";
	std::cout << std::endl;
	
	std::cout << '\t';
	std::cout.width(iWidth);
	std::cout << nactiverange;
	std::cout << " Filtered particles (active & in range).";
	std::cout << std::endl;
	
	std::cout << std::endl;
	
	// calculate statistics
	if (flag[F_STATS]) get_stats();
	
	// show arguments
	if ( flag[F_ARGUMENTS] ) 
	{
    std::cout << std::endl;
    std::cout << "\tPuff Parameters:" << std::endl;
    //cout << "\t----------------" << std::endl;
    std::cout << "\t\tdate & time   = " << ash.date_time << std::endl;
    std::cout << "\t\terupt hours   = " << ash.erupt_hours << std::endl;
    std::cout << "\t\tplume height  = " << ash.plume_height << std::endl;
    std::cout << "\t\tplume shape   = " << ash.plume_shape << std::endl;
    std::cout << "\t\tplume Z width = " << ash.plume_width_z << std::endl;
    std::cout << "\t\tplume H width = " << ash.plume_width_h << std::endl;
    std::cout << "\t\tdiffuse Hor   = " << ash.diffuse_h << std::endl;
    std::cout << "\t\tdiffuse Vert  = " << ash.diffuse_v << std::endl;
    std::cout << "\t\tash log mean  = " << ash.log_mean << std::endl;
    std::cout << "\t\tash log sdev  = " << ash.log_sdev << std::endl;
    std::cout << std::endl;
	}
	
	// FIELDS HEADER:
	std::cout.unsetf(std::ios::left);
	std::cout.setf(std::ios::right, std::ios::adjustfield);
	
	if (flag[F_ALL] || flag[F_LON]) {
	    std::cout.width(fieldWidth);
	    std::cout << "LON (deg):";
	}
	if (flag[F_ALL] || flag[F_LAT]) {
	    std::cout.width(fieldWidth);
	    std::cout << "LAT (deg):";
	}
	if (flag[F_ALL] || flag[F_LEVEL]) {
	    if ( argument.feet ) {
	      std::cout.width(fieldWidth);
	      std::cout << "HEIGHT (ft):";
	    }
	    else {
	      std::cout.width(fieldWidth);
	      std::cout << "HEIGHT (m):";
	    }
	}
	if (flag[F_ALL] || flag[F_SIZE]) {
	    std::cout.width(fieldWidth);
	    std::cout << "SIZE (m):";
	}
	if (flag[F_ALL] || flag[F_AGE]) {
	    std::cout.width(fieldWidth);
	    std::cout << "AGE (hours):";
	}
	std::cout << std::endl;
    }
    
    // STOP IF ONLY HDR/STATS REQUESTED:
    if ( !flag[F_ALL] && !flag[F_LON] && !flag[F_LAT] && !flag[F_LEVEL] && !flag[F_SIZE] && !flag[F_AGE] ) {
	return 0;
    }
    
    // FIELDS DISPLAY:
    std::cout.unsetf(std::ios::left);
    std::cout.setf(std::ios::right, std::ios::adjustfield);
    std::cout.precision(int(argument.precision));
    
    float secs2hrs = 1.0/3600.0;
    int writeFlag;
    
    std::cout.unsetf(std::ios::scientific);
    std::cout.setf(std::ios::fixed);
    
    for (int i=0; i<ash.n(); i++) 
		{
	writeFlag = 0;
	
	// filter the range
	if ( ash.r[i].x >= xRange[0] && ash.r[i].x <= xRange[1] &&
	     ash.r[i].y >= yRange[0] && ash.r[i].y <= yRange[1] &&
	     ash.r[i].z >= zRange[0] && ash.r[i].z <= zRange[1] &&
	     ash.getSize(i) >= sRange[0] && ash.getSize(i) <= sRange[1]) {
	     
	     // FILTER AGE:
	     if ( !flag[F_ACTIVE] ) {
		 writeFlag = 1;
	     }
	     else {
		 if ( ash.age(i) >= 0 ) {
		     writeFlag = 1;
		 }
	     }
	}
	
	if ( writeFlag) {	
	    if (flag[F_ALL] || flag[F_LON]) {
		std::cout.width(fieldWidth);
		std::cout << ash.r[i].x << ' ';
	    }
		 
	    if (flag[F_ALL] || flag[F_LAT]) {
		std::cout.width(fieldWidth);
		std::cout << ash.r[i].y << ' ';
	    }
		 
	    if (flag[F_ALL] || flag[F_LEVEL]) {
		std::cout.width(fieldWidth);
		std::cout << ash.r[i].z << ' ';
	    }
		 
	    if (flag[F_ALL] || flag[F_SIZE]) {
		std::cout.unsetf(std::ios::fixed);
		std::cout.setf(std::ios::scientific);
		std::cout.width(fieldWidth);

		std::cout << ash.getSize(i) << ' ';

		std::cout.unsetf(std::ios::scientific);
		std::cout.setf(std::ios::fixed);
	    }
		 
	    if (flag[F_ALL] || flag[F_AGE]) {
		std::cout.width(fieldWidth);
		std::cout << ash.age(i)*secs2hrs << ' ';
	    }
		 
	    std::cout << std::endl;
	}    // END WRITEFLAG
	
	
    } // END OF NASH LOOP

    return 0;
}

///////////////////////////////////////////////////////////////////////////	
//
// PARSE FLAGS:
//
//////////////////////////////////////////////////////////////////////////
void parse_flags() 
{
    
  if ( argument.lon ) 
	{
  	flag[F_LON] = true;
  	flag[F_ALL] = false;
  }
    
  if ( argument.lat ) 
	{
  	flag[F_LAT] = true;
  	flag[F_ALL] = false;
  }
    
  if ( argument.showHeight) 
	{
		flag[F_LEVEL] = true;
  	flag[F_ALL] = false;
  }
    
  if ( argument.showSize ) 
	{
		flag[F_SIZE] = true;
  	flag[F_ALL] = false;
  }
    
  if ( argument.age ) 
	{
		flag[F_AGE] = true;
  	flag[F_ALL] = false;
  }

  if ( argument.hdr ) 
	{
		flag[F_HEADER] = true;
  	flag[F_ALL] = false;
  }
    
  if ( argument.stats ) 
	{
		flag[F_STATS] = true;
  	flag[F_ALL] = false;
  }
    
  if ( argument.active ) 
	{
		flag[F_ACTIVE] = true;
  }
    
  if ( argument.showParams ) 
	{
		flag[F_ARGUMENTS] = true;
  }
    
  return;    
}

///////////////////////////////////////////////////////////////////////////	
//
// PARSE RANGES:
//
//////////////////////////////////////////////////////////////////////////
void parse_ranges() {
  float temp;
#define SWAP(a,b) temp=(a); (a)=(b); (b)=temp;
  if ( argument.range ) {
    char *rangeStr = argument.range;
    // make a working copy so we can easily report bad rangStr
    char *pStr=rangeStr;
      
    if (*rangeStr != '*') {
      xRange[0]=(float)strtod(rangeStr,&pStr);
      if (*pStr == '\0') {badrange(rangeStr);}
      pStr++;
      xRange[1]=(float)strtod(pStr,&pStr);
      if (*pStr == '\0') {badrange(rangeStr);}
      pStr++; }
    else { pStr++; pStr++;}  // move forward two positions
    if (*pStr != '*') {  
      yRange[0]=(float)strtod(pStr,&pStr);
      if (*pStr == '\0') {badrange(rangeStr);}
      pStr++;
      yRange[1]=(float)strtod(pStr,&pStr);
//      if (*pStr == '\0') {badrange(rangeStr);}
      }
    if (xRange[0] > xRange[1]) { SWAP(xRange[0],xRange[1]) }
    if (yRange[0] > yRange[1]) { SWAP(yRange[0],yRange[1]) }
    }
  if ( argument.height ) {
     char *rangeStr = argument.height;
    // make a working copy so we can easily report bad rangStr
    char *pStr=rangeStr;
      
    if (*rangeStr != '*') {
      zRange[0]=(float)strtod(rangeStr,&pStr);
      if (*pStr == '\0') {badrange(rangeStr);}
      pStr++;
      zRange[1]=(float)strtod(pStr,&pStr);
//      if (*pStr == '\0') {badrange(rangeStr);}
      }
    if (zRange[0] > zRange[1]) { SWAP(zRange[0],zRange[1]) }
    }
 
  if ( argument.size ) {
     char *rangeStr = argument.size;
    // make a working copy so we can easily report bad rangStr
    char *pStr=rangeStr;
      
    if (*rangeStr != '*') {
      sRange[0]=(float)strtod(rangeStr,&pStr);
      if (*pStr == '\0') {badrange(rangeStr);}
      pStr++;
      sRange[1]=(float)strtod(pStr,&pStr);
//      if (*pStr == '\0') {badrange(rangeStr);}
      }
    if (sRange[0] > sRange[1]) { SWAP(sRange[0],sRange[1]) }
    }
         
  }
///////////////////////////////////////////////////////////////////////////	
void badrange(char *rangeStr) 
{
  std::cerr << std::endl;
  std::cerr << "ERROR: Bad range string: -range=\"" << rangeStr << "\"" << std::endl;
  std::cerr << "FAILED" << std::endl;
  exit(1);
	return;
}

///////////////////////////////////////////////////////////////////////////	
//
// STATS:
//
//////////////////////////////////////////////////////////////////////////
void get_stats() 
{
    
	// return if there is only one particle
  if ( ash.n() <= 1 ) return;
    
    // GEOMETRIC MEAN:
    double minLon = 999;
    double maxLon = -999;
    double minLat = 999;
    double maxLat = -999;
    double minZ = 99999;
    double maxZ = -999;
    double minS = 99999;
    double maxS = -999;
    double meanX = 0;
    double meanY = 0;
    double meanZ = 0;
    double meanS = 0;
    double logmeanS = 0;
    int count = 0;

    for (int i=0; i<ash.n(); i++) {
	if ( ash.age(i) > 0 &&
	     ash.r[i].x >= xRange[0] && ash.r[i].x <= xRange[1] &&
	     ash.r[i].y >= yRange[0] && ash.r[i].y <= yRange[1] &&
	     ash.r[i].z >= zRange[0] && ash.r[i].z <= zRange[1] &&
	     ash.getSize(i) >= sRange[0] && ash.getSize(i) <= sRange[1]) {
	     
	    meanX += ash.r[i].x;
	    meanY += ash.r[i].y;
	    meanZ += ash.r[i].z;
	    meanS += ash.getSize(i);
	    logmeanS += log10(ash.getSize(i));
	    
	    if ( ash.r[i].x < minLon ) minLon = ash.r[i].x;
	    if ( ash.r[i].x > maxLon ) maxLon = ash.r[i].x;
	    if ( ash.r[i].y < minLat ) minLat = ash.r[i].y;
	    if ( ash.r[i].y > maxLat ) maxLat = ash.r[i].y;
	    if ( ash.r[i].z < minZ ) minZ = ash.r[i].z;
	    if ( ash.r[i].z > maxZ ) maxZ = ash.r[i].z;
	    if ( ash.getSize(i) < minS ) minS = ash.getSize(i);
	    if ( ash.getSize(i) > maxS ) maxS = ash.getSize(i);
	    
	    count++;
	}
    }
    
    if ( count <= 1 ) {
	return;
    }
    
    int nash = count;
    
    meanX = meanX/nash;
    meanY = meanY/nash;
    meanZ = meanZ/nash;
    meanS = meanS/nash;
    logmeanS = logmeanS/nash;
    
    // STANDARD DEVIATION:
    double devX = 0;
    double devY = 0;
    double devZ = 0;
    double devS = 0;
    double logdevS = 0;
    double *distance = new double[nash];
    count = 0;
    for (int i=0; i<ash.n(); i++) {
	if ( ash.age(i) > 0 &&
	     ash.r[i].x >= xRange[0] && ash.r[i].x <= xRange[1] &&
	     ash.r[i].y >= yRange[0] && ash.r[i].y <= yRange[1] &&
	     ash.r[i].z >= zRange[0] && ash.r[i].z <= zRange[1] &&
	     ash.getSize(i) >= sRange[0] && ash.getSize(i) <= sRange[1]) {
	     
	    devX += (ash.r[i].x - meanX)*(ash.r[i].x - meanX);
	    devY += (ash.r[i].y - meanY)*(ash.r[i].y - meanY);
	    devZ += (ash.r[i].z - meanZ)*(ash.r[i].z - meanZ);
	    devS += (ash.getSize(i) - meanS)*(ash.getSize(i) - meanS);
	    logdevS += (log10(ash.getSize(i)) - logmeanS)*(log10(ash.getSize(i)) - logmeanS);
	    
	    conaz(ash.r[i].y, ash.r[i].x, meanY, meanX, distance[count]); 
	    count++;
	}
    }
    devX = sqrt(devX/(nash-1));
    devY = sqrt(devY/(nash-1));
    devZ = sqrt(devZ/(nash-1));
    devS = sqrt(devS/(nash-1));
    logdevS = sqrt(logdevS/(nash-1));
    
    double meanDistance = 0;
    for (int i=0; i<nash; i++) 
		{ 
			meanDistance += distance[i];
    }
    meanDistance = meanDistance/nash;
    
    double sdevDistance = 0.0;
    for (int i=0; i<nash; i++) {
	sdevDistance += (distance[i] - meanDistance)*(distance[i] - meanDistance);
    }
    sdevDistance = sqrt(sdevDistance/(nash-1));
    
    
    std::cout.setf(std::ios::fixed);
    std::cout.setf(std::ios::right, std::ios::adjustfield);
    
    std::cout << std::endl;
    std::cout << '\t' << "STATS: " << nash << " filtered particles:" << std::endl;
    std::cout << '\t' << "-----------------------------------" << std::endl;
    
    // RANGE:
    std::cout.precision(2);
    std::cout << "\t" << "Geometric Range: ";
    std::cout << "\t[ ";
    std::cout << minLon;
    std::cout << "/";
    std::cout << maxLon;
    std::cout << "/";
    std::cout << minLat;
    std::cout << "/";
    std::cout << maxLat;
    std::cout << "/";
    std::cout.precision(0);
    std::cout << minZ;
    std::cout << "/";
    std::cout << maxZ;
    std::cout << " ]";
    std::cout << std::endl;
    
    // MEAN:
    std::cout << std::endl;
    std::cout.precision(2);
    std::cout << "\t" << "Geometric Center:\n";
    std::cout << "\t\t    (";
    std::cout.width(7);
    std::cout << meanX;
    std::cout << ",";
    std::cout.width(6);
    std::cout << meanY;
    std::cout << ",";
    std::cout.precision(0);
    std::cout.width(6);
    std::cout << meanZ;
    std::cout << ")\n";
    std::cout << "\t\t+/- (";
    std::cout.precision(2);
    std::cout.width(7);
    std::cout << devX;
    std::cout << ",";
    std::cout.width(6);
    std::cout << devY;
    std::cout << ",";
    std::cout.precision(0);
    std::cout.width(6);
    std::cout << devZ;
    std::cout << ')';
    std::cout << std::endl;
    
    std::cout << std::endl;
    std::cout << "\t" << "Average distance from center:\n";
    std::cout << "\t\t" << meanDistance << " +/- " << sdevDistance
	 << " meters." << std::endl;

    std::cout << std::endl;
    std::cout << "\t" << "Size Distribution (meters):\n";
    
    
    std::cout.unsetf(std::ios::fixed);
    std::cout.setf(std::ios::scientific);
    std::cout.precision(2);
    std::cout << "\t\t" << "         Min/Max = " << minS << " / " << maxS << "\n";
    
    std::cout.precision(2);
    std::cout.unsetf(std::ios::scientific);
    std::cout.setf(std::ios::fixed);
    std::cout << "\t\t" << "Logarithmic Mean = " << logmeanS << " +/- " << logdevS << std::endl;
    
    std::cout.unsetf(std::ios::fixed);
    std::cout.setf(std::ios::scientific);
    std::cout.precision(2);
    std::cout << "\t\t" << "     Linear Mean = " << meanS << " +/- " << devS << std::endl;

    std::cout << std::endl;
}

/////////////////////////////////////////////////////////////////////////
// CONAZ:
//
// Calculate epicentral distance (DEL, in kilometers) and azimuth
// (AZ, in radians) from a point (PE,RE) to another point (PS,RS)
//
/////////////////////////////////////////////////////////////////////////
void conaz(double pe, double re, double ps, double rs, double &del) 
{
    
  static double sps;
  static double cps;
  static double spe;
  static double cpe;
  static double ses;
  static double ces;
  static double x;
  static double y;
  static double s;
  static double c;
  // earth radius in kilometers
  static double R0km = 6371.220;
    
  sps = sin(ps);
  cps = cos(ps);
  spe = sin(pe);
  cpe = cos(pe);
  ses = sin(re-rs);
  ces = cos(re-rs);
  x = sps*ses;
  y = cpe*sps*ces - spe*cps;
  s = sqrt(x*x + y*y);
  c = spe*sps*ces + cpe*cps;
    
  del = atan2(s, c)*R0km;
  return; 
}

