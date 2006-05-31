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

#include "Grid.h"

const int FG_ERROR = 1;
const int FG_OK = 0;

///////////////////////////////////////////////////////////////////
//
// CONSTRUCTORS:
//
///////////////////////////////////////////////////////////////////
Grid::Grid() {
    fgNdims = 0;
    fgData[VAR].size = 0;
    initialize();
}

Grid::Grid(unsigned int nx1, unsigned int nx2, 
                 unsigned int nx3, unsigned int nx4) {
    fgNdims = 4;
    initialize();
    allocate(nx1, nx2, nx3, nx4);
}

void Grid::create(unsigned int nx1, unsigned int nx2, 
                     unsigned int nx3, unsigned int nx4) {
    fgNdims = 4;
    allocate(nx1, nx2, nx3, nx4);    
}

///////////////////////////////////////////////////////////////////
//
// ALLOCATE:
// allocates the array spaces and initializes certian arrays
//
///////////////////////////////////////////////////////////////////
void Grid::allocate(unsigned int nx1, unsigned int nx2, 
		       unsigned int nx3, unsigned int nx4) {

    switch ( fgNdims ) {
	case (1) : {
	    if ( nx1 <= 0 ) {
		fgErrorStrm << "allocate(int): Negative or Zero valued." << std::endl;
		fg_error();
	    }
	    fgData[FRTIME].size = nx1;
	    fgData[VAR].size = nx1;
	    break;
	}
	case (2) : {
	    if ( nx1 <= 0 || nx2 <= 0 ) {
		fgErrorStrm << "allocate(int,int): Negative or Zero valued." << std::endl;
		fg_error();
	    }
	    fgData[FRTIME].size = nx1;
	    fgData[LEVEL].size = nx2;
	    fgData[VAR].size = nx1*nx2;
	    break;
	}
	case (3) : {
	    if ( nx1 <= 0 || nx2 <= 0 || nx3 <= 0 ) {
		fgErrorStrm << "allocate(int,int,int): Negative or Zero valued." << std::endl;
		fg_error();
	    }
	    fgData[FRTIME].size = nx1;
	    fgData[LEVEL].size = nx2;
	    fgData[LAT].size = nx3;
	    fgData[VAR].size = nx1*nx2*nx3;
	    break;
	}
	case (4) : {
	    if ( nx1 <= 0 || nx2 <= 0 || nx3 <= 0 || nx4 <=0 ) {
		fgErrorStrm << "allocate(int,int,int,int): Negative or Zero valued." << std::endl;
		fg_error();
	    }
	    fgData[FRTIME].size = nx1;
	    fgData[LEVEL].size = nx2;
	    fgData[LAT].size = nx3;
	    fgData[LON].size = nx4;
	    fgData[VAR].size = nx1*nx2*nx3*nx4;
	    break;
	}
	default : {
	    fgErrorStrm << "allocate(): Bad dimension" << std::endl;
	    fg_error();
	}
    }
    
    // ALLOCATE:
    unsigned int i;
    for (i=0; i<=fgNdims; i++) {
	fgData[i].val = new float[fgData[i].size];
	if ( !fgData[i].val ) {
	    fgErrorStrm << "allocate(): new failed" << std::endl;
	    fg_error();
	}
    }

    return;
  }

///////////////////////////////////////////////////////////////////
//
// DESTRUCTOR:
//
///////////////////////////////////////////////////////////////////
Grid::~Grid() {

    unsigned int i;
    for (i=0; i<=fgNdims; i++) {
	
	if ( fgData[i].val ) 
	    free(fgData[i].val);
	
    }
    
}

///////////////////////////////////////////////////////////////////
//
// ERROR ROUTINES:
//
///////////////////////////////////////////////////////////////////
void Grid::fg_error() {
    
    std::cerr << std::endl;
    std::cerr << fgErrorStrm.str() << std::endl;

    
}

void Grid::fg_error_ndims() {
    
    fgErrorStrm << "operator(): object \"" << fgData[VAR].name << "\" expects a "
                << fgNdims << "D call." << std::endl;
    fg_error();
    
}

void Grid::fg_error_index(ID dimid) {
    
    fgErrorStrm << "operator(): Index X" << int(dimid) << " out of bounds for object \""
                << fgData[VAR].name << "\"." << std::endl;
    fg_error();
    
    return;
}

//////////////////////////////////////////////////////////////////////
//
// DIMENSION SETS:
//
//////////////////////////////////////////////////////////////////////
int Grid::dim_norm(ID dimid, float fmax) {
  std::cerr << "function dim_norm is deprecated\n";
  exit(1);
    return FG_OK;
}

int Grid::dim_step(ID dimid, float start, float step) {
  std::cerr << "function dim_step is deprecated\n";
  exit(1);
    return FG_OK;
}

//////////////////////////////////////////////////////////////////////
//
// BASIC STATS:
//
//////////////////////////////////////////////////////////////////////
// PERCENT BAD:
float Grid::pct_bad() {
    if ( fgData[VAR].size == 0 ) {
	fgErrorStrm << "pct_bad(): Empty object";
	fg_error();
    }
    unsigned count = 0;
    for (unsigned i=0; i<fgData[VAR].size; i++) {
	if (fgData[VAR].val[i] < fgData[VAR].range[0] || 
	    fgData[VAR].val[i] > fgData[VAR].range[1] ||
	    fgData[VAR].val[i] == fgFillValue) { 
	    	count++;
	}
    }
    return 100.*float(count)/float(fgData[VAR].size);
};

// PERCENT FILL:
float Grid::pct_fill() {
    if ( fgData[VAR].size == 0 ) {
	fgErrorStrm << "pct_fill(): Empty object";
	fg_error();
	return 0.0;
    }
    unsigned count = 0;
    for (unsigned i=0; i<fgData[VAR].size; i++) {
	if (fgData[VAR].val[i] == fgFillValue) { 
	    count++;
	}
    }
    return 100.*float(count)/float(fgData[VAR].size);
};

// MINIMUM:
void Grid::set_minimum(ID idx) {
    if ( fgData[idx].size == 0 ) return;
    float minimum = fgData[idx].val[0];
    for (unsigned i=0; i<fgData[idx].size; i++) {
	if ( fgData[idx].val[i] != fgFillValue && fgData[idx].val[i] < minimum) {
	    minimum = fgData[idx].val[i];
	}
    }
	min_value[idx] = minimum;
};

float Grid::min(ID idx) {
	return min_value[idx];
}

// MAXIMUM:
void Grid::set_maximum( ID idx) {
    if ( fgData[VAR].size == 0 ) return;
    float maximum = fgData[idx].val[0];
    for (unsigned i=0; i<fgData[idx].size; i++) {
	if ( fgData[idx].val[i] != fgFillValue && fgData[idx].val[i] > maximum) {
	    maximum = fgData[idx].val[i];
	}
    }
	max_value[idx] = maximum;
};

float Grid::max(ID idx) {
	return max_value[idx];
}

// MEAN:
float Grid::mean() {
    if ( fgData[VAR].size == 0 ) {
	fgErrorStrm << "mean(): Empty object";
	fg_error();
	return 0.0;
    }
    float sum = 0;
    int icount = 0;
    for (unsigned i=0; i<fgData[VAR].size; i++) {
        if ( fgData[VAR].val[i] != fgFillValue ) {
	    sum += fgData[VAR].val[i];
	    icount++;
	}
    }
    if ( icount != 0 ) {
	return sum/(float(icount));
    }
    else {
	return fgFillValue;
    }

};


// VARIANCE:
float Grid::variance() {
    if ( fgData[VAR].size == 0 ) {
	fgErrorStrm << "variance(): Empty object";
	fg_error();
	return 0.0;
    }
    float tempval;
    float s_squared = 0;
    float mn = this->mean();
    int icount = 0;
    for (unsigned i=0; i<fgData[VAR].size; i++) {
	if ( fgData[VAR].val[i] != fgFillValue ) {
	    tempval = fgData[VAR].val[i] - mn;
	    tempval *= tempval;
	    s_squared += tempval;
	    icount++;
	}
    }
    if ( icount > 0 ) {
	s_squared /= float(icount - 1);
    }
    else {
	s_squared = 0.0;
    }

    return s_squared;
};

//////////////////////////////////////////////////////////////////////
//
// RETURN VALUES:
//
//////////////////////////////////////////////////////////////////////
float Grid::valid_range(unsigned i) {
    if (i != 0 && i != 1) {
	fgErrorStrm << "valid_range(int) : index must be 0 or 1";
	fg_error();
	return 0.0;
    }
    return fgData[0].range[i];
}

float Grid::valid_range(ID dimid , unsigned i) {
    if (i != 0 || i != 1) {
	fgErrorStrm << "valid_range(ID,int) : index must be 0 or 1";
	fg_error();
	return 0.0;
    }
    return fgData[dimid].range[i];
}

//////////////////////////////////////////////////////////////////////
//
// SET VALUES:
//
//////////////////////////////////////////////////////////////////////
void Grid::set_valid_range(float rlo, float rhi) {
    if (rlo > rhi) {
	fgData[VAR].range[1] = rlo;
	fgData[VAR].range[0] = rhi;	
    }
    else {
	fgData[VAR].range[0] = rlo;
	fgData[VAR].range[1] = rhi;	
    }
}

void Grid::set_range(ID index, float rlo, float rhi) {
    if (rlo > rhi) {
	fgData[index].range[1] = rlo;
	fgData[index].range[0] = rhi;	
    }
    else {
	fgData[index].range[0] = rlo;
	fgData[index].range[1] = rhi;	
    }
	// don't let latitude values be out of range
	if (fgData[LAT].range[0] < -90) fgData[LAT].range[0] = -90;
	if (fgData[LAT].range[1] > +90) fgData[LAT].range[1] = +90;
	return;
}

void Grid::set_name(const char *s) {
    if ( strlen(s) < FGMAXCHAR ) {
	strcpy( fgData[VAR].name, s );
    }
    else {
	strncpy( fgData[VAR].name, s, FGMAXCHAR-1 );
	fgData[VAR].name[FGMAXCHAR-1] = '\0';
    }
}

void Grid::set_name(ID index, const char *s) {
    if ( strlen(s) < FGMAXCHAR ) {
	strcpy( fgData[index].name, s );
    }
    else {
	strncpy( fgData[index].name, s, FGMAXCHAR-1 );
	fgData[index].name[FGMAXCHAR-1] = '\0';
    }
}

void Grid::set_units(const char *s) {
    if ( strlen(s) < FGMAXCHAR ) {
	strcpy( fgData[VAR].units, s );
    }
    else {
	strncpy( fgData[VAR].units, s, FGMAXCHAR-1 );
	fgData[VAR].units[FGMAXCHAR-1] = '\0';
    }
}

void Grid::set_units(ID index, const char *s) {
    if ( strlen(s) < FGMAXCHAR ) {
	strcpy( fgData[index].units, s );
    }
    else {
	strncpy( fgData[index].units, s, FGMAXCHAR-1 );
	fgData[index].units[FGMAXCHAR-1] = '\0';
    }
}

void Grid::set_title(const char *s) {
    if ( strlen(s) < FGMAXCHAR ) {
	strcpy( fgTitle, s );
    }
    else {
	strncpy( fgTitle, s, FGMAXCHAR-1 );
	fgTitle[FGMAXCHAR-1] = '\0';
    }
}

void Grid::set_reftime(const char *s) {
    if ( strlen(s) < FGMAXCHAR ) {
	strcpy( fgReftime, s );
    }
    else {
	strncpy( fgReftime, s, FGMAXCHAR-1 );
	fgReftime[FGMAXCHAR-1] = '\0';
    }
}

void Grid::spacing(char *s) {
    strncpy(fgSpace, s, 6);
    fgSpace[6] = '\0';
}

/////////////////////////////////////////////////////////////////////////////
//
// LOCATE:
// return the index j from an ordered table xx[n] such that x lies between
// xx[j] and xx[j+1]. This is from Numerical Recipes and modified for
// zero-index arrays xx[0] to xx[n-1]. A return value of j=-1 lies below 
// xx[0] and a return value of j=n lies above xx[n-1].
//
////////////////////////////////////////////////////////////////////////////
void Grid::locate(float *xx, int n, float x, int &j) {

    int ju, jm, jl;
    int ascnd;

    jl = -1;
    ju = n+1;

    ascnd = ( xx[n-1] > xx[0] );

    while ( ju-jl > 1 ) {
	jm = (ju+jl) >> 1;
	if ( x > xx[jm] == ascnd ) {
	    jl = jm;
	}
	else {
	    ju = jm;
	}
    }
    j = jl;

    if ( !ascnd && j == n-1 ) j++;

    return;

}
////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////
void Grid::set_coverage() {

  // only bother to check for global data if units are degrees in both
  // longitude and latitude

  if ( strncmp(fgData[LON].units,"km",2) == 0 ||
       strncmp(fgData[LON].units,"KM",2) == 0 || 
       strncmp(fgData[LON].units,"kilometer",9) == 0 ) {
    coverage = REGIONAL;
    return;
    }
    
  if ( strncmp(fgData[LAT].units,"km",2) == 0 ||
       strncmp(fgData[LAT].units,"KM",2) == 0 ||
       strncmp(fgData[LAT].units,"kilometer",9) == 0 ) {
    coverage = REGIONAL;
    return;
    }
    
    
  double delta;
  double minLon = min(LON);
  double maxLon = max(LON);
  double minLat = min(LAT);
  double maxLat = max(LAT);
  
  // determine longitude coverage
  // data should already be sorted from smallest to largest positive values
  delta = fgData[LON].val[1]-fgData[LON].val[0];
  // use 1 percent to allow for round-off error
  if ((360-maxLon)+minLon >= (1.01)*delta ){
    coverage = REGIONAL;
    return;
    }
  // global coverage needs data at the poles; 90 and -90 degrees lat
  delta = fgData[LAT].val[1]-fgData[LAT].val[0];
  delta = sqrt(delta*delta);
  if ((90-maxLat)+(minLat+90) >= (1.01)*delta ) {
    coverage = REGIONAL;
    return;
    } 
  // if we get to here, it must be global data
    coverage = GLOBAL;
  return;
  }
////////////////////////////////////////////////////////////////////////////
// public access to whether data is global
////////////////////////////////////////////////////////////////////////////
 
bool Grid::isGlobal() {

  return (coverage == GLOBAL);  
  }
////////////////////////////////////////////////////////////////////////////
// public access to whether data is a projection grid
////////////////////////////////////////////////////////////////////////////

bool Grid::isProjectionGrid() {
 // if kilometers match LAT and LON units, assume projection grid
  if ( ( strncmp(fgData[LAT].units,"km",2) == 0 ||
         strncmp(fgData[LAT].units,"KM",2) == 0 ||
         strncmp(fgData[LAT].units,"kilometer",7) == 0 ) &&
       ( strncmp(fgData[LON].units,"km",2) == 0 ||
         strncmp(fgData[LON].units,"KM",2) == 0 ||
         strncmp(fgData[LON].units,"kilometer",7) == 0 ) )
           return true;
  // otherwise we don't know, so return false
  return false;
}
////////////////////////////////////////////////////////////////////////////
// file temperature Grid with standard atmosphere from
// http://www.usatoday.com/weather/wstdatmo.htm
////////////////////////////////////////////////////////////////////////////
void Grid::TstandardAtm()
{
  if (!(*this).empty() ) 
  {
    std::cerr << "ERROR: Temperature object is not empty\n";
    exit(0);
  }
  
  // data is 2 dimensional
  fgNdims = 2;
  
  // allocate space for data
  allocate(1, 36);
  
  fgData[LEVEL].val[0] = 0000;  fgData[VAR].val[0] =  15.0;
  fgData[LEVEL].val[1] = 1000;  fgData[VAR].val[1] =  8.5;
  fgData[LEVEL].val[2] = 2000;  fgData[VAR].val[2] =  2.0;
  fgData[LEVEL].val[3] = 3000;  fgData[VAR].val[3] = -4.5;
  fgData[LEVEL].val[4] = 4000;  fgData[VAR].val[4] = -11.0;
  fgData[LEVEL].val[5] = 5000;  fgData[VAR].val[5] = -17.5;
  fgData[LEVEL].val[6] = 6000;  fgData[VAR].val[6] = -24.0;
  fgData[LEVEL].val[7] = 7000;  fgData[VAR].val[7] = -30.5;
  fgData[LEVEL].val[8] = 8000;  fgData[VAR].val[8] = -37.0;
  fgData[LEVEL].val[9] = 9000;  fgData[VAR].val[9] = -43.5;
  fgData[LEVEL].val[10] = 10000; fgData[VAR].val[10] = -50.0;
  fgData[LEVEL].val[11] = 11000; fgData[VAR].val[11] = -56.5;
  fgData[LEVEL].val[12] = 12000; fgData[VAR].val[12] = -56.5;
  fgData[LEVEL].val[13] = 13000; fgData[VAR].val[13] = -56.5;
  fgData[LEVEL].val[14] = 14000; fgData[VAR].val[14] = -56.5;
  fgData[LEVEL].val[15] = 15000; fgData[VAR].val[15] = -56.5;
  fgData[LEVEL].val[16] = 16000; fgData[VAR].val[16] = -56.5;
  fgData[LEVEL].val[17] = 17000; fgData[VAR].val[17] = -56.5;
  fgData[LEVEL].val[18] = 18000; fgData[VAR].val[18] = -56.5;
  fgData[LEVEL].val[19] = 19000; fgData[VAR].val[19] = -56.5;
  fgData[LEVEL].val[20] = 20000; fgData[VAR].val[20] = -56.5;
  fgData[LEVEL].val[21] = 21000; fgData[VAR].val[21] = -55.5;
  fgData[LEVEL].val[22] = 22000; fgData[VAR].val[22] = -54.5;
  fgData[LEVEL].val[23] = 23000; fgData[VAR].val[23] = -53.5;
  fgData[LEVEL].val[24] = 24000; fgData[VAR].val[24] = -52.5;
  fgData[LEVEL].val[25] = 25000; fgData[VAR].val[25] = -51.5;
  fgData[LEVEL].val[26] = 26000; fgData[VAR].val[26] = -50.5;
  fgData[LEVEL].val[27] = 27000; fgData[VAR].val[27] = -49.5;
  fgData[LEVEL].val[28] = 28000; fgData[VAR].val[28] = -48.5;
  fgData[LEVEL].val[29] = 29000; fgData[VAR].val[29] = -47.5;
  fgData[LEVEL].val[30] = 30000; fgData[VAR].val[30] = -46.5;
  fgData[LEVEL].val[31] = 31000; fgData[VAR].val[31] = -45.5;
  fgData[LEVEL].val[32] = 32000; fgData[VAR].val[32] = -44.5;
  fgData[LEVEL].val[33] = 33000; fgData[VAR].val[33] = -41.7;
  fgData[LEVEL].val[34] = 34000; fgData[VAR].val[34] = -38.9;
  fgData[LEVEL].val[35] = 35000; fgData[VAR].val[35] = -36.1;
  
  // these values should be Kelvin
  for (unsigned int i = 0; i < fgData[VAR].size; i++)
  {
    fgData[VAR].val[i] = fgData[VAR].val[i] + 273.15;
  }
  
  // LEVEL units are meters
  strcpy(fgData[LEVEL].units,"meters");
  
  // VAR units are degrees_kelvin
  strcpy(fgData[VAR].units, "degree_Kelvin");
  return;
}
