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

#include <iostream>
#include <fstream>
#include <cmath> 
#include <vector>
#include <string> // string type

#ifdef HAVE_SSTREAM
#include <sstream> // istringstream
#elif HAVE_STRSTREAM
#include <strstream>
#else
#error Do not have <sstream> or <strstream>
#endif

#include "ran_utils.h"
#include "particle.h"
#include "ash.h"
#include "cloud.h"
  
#define MAX_COMMENT_LENGTH	80
//////////////////////////////////
Cloud::~Cloud() {

  if (lonlatValues) delete[] lonlatValues;
  
  } 
//////////////////////////////////
// it is called lonlatValues but it actually holds lon, lat, and elevation
// values.
//////////////////////////////////
Cloud::Cloud(long n, char *name) {

  mean = -7;
  sdev = 1;
  restartFile = name;
  lonlatValues = new double[3*n];
  if (!lonlatValues) {
    std::cerr << "cannot allocate memory for cloud lat/lon values\n";
    exit(0);
    }
  build();
  return;
  }
  
//////////////////////////////////
// called during initialization of the cloud object.  Points defining the
// custom cloud are read in and a lengthscale for the polygonal region is
// returned.  Slopes are calculated for each line segment.  'define' returns
// a +/-1 value as 'inside', which is used to test whether later points are
// inside or outside the region.
//////////////////////////////////
void Cloud::build() {
    
  readPoints();
  setLengthScale();
  makeSlopes();
  if ( !validRegion() ) {
    std::cerr << "Invalid specification in restart file " << restartFile;
    std::cerr << ". Line segments intersect.\n";
    exit(1);
    }

  // set 'inside' to a +/- value that defines one side of the polygon
  inside = define(elevPoint[0].x,elevPoint[0].y);

  // check that all the elevation points are on the same side as the 
  // first elevation point.  
  // loop through the rest of the points and make sure they agree with what
  // the first elevation point defined as 'inside'
  for (int i=1; i<nElev; i++) {
    // inCloud return positive when inside and negative when outside
    if ( inCloud(elevPoint[i].x,elevPoint[i].y) < 0) {
      std::cerr << "ERROR: Elevation point " << i+1 << " at lon:" << elevPoint[i].x;
      std::cerr << " lat:" << elevPoint[i].y << " is not on the same side of the ";
      std::cerr << "of the polygon as elevation point 1 at lon:" << elevPoint[0].x;
      std::cerr << " lat:" << elevPoint[0].y << std::endl;
      exit(1);
      }
    }

  return;
  }
  
//////////////////////////////////  
// read in the cloud specification file.  There are three fields, each opened
// and closed with <field> and </field>. They can be opened/closed more than 
// once, but no two open at the same time.  They are
// verticies: pair of lon/lat points.
// elevations: trio of lon/lat/elevation within the polygon specified above.
// size: mean and sdev values, same as -ashLogMean and -ashLogSdev [optional]
// comments are allowed with the '#' character, and blank lines are ignored.
// A random walk step size is returned based on the size of the polygon sides.
//////////////////////////////////
void Cloud::readPoints() 
{
  // open the file
  std::ifstream cloudFile ( restartFile, std::ios::in );

  if (!cloudFile) {
    std::cerr << "Failed to open restart data file: "
         << restartFile << std::endl;
    exit(1);
    }
  
  int lineNumber = 0;  // line number for reporting errors
  int openFields = 0;  // for reporting errors when fields are not closed
  char *line = new char[1024];	// buffer to hold each line from file 
  std::string text;  // string copy of the buffer; easier to parse
  std::string::size_type loc; // location in string
  bool readingVerticies = false;
  bool readingElevations = false;
  bool readingSize = false;
  
  while (cloudFile.getline(line, 1024, '\n') != NULL) 
  {
    lineNumber++;
    // copy char* into string for parsing
    text = line; 
    // skip initial spaces
    loc = text.find_first_not_of(" ");
    if (loc != std::string::npos) text = text.substr(loc,text.length()-loc);
    // clear comment
    loc = text.find_first_of("#");
    if (loc != std::string::npos) text = text.substr(0,loc);
    // read next line if nothing is left
    if (text.length() == 0) continue;
    
    // determine if fields are being opened or closed
    if (text.find("<verticies>") != std::string::npos) 
    {
      readingVerticies = true;
      openFields++;
    }
    else if (text.find("</verticies>") != std::string::npos) 
    {
      readingVerticies = false;
      openFields--;
    }
    else if (text.find("<elevations>") != std::string::npos)
    { 
      readingElevations = true;
      openFields++;
    }
    else if (text.find("</elevations>") != std::string::npos) 
    {
      readingElevations = false;
      openFields--;
    }
    else if (text.find("<size>") != std::string::npos)
    { 
      readingSize = true;
      openFields++;
    }
    else if (text.find("</size>") != std::string::npos) 
    {
      readingSize = false;
      openFields--;
    }
    else {
    if (openFields > 1) 
    {
      std::cerr << "ERROR: Failed to close field in input file \"" << restartFile;
      std::cerr << "\" Missing a \"</elevations>\" for example?\n";
      exit(1);
    }
      
    if (readingVerticies)
    {
#ifdef HAVE_SSTREAM
      std::istringstream textStream(text);
#elif HAVE_STRSTREAM
      std::istrstream textStream(text.c_str());
#else
#error Do not have <sstream> or <strstream>
#endif
      Segment seg;
      textStream >> seg.x1 >> seg.y1; 
      // add them to the vertex vector
      segment.push_back(seg);
    }
    if (readingElevations) 
    {
#ifdef HAVE_SSTREAM
      std::istringstream textStream(text);
#elif HAVE_STRSTREAM
      std::istrstream textStream(text.c_str());
#else
#error Do not have <sstream> or <strstream>
#endif
      Particle part;
      textStream >> part.x >> part.y >> part.z; 
      // add them to the vertex vector
      elevPoint.push_back(part);
    }
    if (readingSize)
    {
#ifdef HAVE_SSTREAM
      std::istringstream textStream(text);
#elif HAVE_STRSTREAM
      std::istrstream textStream(text.c_str());
#else
#error Do not have <sstream> or <strstream>
#endif
      textStream >> mean >> sdev; 
    }
  }
  }
  if (openFields > 0) 
  {
    std::cerr << "ERROR: premature end of file reached in \"" << restartFile;
    std::cerr << "\".  Missing a \"</elevations>\" for example?\n";
    exit(1);
  }

  // set sizes
  nElev = elevPoint.size();
  nSegments = segment.size();
  
  // connect segments
  segment[nSegments-1].x2 = segment[0].x1;
  segment[nSegments-1].y2 = segment[0].y1;
  for (int i = 1; i < nSegments; i++) 
  {
    segment[i-1].x2 = segment[i].x1;
    segment[i-1].y2 = segment[i].y1;
  } 
  return;
  
}
//////////////////////////////////
// returns an integer +/-1.  For each line segment that is defined on an
// interval containing 'x', we calculate whether 'y' is above the line
// segment, and flip the sign of 'state' if it is.  It the final sign of
// 'state' for this (x,y) is the same as the state of the initialization point // (which is stored in the integer 'inside'), then this (x,y) is inside the
// region defined by the line segments.  This flipping routine should allow for
// a new point that skips over two (or any even integer number) boundary lines, // thus remaining inside the region.  Otherwise, we could have simply counted
// the number of line segments that were below the initialization point, and
// madated that all new points had to be above the same number of line segments.
//////////////////////////////////
int Cloud::define(double x, double y) {

  int i;
  int state = 1 ;
  
  for (i=0; i<nSegments; i++) {
    if ( ((x - segment[i].x1)*(x - segment[i].x2)) < 0 ){
      if ( y > segment[i].slope * x + segment[i].intercept ) {
        state *= (-1); }
      }
    }
    
  return state;
  }
    
//////////////////////////////////
// solve for 'm'(slope) and 'b'(intercept) in y = mx + b
//////////////////////////////////
void Cloud::makeSlopes() {
     
 for (int i=0; i<nSegments; i++) {
    if (segment[i].x1 != segment[i].x2) {
     segment[i].slope=(segment[i].y2-segment[i].y1) / 
                       (segment[i].x2-segment[i].x1);
     segment[i].intercept=segment[i].y1-segment[i].slope *
   	                   segment[i].x1;
     }
   else {
     segment[i].slope = HUGE_VAL;
     segment[i].intercept = HUGE_VAL;
     }
   }
 return;
 }

//////////////////////////////////
// check that no two line segment intersect, which would create an invalid
// region. To do this, two simultaneous y=mx+b equations are combined and 'x'
// is solved for. If 'x' is defined within both line segments, it is invalid.
//////////////////////////////////
bool Cloud::validRegion() {
  float xsol;
   
  for (int i=0; i<nSegments; i++) {
    // start 'j' at i+2 so we don't compare with our immediate neighbor
    for ( int j=i+2; j<nSegments; j++) {
      // do not compare the first segment with the last, they are neighbors
      if (i==0 && j==(nSegments-1)) continue;
      // if two segments have the same slope, they are parallel and can't
      // intersect.
      if ((segment[i].slope - segment[j].slope) == 0) continue;
      xsol = ((segment[j].intercept - segment[i].intercept) /
              (segment[i].slope - segment[j].slope));
      // see if 'xsol' is in both segments
      if ( ((xsol - segment[i].x1)*(xsol-segment[i].x2) < 0) &&
         ((xsol - segment[j].x1)*(xsol-segment[j].x2) < 0) ) 
	 return false;
      }
    }
  return true;
  }
//////////////////////////////////
void Cloud::setLengthScale() 
{
  float minSideLength = 1e9;
  float sideLength = 1e9;
  for (int i = 0; i < nSegments; i++) 
  {
    sideLength = sqrt( (segment[i].x2 - segment[i].x1)*
                       (segment[i].x2 - segment[i].x1) + 
		       (segment[i].y2 - segment[i].y1)*
                       (segment[i].y2 - segment[i].y1) );
    if ( sideLength  < minSideLength ) minSideLength = sideLength;
  }
  
  lengthScale = minSideLength;
  
  return;
}  
  
//////////////////////////////////
// bubble sort the elevation points based on how close they are to
// the (testX,testY) pair.  
//////////////////////////////////
 void Cloud::sortElevations() {
   bool changed = true;
   Particle temp;
   
   while (changed) { 
     changed = false;
     for (int i=0; i<nElev-1; i++) {
       if (distance(elevPoint[i]) > distance(elevPoint[i+1])) {
         temp = elevPoint[i];
	 elevPoint[i] = elevPoint[i+1];
	 elevPoint[i+1] = temp;
	 changed = true;
	 }  // end if()
      } // end for()
    } // end while (changed)
  return;   
}

//////////////////////////////////
// calculate the horizontal distance from the Point passed to it and the
// (testX,testY) pair
//////////////////////////////////
float Cloud::distance(Particle point) {
  
  return (sqrt((point.x-testX)*(point.x-testX) +
               (point.y-testY)*(point.y-testY)));
  }
  
//////////////////////////////////
// returns the linearly interpolated elevation using constants A, B, C
// solved for earlier in calcElevEqn
//////////////////////////////////
double Cloud::elevation(double x, double y) {

  testX = x;
  testY = y;
  
  sortElevations();
  
  // if there is more than 1 elevation point, weight the returned elevation
  // between the two closest.  Otherwise, use the only point we have.
  if (nElev > 1) 
  {
		// avoid dividing by zero when two elevation points are the same
	  float denominator = 	distance(elevPoint[0]) + distance(elevPoint[1]);
		if (denominator == 0) return elevPoint[0].z;
    return (distance(elevPoint[0])*elevPoint[1].z +
            distance(elevPoint[1])*elevPoint[0].z)/ denominator;
  } else {
    return elevPoint[0].z;
  }
}
  
//////////////////////////////////
// returns the integer +/-1 depending on whether the point (x,y) is in the
// same region at the first elevation point given in the input file 
//////////////////////////////////
int Cloud::inCloud (double x, double y) {
   
   return ( define(x,y) * inside ) ;
   
   }
//////////////////////////////////
// return a size value
//////////////////////////////////
float Cloud::size() {
  double logSize;
  logSize=mean+sdev*gasdev(iseed);
  return( pow(10., logSize) );
  }
  
//////////////////////////////////
// fills the specified region with particles using a random walk with a step
// size some order of magnitude (probably one) less than the characteristic
// length of the side of the polygon.  See RANDOM_WALK_STEP_SIZE in readPoints.
// A new random walk is initiated at each elevPoint so as to reasonably
// distribute the particles within the polygon in case it has an odd shape.
// An equal number of points is alloted to each elevPoint, with the remainder
// added to the last point
//////////////////////////////////
void Cloud::fill(int ashN) 
{

  int numTries = 0; 
  int i = 0;
  int validPoints,nPolySide,state,ashNLocal;
  double lonTest,latTest;
  int failed = 0;
  
  for (nPolySide=0; nPolySide<nElev; nPolySide++) {
// begin the random walk at the specified elevPoint
    lonTest = elevPoint[nPolySide].x;
    latTest = elevPoint[nPolySide].y; 
    validPoints = 0;
    state = 1;		// first point (elevPoint) is assumed to be valid
    ashNLocal = (int)floor((float)(ashN/nElev));
    if (nPolySide == (nElev-1))  ashNLocal += (ashN%nElev); 
    while (validPoints < ashNLocal ) {
    numTries++;

// if state is greater than zero, both (lonTest,latTest) and elevPoint[0] 
// reside within the polygon
 
      if ( state > 0 ) {
        lonlatValues[i]        = lonTest;
        lonlatValues[ashN+i]   = latTest;
	lonlatValues[2*ashN+i] = elevation(lonTest,latTest);
      i++;
      validPoints++;

      
      }
      else {
        failed++;
        }      

      lonTest = (ran1(iseed)-0.5) * lengthScale + lonlatValues[i-1];
      latTest = (ran1(iseed)-0.5) * lengthScale + lonlatValues[ashN+i-1];
      state = inCloud(lonTest,latTest) ;

      }
    if (numTries > ashN*1000) 
    {
    std::cerr << "ERROR: Failed to fill custom cloud after considerable effort."
      << "Perhaps the specified shape in file \"" << restartFile
      << "\" has an extreme aspect ratio or curvature.  You can disable "
      << "this by adding the line \"do not stop trying\" in the cloud "
      << "specification file.\n";
      exit(1);
    }
  }

  return;
}
//////////////////////////////////
double* Cloud::customCloud(long ashN) 
{
  fill(ashN);
  return (lonlatValues);

}
