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
#include "Grid.h"
#include <fstream>
#include <cstdio> // sscanf
#include <iostream>
#include <string>
#include <vector>

extern void Tokenize(const std::string&, std::vector<std::string>&, const std::string&);
////////////////////////////////////////////////////////////////////////////
// read ukmet office BADC stratospheric data in 'pp' format
// see http://badc.nerc.ac.uk/data/assim/assimhelp.html for more info
////////////////////////////////////////////////////////////////////////////
int Grid::read_pp(std::string *pp_file) {
  // temperature and geopotential height have data at the north pole, the
  // u,v,w winds do not.  Data is on a 2.5-degree lat x 3.75-degree lon grid
  // with 23 pressure levels - so each slab of wind is of size 
  // (180/2.5)*(360/3.75) = 6912, and temp & geo-height have an extra
  // (360/3.75) = 96 points, which is 7008.  There is a junk value at the end
  // of each record, so we add an extra for that and discard it later.

  static const int dataSize = 6912;

  // header data is 64 integer values, but there are three junk at the end
  // so add three for that

  static const int hdrSize = 67;
  
  // temporary data-holding union so byte-swapping can occur if necessary
  union {
    int i;
    float f;
    } tmpData;
  
  // tokenize the filename since there might be multiple files
  std::vector<std::string> fileList;
  Tokenize(*pp_file, fileList, ":");
  // fileList has second offsets in it, move that into another list
  std::vector<int> secOffsetList;  // list of offsets
  int secOffset;  // temporary holder for offset value
  std::vector<std::string>::iterator p;
  
  // add zero hour offset to list of offset times
  secOffsetList.push_back(0);
  
  for (p=fileList.begin(); p!=fileList.end(); ++p) {
    if (sscanf(&(*p->c_str()), "%i", &secOffset) == 1) {
      secOffsetList.push_back(secOffset); 
      fileList.erase(p);
      }
    }
  
  // set the dimensions statically since it is not possible to read that
  // information from the file. 
  fgNdims = 4;
  fgData[LAT].size = 72;
  fgData[LON].size = 96;
  fgData[LEVEL].size = 22;
  fgData[FRTIME].size = secOffsetList.size();
  
  allocate(fgData[FRTIME].size,
           fgData[LEVEL].size,
	   fgData[LAT].size,
	   fgData[LON].size);
	   
  // explicitely set the units
  if (strncmp(fgData[VAR].name, "Z", 1) == 0 )
  {
    strncpy(fgData[VAR].units, "gp m", 5);
  } else {
    strncpy(fgData[VAR].units, "m/s", 5);
  }
  
  // set the coordinate values; it would be better to read these from the
  // header in case they change, or the file is corrupted
  // set latitude values
  for (int i = 0; i < 72; i++) { fgData[LAT].val[i] = 87.5 - 2.5*i; }
  strcpy(fgData[LAT].units, "degrees_north");
  // set longitude values
  for (int i = 0; i < 96; i++) { fgData[LON].val[i] = 3.75*i; }
  strcpy(fgData[LON].units, "degrees_east");
  // set level values
  fgData[LEVEL].val[0] = 1000.0;  fgData[LEVEL].val[1] = 681.3;
  fgData[LEVEL].val[2] = 464.2;   fgData[LEVEL].val[3] = 316.2; 
  fgData[LEVEL].val[4] = 215.4;   fgData[LEVEL].val[5] = 146.8; 
  fgData[LEVEL].val[6] = 100.0;   fgData[LEVEL].val[7] = 68.123;
  fgData[LEVEL].val[8] = 46.42;   fgData[LEVEL].val[9] = 31.62;
  fgData[LEVEL].val[10] = 21.54;  fgData[LEVEL].val[11] = 14.68;
  fgData[LEVEL].val[12] = 10.0;   fgData[LEVEL].val[13] = 6.813;
  fgData[LEVEL].val[14] = 4.642;  fgData[LEVEL].val[15] = 3.162;
  fgData[LEVEL].val[16] = 2.154;  fgData[LEVEL].val[17] = 1.468;
  fgData[LEVEL].val[18] = 1.0;    fgData[LEVEL].val[19] = 0.681;
  fgData[LEVEL].val[20] = 0.464;  fgData[LEVEL].val[21] = 0.316;
  
  strcpy(fgData[LEVEL].units, "millibars");

// add offsets to the time vector
  int idx = 0;

  std::vector<int>::const_iterator pInt;
  for (pInt=secOffsetList.begin(); pInt!= secOffsetList.end(); ++pInt) {
    fgData[FRTIME].val[idx] = (*pInt)/3600.;
    idx++;
    }

  int fileIdx = 0;  // counter for number of files read
  
//  list<string>::const_iterator p;
  for (p=fileList.begin(); p!=fileList.end(); ++p) {
  
    // open an input file stream, or exit if it fails.
    std::ifstream ppFile( p->c_str(), std::ios::in);
    if (!ppFile) {
      std::cerr << "failed to open pp file " <<pp_file << std::endl;
      return FG_ERROR;
      }
  // set the reference time the first time through
  if (p == fileList.begin())
     strcpy(fgReftime,pp_reftime(&ppFile) );
   
    // data is sequential with a header between each slab of data.  The order is
    // u, v, geo-height, temperature, w.  
    
    int lvl_idx;  // level index 
  
    // read or skip the u wind values
    for (lvl_idx = 0; lvl_idx < (int)fgData[LEVEL].size; lvl_idx++) {
      // skip header
      ppFile.seekg(hdrSize*sizeof(int), std::ios::cur);
      if (strncmp(fgData[VAR].name, "u", 1) == 0) {
        // sequential read so byte-swap can occur if necessary
        for (int dataIdx = 0; dataIdx < dataSize; dataIdx++) {
          ppFile.read(reinterpret_cast<char*>(&tmpData.i),sizeof(float));
#ifndef WORDS_BIGENDIAN
          tmpData.i = ((((tmpData.i) & 0x00ff0000) >>  8) | 
           (((tmpData.i) & 0x0000ff00) <<  8) | 
           (((tmpData.i) & 0xff000000) >> 24) | 
           (((tmpData.i) & 0x000000ff) << 24)   ) ;
#endif
          fgData[VAR].val
	  [fileIdx*fgData[LEVEL].size*dataSize + lvl_idx*dataSize+dataIdx]
	   = tmpData.f;        
//      ppFile.read(reinterpret_cast<char*>(&fgData[VAR].val[lvl_idx*dataSize]),
//                  dataSize*sizeof(float));
          }
      } else {
        ppFile.seekg(dataSize*sizeof(float), std::ios::cur);
      }
      // skip junk value at end
      ppFile.seekg(1*sizeof(float), std::ios::cur);
    }

    // read or skip the v-wind values
    for (lvl_idx = 0; lvl_idx < (int)fgData[LEVEL].size; lvl_idx++) {
      // skip header
      ppFile.seekg(hdrSize*sizeof(int), std::ios::cur);
      if (strncmp(fgData[VAR].name, "v", 1) == 0) {
        // sequential read so byte-swap can occur if necessary
        for (int dataIdx = 0; dataIdx < dataSize; dataIdx++) {
          ppFile.read(reinterpret_cast<char*>(&tmpData.i),sizeof(float));
#ifndef WORDS_BIGENDIAN
          tmpData.i = ((((tmpData.i) & 0x00ff0000) >>  8) | 
           (((tmpData.i) & 0x0000ff00) <<  8) | 
           (((tmpData.i) & 0xff000000) >> 24) | 
           (((tmpData.i) & 0x000000ff) << 24)   ) ;
#endif
          fgData[VAR].val
	  [fileIdx*fgData[LEVEL].size*dataSize + lvl_idx*dataSize+dataIdx]
	   = tmpData.f;        
//      ppFile.read(reinterpret_cast<char*>(&fgData[VAR].val[lvl_idx*dataSize]),
//                  dataSize*sizeof(float));
          }
      } else {
        ppFile.seekg(dataSize*sizeof(float), std::ios::cur);
      }
      // skip junk value at end
      ppFile.seekg(1*sizeof(float), std::ios::cur);
    }
  
    // read or skip the geopotential height values
    for (lvl_idx = 0; lvl_idx < (int)fgData[LEVEL].size; lvl_idx++) {
      // skip header
      ppFile.seekg(hdrSize*sizeof(int), std::ios::cur);
      if (strncmp(fgData[VAR].name, "Z", 1) == 0) {
        // skip over the north pole values becuase u,v,w do not have values 
	// there and grids must be the same size; see comments at the top of
	// this function about grid sizes
	ppFile.seekg(96*sizeof(float), std::ios::cur);
        // sequential read so byte-swap can occur if necessary
        for (int dataIdx = 0; dataIdx < dataSize; dataIdx++) {
          ppFile.read(reinterpret_cast<char*>(&tmpData.i),sizeof(float));
#ifndef WORDS_BIGENDIAN
          tmpData.i = ((((tmpData.i) & 0x00ff0000) >>  8) | 
           (((tmpData.i) & 0x0000ff00) <<  8) | 
           (((tmpData.i) & 0xff000000) >> 24) | 
           (((tmpData.i) & 0x000000ff) << 24)   ) ;
#endif
          fgData[VAR].val
	  [fileIdx*fgData[LEVEL].size*dataSize + lvl_idx*dataSize+dataIdx]
	   = tmpData.f;        
//      ppFile.read(reinterpret_cast<char*>(&fgData[VAR].val[lvl_idx*dataSize]),
//                  dataSize*sizeof(float));
          }
      } else {
        ppFile.seekg(dataSize*sizeof(float), std::ios::cur);
      }
      // skip junk value at end
      ppFile.seekg(1*sizeof(float), std::ios::cur);
    }

  
    ppFile.close();
    // increment the file index
    fileIdx++;
    }  // end loop over fileList
    
  return FG_OK;
  }

////////////////////////////////////////////////////////////////////////////
char *Grid::pp_reftime(std::ifstream *ppFile) {
  char *dateString = new char[16];
  // nullify this string so concatenation begins at the first character
  dateString[0]='\0';
  
  // read the date from the header of the first data slab, then reset the
  // file input pointer
    
  // read in four integer values that are YYYY MM DD HH, the first value is
  // junk
  int *date;
  date = new int[5];
  for (int i=0; i<5; i++) {
  ppFile->read(reinterpret_cast<char*>(&date[i]),1*sizeof(int));
#ifndef WORDS_BIGENDIAN
  date[i] = ((((date[i]) & 0x00ff0000) >>  8) | 
            (((date[i]) & 0x0000ff00) <<  8) | 
            (((date[i]) & 0xff000000) >> 24) | 
            (((date[i]) & 0x000000ff) << 24)   ) ;
#endif
    }
  // reset the input file pointer
  ppFile->seekg(0);
   
  // put the date into a char string for the Grid object, skipping junk value
  char *chunk  = new char[4];
  for (int i=1; i<5; i++) {
    sprintf(chunk,"%d",date[i]);
    if (strlen(chunk) < 2) strcat(dateString,"0");
    strcat(dateString,chunk);
    if (i!=4) strcat(dateString," ");
    }
  strcat(dateString,":00");
  return dateString;
}  
