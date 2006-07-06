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

#include <cmath>

#include "Grid.h"

/////////////////////////////////////////////////////////////////////////
//
// Grid INITIALIZE:
//
/////////////////////////////////////////////////////////////////////////
void Grid::initialize ()
{

  strcpy (fgData[LON].name, "lon");
  strcpy (fgData[LON].units, "degrees_east");

  strcpy (fgData[LAT].name, "lat");
  strcpy (fgData[LAT].units, "degrees_north");

  strcpy (fgData[LEVEL].name, "level");
  strcpy (fgData[LEVEL].units, "millibars");

  strcpy (fgData[FRTIME].name, "frtime");
  strcpy (fgData[FRTIME].units, "hours");

  strcpy (fgTitle, "uniGrid object with real long name");

  // SET DEFUALT TO FALSE:
  uniShiftWest = 0;

  fgData[FRTIME].range[0] = -1.e30;
  fgData[FRTIME].range[1] = 1.e30;

  fgData[LEVEL].range[0] = -1.e30;
  fgData[LEVEL].range[1] = 1.e30;

  fgData[LAT].range[0] = -1.e30;
  fgData[LAT].range[1] = 1.e30;

  fgData[LON].range[0] = -1.e30;
  fgData[LON].range[1] = 1.e30;

  //added when merging Grids
  fgData[VAR].range[0] = -1.e30;
  fgData[VAR].range[1] = 1.e30;

	// min/max values, are they the same as range?
	min_value[LON]=min_value[LAT]=min_value[LEVEL]=min_value[FRTIME]=min_value[VAR]=-1e-30;
	max_value[LON]=max_value[LAT]=max_value[LEVEL]=max_value[FRTIME]=max_value[VAR]=1e-30;

  // Grid FIELDS:
  fgFillValue = -9999.0;
  strcpy (fgTitle, "Grid object");
  strcpy (fgReftime, "");

  // default scale factor and offset
  scale_factor = 1.0;
  add_offset = 0.0;

  // ERROR STREAM:
  fgErrorStrm << "Grid ERROR: ";
  uniErrorStrm << "Grid ERROR: ";

  // OUTPUT VALS, DEFAULTED IN HEADER:
  this->width ();
  this->precision ();
  this->leftadjust ();
  strcpy (fgSpace, " ");
  coverage = UNKNOWN;

  fgData[VAR].val    = NULL;
  fgData[FRTIME].val = NULL;
  fgData[LEVEL].val  = NULL;
  fgData[LAT].val    = NULL;
  fgData[LON].val    = NULL;

}

/////////////////////////////////////////////////////////////////////////
//
// uniGrid ERROR:
//
/////////////////////////////////////////////////////////////////////////
void Grid::uni_error ()
{

  std::cerr << std::endl;
  std::cerr << uniErrorStrm.str () << std::endl;

}

/////////////////////////////////////////////////////////////////////////
//
// uniGrid SORTER:
//
/////////////////////////////////////////////////////////////////////////
void Grid::shellsort (float *arrPtr, int arrSize)
{
  unsigned i, j, offset, n;
  float swapElem;

  unsigned first = 0;
  unsigned last = unsigned (arrSize - 1);
  unsigned isSorted = 0;

  if (arrSize < 2)
    return;

  // adjust the parameter last?
  last = (last >= unsigned (arrSize)) ? arrSize - 1 : last;
  n = last - first + 1;
  offset = n;
  while (offset > 1) {
    // update offset and make sure it never goes below 1
    offset = (offset - 1) / 3;
    offset = (offset == 0) ? 1 : offset;
    // order neighbors that are offset elements apart
    do {
      isSorted = 1;
      // compare neighbors that are offset elements apart
      for (i = first; i <= (last - offset); i++) {
	j = i + offset;
	// need to swap?
	if (*(arrPtr + i) > *(arrPtr + j)) {
	  // swap elements i and j
	  isSorted = 0;
	  swapElem = *(arrPtr + i);
	  *(arrPtr + i) = *(arrPtr + j);
	  *(arrPtr + j) = swapElem;
	}
      }
    } while (!isSorted);
  }

}

/////////////////////////////////////////////////////////////////////////
//
// SHIFT WEST VALUES:
//
/////////////////////////////////////////////////////////////////////////
int Grid::uni_shift_west ()
{
  
  unsigned int i, j, k, l;
//  exit(0);
  // MAKE A TEMP COPY OF LON:
  float *pLon = new float[fgData[LON].size];
  if (!pLon) {
    uniErrorStrm << "uni_shift_west(): new failed." << std::endl;
    uni_error ();
    return FG_ERROR;
  }

  // COPY:
  for (i = 0; i < fgData[LON].size; i++) {
    pLon[i] = fgData[LON].val[i];
  }

  // MAKE A TEMP COPY OF THE DATA VAL:
  float *pVal = new float[fgData[VAR].size];
  if (!pVal) {
    uniErrorStrm << "uni_shift_west() : new failed." << std::endl;
    uni_error ();
    return FG_ERROR;
  }

  for (i = 0; i < fgData[VAR].size; i++) {
    pVal[i] = fgData[VAR].val[i];
  }

  // SHIFT NEGATIVE LONGITUDE VALUES:
  for (i = 0; i < fgData[LON].size; i++) {
    if (fgData[LON].val[i] < 0) {
      fgData[LON].val[i] += 360.0;
    }
  }

  // SORT THE DATA LON:
  shellsort (fgData[LON].val, fgData[LON].size);

  // FIND ANY DUPLICATE LINES:
  float londup = 0;
  int ndup = 0;

  for (i = 0; i < fgData[LON].size - 1; i++) {
    if (fgData[LON].val[i + 1] == fgData[LON].val[i]) {
      londup = fgData[LON].val[i];
      ndup++;
    }
  }

  if (ndup > 0) {
    ndup = 0;
    float dlon = fgData[LON].val[1] - fgData[LON].val[0];
    for (i = 0; i < fgData[LON].size; i++) {
      if (fgData[LON].val[i] >= londup && ndup > 0) {
	fgData[LON].val[i] += dlon;
      }
      if (fgData[LON].val[i] == londup) {
	ndup++;
      }
    }
  }


  // REARRANGE DATA:
  // pLon[] contains the original lon values, some of which are probably -'ive
  // fgData[LON].val[] now contains only positive values.  What we do is find
  // the index in pLon[] where the longitudes are the same in both pLon[] and
  // fgData[LON].val[].  The index in pLon[] will be 'll' and the corresponding 
  // index in fgData[LON].val[] will be 'l'.   
  unsigned ll;
  int llo;
  float old_lon;
  for (l = 0; l < fgData[LON].size; l++) {

    for (ll = 0; ll < fgData[LON].size; ll++) {
      if (pLon[ll] < 0) {
	old_lon = pLon[ll] + 360.0;
      }
      else {
	old_lon = pLon[ll];
      }

      // see if these indicies point to corresponding values
      if (old_lon == fgData[LON].val[l]) {
        // this is the index we want, copy it to 'llo' so we can change 'll'
	// in order to break out of the loop.
	llo = ll;
	// break out of the loop
	ll = fgData[LON].size;
      }
    }

    // now copy a hyperslab (dimension slice at this longitude) of data from
    // pVal[] into fgData[LON].val[]
    for (i = 0; i < fgData[FRTIME].size; i++) {
      for (j = 0; j < fgData[LEVEL].size; j++) {
	for (k = 0; k < fgData[LAT].size; k++) {
	  (*this) (i, j, k, l) = pVal[this->offset (i, j, k, llo)];
	}
      }
    }

  }  // end loop over fgData[LON].val[]

  delete[] pLon;
  delete[] pVal;

  return FG_OK;
}

/////////////////////////////////////////////////////////////////////
// SORT THE FRTIME DIMENSION:
// For some reason the frtime dimension as given by Unidata
// is not in any order.
// I fix that here.
/////////////////////////////////////////////////////////////////////
// deprecated with current netcdf_io routine that reads in records
// sequentially
/////////////////////////////////////////////////////////////////////
// int Grid::uni_sort_frtime ()
// {
// 
//   unsigned int i, j, k;
// 
//   // MAKE A TEMP COPY OF FRTIME:
//   float *pTime = new float[fgData[FRTIME].size];
//   if (!pTime) {
//     uniErrorStrm << "uni_sort_frtime(): new failed." << std::endl;
//     uni_error ();
//     return FG_ERROR;
//   }
// 
//   for (i = 0; i < fgData[FRTIME].size; i++) {
//     pTime[i] = fgData[FRTIME].val[i];
//   }
// 
//   // MAKE A TEMP COPY OF THE DATA VAL:
//   float *pVal = new float[fgData[VAR].size];
//   if (!pVal) {
//     uniErrorStrm << "uni_sort_frtime() : new failed." << std::endl;
//     uni_error ();
//     return FG_ERROR;
//   }
// 
//   for (i = 0; i < fgData[VAR].size; i++) {
//     pVal[i] = fgData[VAR].val[i];
//   }
// 
//   // SORT THE DATA FRTIME:
//   shellsort (fgData[FRTIME].val, fgData[FRTIME].size);
// 
// 
//   unsigned nvol = fgData[LEVEL].size * fgData[LAT].size * fgData[LON].size;
//   float *pOld;
//   float *pNew;
// 
//   for (i = 0; i < fgData[FRTIME].size; i++) {
// 
//     pNew = fgData[VAR].val + i * nvol;
// 
//     for (j = 0; j < fgData[FRTIME].size; j++) {
//       if (fgData[FRTIME].val[i] == pTime[j]) {
// 
// 	pOld = pVal + j * nvol;
// 	j = fgData[FRTIME].size;
// 
//       }
//     }
// 
//     for (k = 0; k < nvol; k++) {
//       pNew[k] = pOld[k];
//     }
// 
//   }
// 
//   delete[] pTime;
//   delete[] pVal;
// 
//   return FG_OK;
// }

/////////////////////////////////////////////////////////////////////
//
// convert pressure heights from millibars to meters:
// sets warn value if interpolation is outside valid range
//
/////////////////////////////////////////////////////////////////////
int Grid::PtoH (Grid & H, int dz, int &iwarn)
{

  // set iwarn to false:
  iwarn = 0;

  // return if conversion is not necessary
  std::string units = fgData[LEVEL].units;
  if (units.find("meter") != std::string::npos) return FG_OK;
  
  // if there is no data, assume that is ok and return
  if (fgData[VAR].size == 0) return (FG_OK);
  
  // check sizes:
  if (H[LON].size != fgData[LON].size ||
      H[LAT].size != fgData[LAT].size ||
      H[LEVEL].size != fgData[LEVEL].size ||
      H[FRTIME].size != fgData[FRTIME].size) {
    std::cerr << "ERROR: PtoH() : Geopotential Height object is not the same size\n";
    return FG_ERROR;
  }

  // check reftime:
  if (strcmp (H.reftime (), fgReftime) != 0) {
    std::cerr <<
      "ERROR: PtoH() : Geopotential Height reference time is not the same\n";
    return FG_ERROR;
  }

  // check units:

  std::string valid_units (";geopotential meters;gp m;gpm;gpDekameters;");

//    if ( strstr(valid_units.str(), H[VAR].units) == NULL ) {
  if (valid_units.find (H[VAR].units) == std::string::npos) {
    uniErrorStrm << "PtoH() : Unknown geopotential units, expected: "
      << std::endl << valid_units << std::endl;
    uni_error ();
    return FG_ERROR;
  }

  // set scale to convert to meters:
  // default value assumes units are in meters:

  float scale = 1.0;

  if (strstr ("gpDekameters", H[VAR].units) != NULL) {
    scale = 10.0;
  }


  // MAX SIZE IS nz = fgData[LEVEL].size:
  const unsigned int nz = fgData[LEVEL].size;

  // MAXIMIZE DZ:
//  if (dz == 0 && nz != 1) {
//    dz = int (H.max () * scale / 1000) * 1000 / (nz - 1);

//    // Round to nearest 100:
//    float dzr = 100.0 * int (dz / 100.0);
//    if ((dz - dzr) > 50.0) {
//      dzr += 100.0;
//    }

//   dz = int (dzr);
//  }

  // create temporary arrays
  float *xa = new float[nz + 2];
  float *ya = new float[nz + 2];
  float *y2 = new float[nz + 2];
  // bail if these memory allocations failed
  if (!xa || !ya || !y2) {
    std::cerr << "ERROR: PtoH() : new failed\n";
    return FG_ERROR;
  }

	// the vertical levels we will interpolate to will be the average
	// geopotential heights at each pressure level.  
	// Set these new level values now, they are used in the spline()
	// function within the huge, nested loop below
	for (int i=0; i<fgData[LEVEL].size; i++)
	{
		fgData[LEVEL].val[i] = H.mean(LEVEL,i);
	}

  // for each grid location in lat, lon, and time, prepare an array of
  // points to which a spline function will be defined:
  unsigned int npts;
  unsigned int index;
  unsigned int i, j, k, l;
  for (l = 0; l < fgData[FRTIME].size; l++) {
    for (j = 0; j < fgData[LAT].size; j++) {
      for (i = 0; i < fgData[LON].size; i++) {
	// start with the 2nd value, there is a additional point at both ends
	// i.e. nz+2, that is set below
	npts = 1;
	for (k = 0; k < nz; k++) {
	  index = offset (l, k, j, i);
	  if (fgData[VAR].val[index] != fgFillValue ||
	      H[VAR].val[index] != H.fill_value ()) {
	    xa[npts] = scale * H[VAR].val[index];
	    ya[npts] = fgData[VAR].val[index];
	    npts++;
	  }
	}

	// Add points at both ends to extend the functions past
	// the sampling bounds. This is done by duplicating the
	// point values at each end of the array, thereby setting
	// a reasonable slope value.

	xa[0] = xa[1] - (xa[2] - xa[1]);
	ya[0] = ya[1] - (ya[2] - ya[1]);
	xa[npts] = xa[npts - 1] + (xa[npts - 1] - xa[npts - 2]);
	ya[npts] = ya[npts - 1] + (ya[npts - 1] - ya[npts - 2]);

	npts = npts - 1;

	// get second derivative:
	spline (xa, ya, npts, 1.e30, 1.e30, y2);

	// spline for values:
	float x, y;
	unsigned int iz, il;
//	for (iz = il = 0; iz < nz; iz++, il = il + dz) {
	for (iz = il = 0; iz < nz; iz++) {
//	  x = (float)( (-7400)*log(fgData[LEVEL].val[iz]/1000.0) );
	  x = (float)fgData[LEVEL].val[iz];
	  splint (xa, ya, y2, npts, x, y);
	  if (y < fgData[VAR].range[0] || y > fgData[VAR].range[1]) {
	    iwarn = 1;
	  }
	  index = offset (l, iz, j, i);
	  // if 'y' is NaN, die
	  if ( !(y <= 0) && !(y >= 0) )
	  {
	    std::cerr << "\nERROR: PtoH failed to interpolate a value\n";
	    exit(0);
	  }
	  fgData[VAR].val[index] = y;
//	  fgData[LEVEL].val[iz] = float (il);
	}
      } // i
    } // j
  } // l
  
  // change the vertical dimension values
//  for (unsigned int i = 0; i<fgData[LEVEL].size; i++) 
//  {
//    fgData[LEVEL].val[i] = (-7400)*log(fgData[LEVEL].val[i]/1000.0);
//  }

  // finish up:
  strcpy (fgData[LEVEL].units, "meters");
  delete[]xa;
  delete[]ya;
  delete[]y2;
  return FG_OK;
}
/////////////////////////////////////////////////////////////////////
//  create a Grid object and populate it with pressure data.  It is pretty
// mundane at this point because the vertical coordinate is pressure units,
// so every value will simply be the vertical coordinate.  However, using 
// PtoH() with geopotential data will interpolate to a vertical scale in 
// meters.  This is a backwards way to get a pressure Grid!
/////////////////////////////////////////////////////////////////////
void Grid::pressureGridFromZ(Grid &Z)
{
  int idx;
  
  // make it the same size of geopotential data Z
  fgNdims = 4;
  allocate(Z.fgData[FRTIME].size,
           Z.fgData[LEVEL].size,
           Z.fgData[LAT].size,
           Z.fgData[LON].size);
	   
  
  // copy other necessary attributes
  strcpy(fgReftime, Z.reftime() );
  
  // copy the dimension data
  for (unsigned int i = 0; i < fgData[LON].size; i++)
    fgData[LON].val[i] = Z.fgData[LON].val[i];
  for (unsigned int i = 0; i < fgData[LAT].size; i++)
    fgData[LAT].val[i] = Z.fgData[LAT].val[i];
  for (unsigned int i = 0; i < fgData[LEVEL].size; i++)
    fgData[LEVEL].val[i] = Z.fgData[LEVEL].val[i];
  for (unsigned int i = 0; i < fgData[FRTIME].size; i++)
    fgData[FRTIME].val[i] = Z.fgData[FRTIME].val[i];

  for (unsigned int i = 0; i < fgData[LON].size; i++) {
    for (unsigned int j = 0; j < fgData[LAT].size; j++) {
      for (unsigned int k = 0; k < fgData[LEVEL].size; k++) {
        for (unsigned int l = 0; l < fgData[FRTIME].size; l++) {  
          idx = offset(l, k, j, i);
	  fgData[VAR].val[idx] = Z.fgData[LEVEL].val[k];
        }
      }
    }
  }

  // give the variable a name
  strcpy(fgData[VAR].name, "pressure");
  // give the variable units
  if  ( (strncmp(Z.fgData[LEVEL].units,"hectopascal",11) == 0 ) || 
        (strncmp(Z.fgData[LEVEL].units,"millibar",8) == 0 ) )
  {
    strcpy(fgData[VAR].units, "millibars");  
  } else {
    strcpy(fgData[VAR].units, "unknown");
  }
  
  return;
}

/////////////////////////////////////////////////////////////////////
//
// convert pressure heights from millibars to meters using simple
// approximation
//
// use of 'round' is misleading, it takes the "floor" value (i.e. 1.99->1)
/////////////////////////////////////////////////////////////////////
int Grid::PtoH (float P0, float Hconst, float round)
{

  unsigned i;
  for (i = 0; i < fgData[LEVEL].size; i++) {

    fgData[LEVEL].val[i] =
      round * int (-Hconst * log (fgData[LEVEL].val[i] / P0) / round);

  }

  strcpy (fgData[LEVEL].units, "meters");

  return FG_OK;
}
