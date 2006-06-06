#ifndef PUFF_ATMOSPHERE_H
#define PUFF_ATMOSPHERE_H

#include <string> // string
#include <ctime> // time_t
#include "Grid.h"
#include "particle.h"

class Atmosphere {

private:
  Grid P;
  Grid U;
  Grid V;
  Grid W;
  Grid T;
	Grid Kh;
  
  // paths and filenames from where the data was read
  std::string filenameU;
  std::string filenameV;
  std::string filenameW;
  
  time_t reftime_t;

public:
  
	double *center_lon, *center_lat;
	struct GridRotation rotGrid;
  Atmosphere();
  ~Atmosphere();
 
 int init(double *lon, double *lat);
 
 // these functions return scalar values for atmospheric conditions at a
 // given x,y,z point given by Particle's location
 float fallVelocity(float time, Particle *p);
 float temperature (float time, Particle *p);
 float xSpeed(float time, Particle *p); 
 float ySpeed(float time, Particle *p); 
 float zSpeed(float time, Particle *p); 
 float pressure(float time, Particle *p);
 float diffuseKh(float time, Particle *p);
 
 // these return the full path and filename for where the data was read from
 const std::string *fileU() { return &filenameU;};
 const std::string *fileV() { return &filenameV;};
 const std::string *fileW() { return &filenameW;};
 
 // return a true/false whether this x,y point is within the atmospheric data
 bool containsXYPoint(float x, float y);
 int containsZPoint(float z);
 bool containsXYZPoint(float x, float y, float z);
 
 bool isGlobal();
 bool isProjectionGrid();
 
 float xMin() { return U.min(LON);}
 float xMax() { return U.max(LON);}
 float yMin() { return U.min(LAT);}
 float yMax() { return U.max(LAT);}
 float zMin() { return U.min(LEVEL);}
 float zMax() { return U.max(LEVEL);}
 
 time_t reference_time();
 
private:
  int make_winds();
  int read_uni(Grid &grid, std::string *filename);
  int wind_create_W(Grid &U, Grid &V, Grid &W, Grid &Kh);
	void checkRotatedGrid(const char *file);
	void checkRotatedGridError();
};

#ifdef HAVE_EXP10
extern "C" {double exp10(double a);}
#endif

#endif
