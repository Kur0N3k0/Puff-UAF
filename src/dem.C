/****************************************************************************
    puff - a volcanic ash tracking model
    Copyright (C) 2001-2006 Rorik Peterson <rorik@gi.alaska.edu>

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
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib> 
#include <cstdio>
#include <cmath>
#include <sys/types.h> // DIR
#include <dirent.h>
#include "dem.h"


//////////////////////////////////
Dem::~Dem(){
  if (path) delete [] path;
	if (tile) {
		for (int i = 0; i< ntiles; i++) {
			if (tile[i].data) delete[] tile[i].data;
			if (tile[i].name) free(tile[i].name);
		}
	}
  if (tile) free(tile);

  return;
  }
//////////////////////////////////
Dem::Dem() {
  
  initialized = false;
  tile = NULL;
  tileArray = NULL;
  ntiles = 0;
  path = new char[256];
  for (int i=0; i<256; i++) path[i]='\0';
  return;
  }
//////////////////////////////////
int Dem::setPath(const char* inPath) {
  strncpy(path, inPath, 255);
  // if path not specified, try environment variable
  if (strcmp(path,"") == 0 ) {
    if (getenv("PUFF_DEM_FILES")) 
      strcpy(path, getenv("PUFF_DEM_FILES"));
    }
  // if still not defined, set a default
  if (strcmp(path,"") == 0 )
    strcpy(path, "/usr/local/share/puff/GTOPO30");

  // append a trailing slash in necessary
  if (path[strlen(path)-1] != '/') strcat(path,"/");  

  return DEM_OK;
  }
//////////////////////////////////
int Dem::initialize(const char *type) {
  if (strncmp(type, "gtopo30",7) == 0 )  return setGtopo30();
  if (strncmp(type, "dted0",5) == 0 ) return setDted0();
	if (strncmp(type, "ned",3) == 0 ) return setNed();
  return DEM_ERROR;
  }
//////////////////////////////////
// return the elevation.  If the necessary tile does not exist, return 0
// without any warning.  Use a bilinear interpolation

double Dem::elevation(double lat, double lon, maparam* proj_grid) 
{

  double elev = 0;
  double e[4], t, u;
  
  if (! initialized) return 0;
	if (proj_grid)
	{
		double x, y;
		// convert x,y to lat,lon
		cxy2ll(proj_grid, lon, lat, &y, &x);
		lon = x;
		lat = y;
	}

  
  while (lon < -0.0 ) { lon += 360.; } 
  while (lon > 360.0) { lon -= 360.; }
  if (lat > 90 || lat < -90) {
    std::cerr << "ERROR: bad latitude value when finding elevation: " << lat << std::endl;
    return 0;
    }
  int idx = tileNumber(lat,lon);
  if (idx < 0 || idx > (ntiles-1) ) return 0;
  
  static const int xsize = tile[idx].ncols;
  static const int ysize = tile[idx].nrows;
  
  // load tile if necessary
  if (! tile[idx].loaded) readTile(idx);
  
  // get the lower-left coordinates - 'i' to the right, 'j' down
  int  i = (int)floor( (lon - tile[idx].minLon)/(double)tile[idx].dx ) ;
  int  j = (int)ceil ( (tile[idx].maxLat - lat)/(double)tile[idx].dy ) ;
	// 'i' index can be negative near meridian because tile may span meridian
	// so minLon < 360 but maxLon > 360.  Puff's values are 0 <= lon <= 360,
	// so get a new index using a shifted lon value.
	if (i < 0) 
  	i = (int)floor( (lon+360 - tile[idx].minLon)/(double)tile[idx].dx ) ;

  if (i > xsize || j > ysize) {
    std::cerr << "DEM error finding lat,lon coordinates\n";
    return DEM_ERROR;
    }
  // the four corner values 
  // ^   e3  e2
  // | 
  // N   e0  e1  East -->
  
  // GTOPO30 data starts in the northwest and reads westward
  if ((type == GTOPO30) or (type == NED)) {
		if (j==ysize) j--; // no data on the tile's border! fixme
		if (i==xsize-1) i--; // no data on the tile's border! fixme
    e[0] = tile[idx].data[j*xsize+i];
    e[1] = tile[idx].data[j*xsize+i+1];
    e[2] = tile[idx].data[(j-1)*xsize+i+1];
    e[3] = tile[idx].data[(j-1)*xsize+i];
    }
    
  // DTED0 data starts in the southwest and reads northward row by row
  if (type == DTED0) {
    e[0] = tile[idx].data[(ysize-j)*xsize+i];
    e[1] = tile[idx].data[(ysize-j)*xsize+i+1];
    e[2] = tile[idx].data[(ysize-j+1)*xsize+i+1];
    e[3] = tile[idx].data[(ysize-j+1)*xsize+i];
    }
    
  // set no data values to zero
  for (int i =0; i<4; i++) { if ( e[i] == no_data_value) e[i]=0;}
  
  // measure of how close 'lon' is to left boundary; zero is close
  t = ( lon-(i*tile[idx].dx+tile[idx].minLon) )/tile[idx].dx;
  // measure of how close 'lat' is to lower boundary; zero is close
  u = ( j*tile[idx].dy-(tile[idx].maxLat - lat ))/tile[idx].dy;
	// near the meridian, 't' value is wrong, see comments about index 'i' above.
 	if (t < 0 || t > 1) 
  	t = ( lon+360-(i*tile[idx].dx+tile[idx].minLon) )/tile[idx].dx;
  // bilinear interpolation
  elev = (1-t)*(1-u)*e[0]+t*(1-u)*e[1]+t*u*e[2]+(1-t)*u*e[3];
  
  return elev;
}
  
//////////////////////////////////
// return the tile number for this lat/lon pair.  For speed, first try the last
// index number that was returned. Next, go through the array of indicies for
// tiles that have been read already.  Finally, sequentially go through tiles 
// that exist on the filesystem (as determined when reading the headers during
// initialization). 

int Dem::tileNumber(double lat, double lon) {
  int i;
  static int idx = -1;
  double minLat, maxLon;  // create these values for making the comparison
  
  while (lon < 0  ) { lon += 360.; } 
  while (lon > 360) { lon -= 360.; }
  if (lat > 90 || lat < -90) return DEM_ERROR;

  // try last index value (if defined) first for speed
		if (idx != -1) {
	    minLat = tile[idx].maxLat - tile[idx].nrows*tile[idx].dy;
	    maxLon = tile[idx].minLon + tile[idx].ncols*tile[idx].dx;
	    if (minLat < lat && tile[idx].maxLat > lat &&
 	     tile[idx].minLon < lon && maxLon > lon ) return idx;
		}
      
  // try this algorithm for DTED0 and NED
  if ((type == DTED0) or (type == NED)) {
    for (int j=0; j<numTilesRead; j++) {
      idx=tileArray[j];
      minLat = tile[idx].maxLat - tile[idx].nrows*tile[idx].dy;
      maxLon = tile[idx].minLon + tile[idx].ncols*tile[idx].dx;
      if (minLat < lat && tile[idx].maxLat > lat &&
        tile[idx].minLon < lon && maxLon > lon ) return idx;
      }
    }    
  
  idx = -1;	  
  
	// brute force method
  for (i=0; i<ntiles; i++) {
    if (tile[i].exists) {
      minLat = tile[i].maxLat - tile[i].nrows*tile[i].dy;
			// at meridian, this gives values > 360
      maxLon = tile[i].minLon + tile[i].ncols*tile[i].dx;
      if (minLat < lat && tile[i].maxLat > lat &&
          tile[i].minLon < lon && maxLon > lon ) {
	   idx = i;
	   i = ntiles;  // break out of loop
	   };
			// above fails near meridian, try another test by adding 360
			// to 'lon'
			if (maxLon >= 360) {
      	if (minLat < lat && tile[i].maxLat > lat &&
            tile[i].minLon < lon+360 && maxLon > lon+360 ) {
	   		idx = i;
	   		i = ntiles;  // break out of loop
				}
			}
    }
  }

	// add this index value to the list of possible
  if ((type == DTED0 or type == NED) and idx != -1) {
    tileArray[numTilesRead]=idx;
    numTilesRead++;
    }
    
  return idx;
}
  
//////////////////////////////////
// Read the tile numbered 'idx'.  If it has already been read, or does not
// exist, return without error.  Data space is allocated and data read
// sequentially, with different protocols for diffent DEMs.  If the data is
// big-endian, do the byte swaps.  Resolution factor is used to skip
// unnecessary data.  If there is too little/much data, give warning

int Dem::readTile(int idx) {
  std::string filename;
  int didx = 0;	// data index
  short int ndata;
  static const int skip = resolution_factor;

  if (tile[idx].loaded) return DEM_OK;
  if (!tile[idx].exists) return DEM_OK;
  // write the message to stdout, and clear it when done
  std::string msg = "Reading DEM tile ";
  msg.append(tile[idx].name);
 std:: cout << msg << std::flush;
  
  // allocate space for data
  tile[idx].data = new double[tile[idx].ncols*tile[idx].nrows];
      
  // open data file
  filename=path;
  filename.append(tile[idx].name);
  std::ifstream demFile(filename.data(), std::ios::in);
  if (!demFile) {
    std::cerr << "\nfailed to open DEM file " << filename.data() << std::endl;
    tile[idx].exists = false;
    return DEM_ERROR;
    }

  // GTOPO30 data starts in the northwest and reads westward  
  if (type == GTOPO30) {    
    for (int j=0; j<tile[idx].nrows; j++) {
      for (int i=0; i<tile[idx].ncols; i++) {
        demFile.read(reinterpret_cast<char*>(&ndata), sizeof(short int));
        // skip forward for lower resolution reading
        demFile.seekg((skip-1)*sizeof(short int), std::ios::cur);
        if (demFile.eof() ) {
          std::cerr << "\nERROR: end-of-file reached prematurely\n";
	  // this is a fatal error
	  exit(0);
          }
        // swap data if not bigendian
#ifndef WORDS_BIGENDIAN
        ndata = (((ndata & 0x00ff) << 8) | ((ndata & 0xff00) >> 8));
#endif
        tile[idx].data[didx]=ndata;
        didx++;
      }
      // skip entire rows - ncols*skip is actual number of data values in a row
      //                  - (skip-1) is the number of rows to skip over
      demFile.seekg((skip-1)*tile[idx].ncols*skip*sizeof(short int), std::ios::cur);
    }
  } // end GTOPO30 read
  
  // DTED0 data starts in the southwest and reads northward
  if (type == DTED0) {
  // skip the 3430 characters in the header
  demFile.seekg(3430, std::ios::beg);
  for (int j=0; j<tile[idx].ncols; j++) {
    // skip three junk values at the beginning of each column
    demFile.seekg(3*sizeof(short int), std::ios::cur);
    for (int i=0; i<tile[idx].nrows; i++) {
      demFile.read(reinterpret_cast<char*>(&ndata), sizeof(short int));
      // skip forward for lower resolution reading
      demFile.seekg((skip-1)*sizeof(short int), std::ios::cur);
      if (demFile.eof() ) {
        std::cerr << "\nERROR: end-of-file reached prematurely\n";
	// this is a fatal error
	exit(0);
        }
        // swap data if not bigendian
#ifndef WORDS_BIGENDIAN
        ndata = (((ndata & 0x00ff) << 8) | ((ndata & 0xff00) >> 8));
#endif
        tile[idx].data[didx]=ndata;
        didx++;
      }
      // skip overlapping data value at end of column
//      demFile.seekg(1*sizeof(short int), std::ios::cur);
      // skip three junk values at the END of each column
      demFile.seekg(3*sizeof(short int), std::ios::cur);
      // skip entire cols - (nrows*skip)+6+1 is actual number of data values in
      //                   a single col plus 3 junk values at each end 
      //                   plus the overlapping value we are skipping
      //                  - (skip-1) is the number of cols to skip over
      demFile.seekg((skip-1)*(tile[idx].nrows*skip+7)*sizeof(short int), std::ios::cur);
    }
    // there should be one column of data left, which is nrows*skip plus the
    // overlapping value in this column plus the three junk data points at 
    // each end.
    demFile.seekg((tile[idx].nrows*skip+7)*sizeof(short int), std::ios::cur);
    
  } // end read DTED0
  
	// NED data starts in southwest and reads northward
	if (type == NED) {
		// NED data in gridfloat is 32-bit float value
		float fdata = no_data_value;
		for (int jrow = 0; jrow < tile[idx].nrows; jrow++) {
			for (int icol = 0; icol < tile[idx].ncols; icol++) {
				demFile.read(reinterpret_cast<char*>(&fdata), sizeof(float));
				// fixme: byte swapping here
				tile[idx].data[didx++] = fdata;
			}
		}
	} // end read NED

  //DEBUG - test that the end of file was reached
  demFile.read(reinterpret_cast<char*>(&ndata), sizeof(short int));
  if (! demFile.eof() ) {
    // this is a fatal error
    std::cerr << "\nERROR: entire DEM file " << tile[idx].name << " was not read\n";
    int count = 0;
    char junkData;
    while (! demFile.eof() ) {
      demFile.read(&junkData, 1);
      count++;
      }
    std::cerr << count << " bytes remained\n";
    exit(0);
    }

  tile[idx].loaded = true;
  //  clear the "Reading DEM tile ... " message
  for (unsigned int i = 0; i<msg.length(); i++) { std::cout << '\b'; }
  for (unsigned int i = 0; i<msg.length(); i++) { std::cout << " "; }
  for (unsigned int i = 0; i<msg.length(); i++) { std::cout << '\b'; }
  std::cout << std::flush;
  
  return DEM_OK;
  }
//////////////////////////////////
// 
int Dem::setResolution(int res) {

  bool failed = false;  // flag for setting the resolution
  int nrowsLocal, ncolsLocal;  // local copy for printing error message
  if (res < 0) return DEM_ERROR;
	// only use full resolution with dted0 because tile border values
	// overlap and nrows/ncols are uneven.  Things run pretty quick with 
	// dted0 anyway because the tiles are small, so no need for reduced
	// resolution probably.
	if (type == DTED0 and res != 0) {
		std::cout << "DTED0 only runs at full resolution\n";
		res = 0;
		}

  static const int factor = (int)pow((double)2,(double)res);
  for (int idx = 0 ; idx < ntiles ; idx++ ) {
    if (tile[idx].exists &&
       (tile[idx].nrows%factor != 0 || tile[idx].ncols%factor != 0) ) {
          nrowsLocal = tile[idx].nrows;
	  ncolsLocal = tile[idx].ncols;       
	  failed = true;
	  idx = ntiles;  // break out since one failed
	  }
      }
  if (failed) {
    std::cerr << "ERROR: failed to set DEM resolution to level " << res << std::endl;
    std::cerr << "number of rows per tile: " << nrowsLocal << std::endl;
    std::cerr << "number of columns per tile: " << ncolsLocal << std::endl;
    std::cerr << "Both values must be evenly divided by 2^" << res << " = ";
    std::cerr << factor  << std::endl;
    return DEM_ERROR;
    }
  for (int idx = 0; idx < ntiles; idx++) {
    tile[idx].nrows/=factor;
    tile[idx].ncols/=factor;
    tile[idx].dy*=factor;
    tile[idx].dx*=factor;
    }
    
  resolution_factor = factor;
  return DEM_OK;
  }
  
//////////////////////////////////
int Dem::readGtopo30Header(int idx) {

  std::string filename = path;
  filename.append(tile[idx].name);
  // create the header filename from the DEM filename
  filename.replace(filename.find(".DEM"),4,".HDR");
  std::ifstream file(filename.data(), std::ios::in);
  if (!file) return DEM_ERROR;
  
  std::string text;
  while (! file.eof() ) {
   file >> text;
    if (text.compare("NROWS") == 0 ) {
      file >> text;
      if (sscanf(text.data(),"%i",&tile[idx].nrows) != 1)
        std::cerr << "failed to assign NROWS from " << filename << std::endl;
    } else if (text.compare("NCOLS") == 0 ) {
      file >> text;
      if (sscanf(text.data(),"%i",&tile[idx].ncols) != 1)
        std::cerr << "failed to assign NCOLS from " << filename << std::endl;
    } else if (text.compare("XDIM") == 0 ) {
      file >> text;
      if (sscanf(text.data(),"%lf",&tile[idx].dx) != 1)
        std::cerr << "failed to assign XDIM from " << filename << std::endl;
    } else if (text.compare("YDIM") == 0 ) {
      file >> text;
      if (sscanf(text.data(),"%lf",&tile[idx].dy) != 1)
        std::cerr << "failed to assign YDIM from " << filename << std::endl;
    } else if (text.compare("NODATA") == 0 ) {
      file >> text;
      if (sscanf(text.data(),"%i",&no_data_value) != 1)
        std::cerr << "failed to assign NODATA from " << filename << std::endl;
      no_data_value = strtol(text.data(),(char**)NULL,10);
    } else if (text.compare("ULXMAP") == 0) {
      file >> text;
      if (sscanf(text.data(),"%lf",&tile[idx].minLon) != 1)
        std::cerr << "failed to assign ULXMAP from " << filename << std::endl;
      while(tile[idx].minLon < 0) { tile[idx].minLon+=360.0;}
    } else if (text.compare("ULYMAP") == 0) {
      file >> text;
      if (sscanf(text.data(),"%lf",&tile[idx].maxLat) != 1)
        std::cerr << "failed to assign ULYMAP from " << filename << std::endl;
    }
     
  }
  file.close();
  return DEM_OK;
  }
  
//////////////////////////////////
// create file names and set number of tiles for GTOPO30.  If your filenames
// differ, you'll need to modify these values.
int Dem::setGtopo30() {
  ntiles = 33;
  //tile = new Tile[ntiles];
	// use malloc() instead of new[] to be consistent with dted0's allocation
	// which uses realloc() to add each new tile.  Consistency is necessary
	// for freeing memory in the destructor
	tile=(Tile*)malloc(sizeof(Tile)*ntiles);
  for (int i=0; i<ntiles;i++) {
		tile[i].loaded = false;
		tile[i].data = NULL;
	}
  
  tile[0].name = strdup("W180N90.DEM");
  tile[1].name = strdup("W140N90.DEM");
  tile[2].name = strdup("W100N90.DEM");
  tile[3].name = strdup("W060N90.DEM");
  tile[4].name = strdup("W020N90.DEM");
  tile[5].name = strdup("E020N90.DEM");
  tile[6].name = strdup("E060N90.DEM");
  tile[7].name = strdup("E100N90.DEM");
  tile[8].name = strdup("E140N90.DEM");
  tile[9].name = strdup("W180N40.DEM");
  tile[10].name = strdup("W140N40.DEM");
  tile[11].name = strdup("W100N40.DEM");
  tile[12].name = strdup("W060N40.DEM");
  tile[13].name = strdup("W020N40.DEM");
  tile[14].name = strdup("E020N40.DEM");
  tile[15].name = strdup("E060N40.DEM");
  tile[16].name = strdup("E100N40.DEM");
  tile[17].name = strdup("E140N40.DEM");
  tile[18].name = strdup("W180S10.DEM");
  tile[19].name = strdup("W140S10.DEM");
  tile[20].name = strdup("W100S10.DEM");
  tile[21].name = strdup("W060S10.DEM");
  tile[22].name = strdup("W020S10.DEM");
  tile[23].name = strdup("E020S10.DEM");
  tile[24].name = strdup("E060S10.DEM");
  tile[25].name = strdup("E100S10.DEM");
  tile[26].name = strdup("E140S10.DEM");
  tile[27].name = strdup("W180S60.DEM");
  tile[28].name = strdup("W120S60.DEM");
  tile[29].name = strdup("W060S60.DEM");
  tile[30].name = strdup("W000S60.DEM");
  tile[31].name = strdup("E060S60.DEM");
  tile[32].name = strdup("E120S60.DEM");

  // initialize the tiles that exist
  for (int i=0;i<ntiles;i++) {
    if (readGtopo30Header(i) == DEM_OK) {
      tile[i].exists = true;
    } else {
      tile[i].exists = false;
      }
    }
  no_data_value = -9999;
  
  initialized = true; 
  type = GTOPO30;
   
  return DEM_OK;
  }
//////////////////////////////////
// read a DTED0 header
// these headers are a mess, or I just don't understand them.  They are 80
// characters and look something like this for w160/n60.dt0
// UHL11600000W0650000N060003000042U              006101210 
// In the above example header, it reads like this:
// There are 4 characters, 160 degrees west, 65 degrees north, 60-arcsecond
// resolution for longitude, 30-arcsecond resolution for latitude, and then
// some unknown junk.  After the spaces are the grid size, which is 61x121

int Dem::readDted0Header(int idx) {
  
  static const int seconds_per_degree = 3600;
  double minLat;  // temporarily hold minimum latitude, and get a maxLat later
  std::string filename = path;
  filename.append(tile[idx].name);

  std::ifstream file(filename.data(), std::ios::in);
  if (!file) return DEM_ERROR;

  int num;  // temporarily hold integer values converted from the header
  char header[3];  // temporarily hold header characters
	strcpy(header,"***"); // initialize this to anything for sscanf() later
 
  // 20-22 are arc-second resolution for longitude
  // 24-26 are arc-second resolution for latitude

  // skip to index 4 and read 3 chars for minimum longitude 
  file.seekg(4, std::ios::beg);
  file.read(header,3*sizeof(char));
  if (sscanf(header,"%d",&num) != 1) {
    std::cerr << "ERROR: did not convert " << header << "into an integer value\n";
  } else {
    tile[idx].minLon = num;
  }
  
  // skip to 11 and read 1 char for longitude direction
  file.seekg(11, std::ios::beg);
  file.read(header,1*sizeof(char));
  if (strncmp(header,"W",1) == 0) tile[idx].minLon = 360.0-tile[idx].minLon;
  
  // skip to index 12 and read 3 chars for minimum latitude
  file.seekg(12, std::ios::beg);
  file.read(header,3*sizeof(char));
  if (sscanf(header,"%d",&num) != 1) {
    std::cerr << "ERROR: did not convert " << header << "into an integer value\n";
  } else {
    minLat = num;
  }
  
  // skip to index 19 and read 1 char for latitude direction
  file.seekg(19, std::ios::beg);
  file.read(header,1*sizeof(char));
  if (strncmp(header,"S",1) == 0) minLat = (-1)*minLat;
  
  // skip to index 20 and read 3 chars for longitude resolution in arcseconds
  file.seekg(20, std::ios::beg);
  file.read(header,3*sizeof(char));
  if (sscanf(header,"%d",&num) != 1) {
    std::cerr << "ERROR: did not convert " << header << "into an integer value\n";
  } else {
    tile[idx].dx = (static_cast<double>(num)/seconds_per_degree);
  }
  
  // skip to index 25 and read 3 chars for latitude resolution in arcseconds
  file.seekg(24, std::ios::beg);
  file.read(header,3*sizeof(char));
  if (sscanf(header,"%d",&num) != 1) {
    std::cerr << "ERROR: did not convert " << header << "into an integer value\n";
  } else {
    tile[idx].dy = (static_cast<double>(num)/seconds_per_degree);
  }
  
  // skip to index 48 and read 3 chars for size in longitude direction
  file.seekg(48, std::ios::beg);
  file.read(header,3*sizeof(char));
  if (sscanf(header,"%d",&num) != 1) {
    std::cerr << "ERROR: did not convert " << header << "into an integer value\n";
  } else {
    tile[idx].ncols = num;
  }
  
  // skip to index 52 and read 3 chars for size in longitude direction
  file.seekg(52, std::ios::beg);
  file.read(header,3*sizeof(char));
  if (sscanf(header,"%d",&num) != 1) {
    std::cerr << "ERROR: did not convert " << header << "into an integer value\n";
  } else {
    tile[idx].nrows = num;
  }
  
  // now convert minLat into a maxLat since that the is the standard for
  // this object
  tile[idx].maxLat = minLat + (tile[idx].nrows-1)*tile[idx].dy;
  
  // to allow for changing resolution, the xsize and ysize need to be even,
  // at least initially, so forget the last set of points, which overlap the
  // next tile
// DEBUG:
// can we not do this since overlap is needed for interpolating elevations?
//  if (tile[idx].nrows != 0 && tile[idx].nrows%2 == 1) tile[idx].nrows--;
//  if (tile[idx].ncols != 0 && tile[idx].ncols%2 == 1) tile[idx].ncols--;
  
  return DEM_OK;
  }
//////////////////////////////////
// National Elevation Dataset downloaded from the Seamless Data 
// Distribution System (SDDS) at http://seamless.usgs.gov.
// Files have random numerical names based on the data request, for example
// 9993428.zip.  Inside is a header file *.hdr and the data in *.flt for 
// GridFloat data.  It also comes in other formats that could possibly be
// supported.  Assume all directories in the path are possible tiles, name
// them after their number (although meaningless) and read their header.
// The ASCII header is something like this
// ncols         7277
// nrows         3039
// xllcorner     -151.80833333332
// yllcorner     60.445555555564
// cellsize      0.000555555555556
// NODATA_value  -9999
// byteorder     LSBFIRST
// 
int Dem::setNed()
{
	DIR *dp;	// pointer to directory entries
	// attempt to open directory with NED dem data
	dp = opendir(path);
	if (!dp) {
		std::cerr << "ERROR: Failed to open NED directory " << path << std::endl;
		return DEM_ERROR;
		}

	struct dirent *ep;  // pointer to directory entry	
	// loop through all entries, looking for header files
	while ((ep = readdir(dp) )) {
		std::string possibleDir = ep->d_name;
		// don't do hidden files, this, or the parent dir
		if (strncmp(ep->d_name, ".", 1) == 0) continue;
		// look for header inside this directory
		DIR *ddp; // pointer to subdirectory entry
		// make this into full path name, trailing / already there from setPath()
		std::string possibleDirFull = path + possibleDir;
		ddp = opendir(possibleDirFull.c_str());
		struct dirent *eep; // all subdirectory entries
		while ((eep = readdir(ddp) )) {
			std::string possibleFlt = eep->d_name; // possible .flt file
			// see if the name matches
			if (possibleFlt.find(".flt") != std::string::npos) {
				addTile();
				// name tile, include subdir, but not path
				possibleFlt = possibleDir + "/" + possibleFlt;
				tile[ntiles-1].name = strdup(possibleFlt.c_str()); 
			  if (readNedHeader(ntiles-1) == DEM_OK) {
					tile[ntiles-1].exists = true;
					}
				}
			} 
			(void)closedir(ddp);

		}
	(void)closedir(dp);
  tileArray = new unsigned int[ntiles];
  numTilesRead = 0;
  initialized = true;
	type = NED;

	return DEM_OK;
}
			
//////////////////////////////////
void Dem::addTile()
{
	tile = (Tile*)realloc(tile,sizeof(Tile)*(ntiles+1));
	tile[ntiles].name = NULL;
	tile[ntiles].data = NULL;
	tile[ntiles].loaded = false;
	ntiles++;
	return;
}	
//////////////////////////////////
// 
// The ASCII header is something like this
// ncols         7277
// nrows         3039
// xllcorner     -151.80833333332
// yllcorner     60.445555555564
// cellsize      0.000555555555556
// NODATA_value  -9999
// byteorder     LSBFIRST
//
// reading is done similar to GTOPO30 headers
int Dem::readNedHeader(int idx)
{
  std::string filename = path;
  filename.append(tile[idx].name);
  // create the header filename from the DEM filename
  filename.replace(filename.find(".flt"),4,".hdr");
  std::ifstream file(filename.data(), std::ios::in);
  if (!file) return DEM_ERROR;
  
  std::string text;
  while (! file.eof() ) {
   file >> text;
    if (text.compare("ncols") == 0 ) {
      file >> text;
      if (sscanf(text.data(),"%i",&tile[idx].ncols) != 1)
        std::cerr << "failed to assign ncols from " << filename << std::endl;
    } else if (text.compare("nrows") == 0 ) {
      file >> text;
      if (sscanf(text.data(),"%i",&tile[idx].nrows) != 1)
        std::cerr << "failed to assign nrows from " << filename << std::endl;
    } else if (text.compare("cellsize") == 0 ) {
      file >> text;
      if (sscanf(text.data(),"%lf",&tile[idx].dx) != 1)
        std::cerr << "failed to assign cellsize from " << filename << std::endl;
			tile[idx].dy = tile[idx].dx;
    } else if (text.compare("NODATA_value") == 0 ) {
      file >> text;
      if (sscanf(text.data(),"%i",&no_data_value) != 1)
        std::cerr << "failed to assign NODATA_value from " << filename << std::endl;
      no_data_value = strtol(text.data(),(char**)NULL,10);
    } else if (text.compare("xllcorner") == 0) {
      file >> text;
      if (sscanf(text.data(),"%lf",&tile[idx].minLon) != 1)
        std::cerr << "failed to assign xllcorner from " << filename << std::endl;
      while(tile[idx].minLon < 0) { tile[idx].minLon+=360.0;}
    } else if (text.compare("yllcorner") == 0) {
      file >> text;
      if (sscanf(text.data(),"%lf",&tile[idx].maxLat) != 1)
        std::cerr << "failed to assign yllcorner from " << filename << std::endl;
    }
     
  }

	// need to maxLat because minLat was actually read in, but had to wait for
	// 'dy' to be read from cellsize
 	tile[idx].maxLat = tile[idx].maxLat + (tile[idx].nrows-1)*tile[idx].dy;

	// clean up the object a little
	tile[idx].data = NULL;
	tile[idx].loaded = false;
	// dem data file exists, that is how we got here from setNed()
	tile[idx].exists = true;  	

	file.close();
	return DEM_OK;
}
//////////////////////////////////
// this allocates space for the tile with addTile() only if the data file
// exists. No header is read, nor is data loaded.  This is used to create a 
// small list of possible tiles, contrary to GTOPO30 where all 32 tiles are 
// in the list.  There would be 1000's of possible tiles for DTED0, otherwise.
void Dem::dtedTileExists(char *n)
{
  std::string filename = path;
  filename.append(n);
	
  std::ifstream file(filename.data(), std::ios::in);
  if (!file) return;

	addTile();
	tile[ntiles-1].name = strdup(n);
	return;
}
//////////////////////////////////
// create file names and set number of tiles for DTED0.  If your filenames
// and/or structure differ, you'll need to modify this function.  It is 
// assumed the files are named things like w160/n63.dt0 or e002/s21.dt0
// This could be done like NED where any file called *.dt0 is allowed, but
// this was written beforehand in the image of GTOPO30, where we knew all
// the file names.  We do here as well, the list is just long. fixme??

int Dem::setDted0() {

  // tiles are 1-degree x 1-degree  
//  ntiles = 360*180;
//  tile = new Tile[ntiles];
//  int idx;  // tile index
  // allocate space for tile names
//  for(idx = 0; idx<ntiles; idx++) {
//    tile[idx].name = new char[13]; // i.e. w151/n34.dt0
//    tile[idx].loaded = false;
//    }
	char name[13];   // i.e. w151/n34.dt0
    
//  idx = 0;  // reset tile index to zero

  // create the DTED0 filenames based on geoengine.nima.mil names
  // filename corresponds to the lower-left corner of the grid
  // north files have zero index i.e. n00.dt0
  // south files have a 90 index i.e. s90.dt0
  // east directories have zero index i.e. e000/
  // west directories have 180 index i.e. w180/
  
  // north-east quadrant
  for (int lat_idx=0; lat_idx<90; lat_idx++) {
    for (int lon_idx=0; lon_idx<180; lon_idx++) {
      if (lat_idx < 10 && lon_idx < 10) { // e00x/n0x.dt0
        sprintf(name,"e00%1i/n0%1i.dt0",lon_idx,lat_idx);
      } else if (lat_idx < 10 && lon_idx < 100) { // e0xx/n0x.dt0
        sprintf(name,"e0%2i/n0%1i.dt0",lon_idx,lat_idx);
      } else if (lat_idx < 10 && lon_idx > 99)  {// exxx/n0x.dt0
        sprintf(name,"e%3i/n0%1i.dt0",lon_idx,lat_idx);
      } else if (lat_idx >= 10 && lon_idx < 10) { // e00x/nxx.dt0
        sprintf(name,"e00%1i/n%2i.dt0",lon_idx,lat_idx);
      } else if (lat_idx >= 10 && lon_idx < 100) { // e0xx/nxx.dt0
        sprintf(name,"e0%2i/n%2i.dt0",lon_idx,lat_idx);
      } else if (lat_idx >= 10 && lon_idx > 99) { // exxx/nxx.dt0
        sprintf(name,"e%3i/n%2i.dt0",lon_idx,lat_idx);
      }  else { 
        std::cerr << "ERROR: did not assign DTED tile at " << lon_idx << "," << lat_idx << std::endl;
      }
//      idx++; // advance tile index
			dtedTileExists(name);
      }
    }

  // north-west quadrant
  for (int lat_idx=0; lat_idx<90; lat_idx++) {
    for (int lon_idx=1; lon_idx<181; lon_idx++) {
      if (lat_idx < 10 && lon_idx < 10) { // w00x/n0x.dt0
        sprintf(name,"w00%1i/n0%1i.dt0",lon_idx,lat_idx);
      } else if (lat_idx < 10 && lon_idx < 100) { // w0xx/n0x.dt0
        sprintf(name,"w0%2i/n0%1i.dt0",lon_idx,lat_idx);
      } else if (lat_idx < 10 && lon_idx > 99)  {// wxxx/n0x.dt0
        sprintf(name,"w%3i/n0%1i.dt0",lon_idx,lat_idx);
      } else if (lat_idx >= 10 && lon_idx < 10) { // w00x/nxx.dt0
        sprintf(name,"w00%1i/n%2i.dt0",lon_idx,lat_idx);
      } else if (lat_idx >= 10 && lon_idx < 100) { // w0xx/nxx.dt0
        sprintf(name,"w0%2i/n%2i.dt0",lon_idx,lat_idx);
      } else if (lat_idx >= 10 && lon_idx > 99) { // wxxx/nxx.dt0
        sprintf(name,"w%3i/n%2i.dt0",lon_idx,lat_idx);
      }  else { 
        std::cerr << "ERROR: did not assign DTED tile at " << lon_idx << "," << lat_idx << std::endl;
      }
//      idx++; // advance tile index
			dtedTileExists(name);
      }
    }

  // south-east quadrant
  for (int lat_idx=1; lat_idx<91; lat_idx++) {
    for (int lon_idx=0; lon_idx<180; lon_idx++) {
      if (lat_idx < 10 && lon_idx < 10) { // e00x/s0x.dt0
        sprintf(name,"e00%1i/s0%1i.dt0",lon_idx,lat_idx);
      } else if (lat_idx < 10 && lon_idx < 100) { // e0xx/s0x.dt0
        sprintf(name,"e0%2i/s0%1i.dt0",lon_idx,lat_idx);
      } else if (lat_idx < 10 && lon_idx > 99)  {// exxx/s0x.dt0
        sprintf(name,"e%3i/s0%1i.dt0",lon_idx,lat_idx);
      } else if (lat_idx >= 10 && lon_idx < 10) { // e00x/sxx.dt0
        sprintf(name,"e00%1i/s%2i.dt0",lon_idx,lat_idx);
      } else if (lat_idx >= 10 && lon_idx < 100) { // e0xx/sxx.dt0
        sprintf(name,"e0%2i/s%2i.dt0",lon_idx,lat_idx);
      } else if (lat_idx >= 10 && lon_idx > 99) { // exxx/sxx.dt0
        sprintf(name,"e%3i/s%2i.dt0",lon_idx,lat_idx);
      }  else { 
        std::cerr << "ERROR: did not assign DTED tile at " << lon_idx << "," << lat_idx << std::endl;
      }
//      idx++; // advance tile index
			dtedTileExists(name);
      }
    }

  // south-west quadrant
  for (int lat_idx=1; lat_idx<91; lat_idx++) {
    for (int lon_idx=1; lon_idx<181; lon_idx++) {
      if (lat_idx < 10 && lon_idx < 10) { // w00x/n0x.dt0
        sprintf(name,"w00%1i/s0%1i.dt0",lon_idx,lat_idx);
      } else if (lat_idx < 10 && lon_idx < 100) { // w0xx/s0x.dt0
        sprintf(name,"w0%2i/s0%1i.dt0",lon_idx,lat_idx);
      } else if (lat_idx < 10 && lon_idx > 99)  {// wxxx/s0x.dt0
        sprintf(name,"w%3i/s0%1i.dt0",lon_idx,lat_idx);
      } else if (lat_idx >= 10 && lon_idx < 10) { // w00x/sxx.dt0
        sprintf(name,"w00%1i/s%2i.dt0",lon_idx,lat_idx);
      } else if (lat_idx >= 10 && lon_idx < 100) { // w0xx/sxx.dt0
        sprintf(name,"w0%2i/s%2i.dt0",lon_idx,lat_idx);
      } else if (lat_idx >= 10 && lon_idx > 99) { // wxxx/sxx.dt0
        sprintf(name,"w%3i/s%2i.dt0",lon_idx,lat_idx);
      }  else { 
        std::cerr << "ERROR: did not assign DTED tile at " << lon_idx << "," << lat_idx << std::endl;
      }
//      idx++; // advance tile index
			dtedTileExists(name);
      }
    }

//	ntiles = idx;

  // initialize the tiles that exist
  std::cout << "reading DTED0 headers ... " << std::flush;
  for (int idx = 0; idx < ntiles; idx++ ) {
    if (readDted0Header(idx) == DEM_OK) {
      tile[idx].exists = true;
    } else {
      tile[idx].exists = false;
      }
    }
  std::cout << "done" << std::endl;

  no_data_value = -9999;  // this is made  up right now
  type = DTED0;
  
  // this array holds the indexes of tiles already read into memory.  In 
  // member function tileNumber(), this array is searched first, which appears
  // to really speed things up
  tileArray = new unsigned int[ntiles];
  numTilesRead = 0;
  
  initialized = true;
  
  return DEM_OK;
  }
