/****************************************************************************
    puff - a volcanic ash tracking model
    Copyright (C) 2001-2004 Rorik Peterson <rorik@gi.alaska.edu>

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
#include <fstream>
#include <cstdio>		// sscanf
#include <iostream>
#include <string>
#include <list>
#include <cmath>

#ifdef HAVE_NETCDFCPP_H
#include <netcdfcpp.h>
#else
#include "netcdfcpp.h"
#endif 

static const long int latSize = 72;
static const long int lonSize = 96;
static const long int levSize = 25;

// local functions
void createOutputFile (NcFile *ncfile);
void dump_header(std::ifstream *file, int size);
char *getOutfileName(char* ppfile);
char *pp_reftime(std::ifstream *filestream);
time_t unistr2time(const char *unistr);
void usage(char*);
void writeData(NcFile *ncfile, char *file);
void write_single_val(std::string);

int
main (int argc, char **argv)
{
  bool fileAddition = false;
  char *outFile = new char [1024];
  int fileIdx = 1;
  
  if (argc < 2)
  {
    usage(argv[0]);
    exit(1);
  }
  if (strcmp(argv[1],"add") == 0) 
  {
    if (argc < 3)
    {
      std::cerr << "ERROR: no file to add\n";
      usage(argv[0]);
      exit(1);
    } else {
      strncpy(outFile,argv[2],1024);
    }
    fileAddition = true;
    fileIdx += 2;
  } else {
    strcpy(outFile,getOutfileName(argv[1]) );
  }
  
  NcFile::FileMode state;
  if (fileAddition)
  {
    state = NcFile::Write;
  } else {
    state = NcFile::New;
  }
  
  NcFile ncfile (outFile, state);
  if (!ncfile.is_valid ())
    {
      std::string stateStr = (fileAddition ? "open" : "create");
      std::cerr << "ERROR: Failed to " << stateStr << " file " << outFile << std::endl;
      exit (1);
    }


  if (!fileAddition) createOutputFile (&ncfile);

  char *inFile = new char[1024];
  while (fileIdx < argc)
  {
    strcpy(inFile,argv[fileIdx]);
    writeData (&ncfile, inFile);
    fileIdx++;
  }
  
  ncfile.close ();
  return 0;
}

////////////////////////////////////////////////////////////////////////////
// read ukmet office BADC stratospheric data in 'pp' format
// see http://badc.nerc.ac.uk/data/assim/assimhelp.html for more info
////////////////////////////////////////////////////////////////////////////
void
writeData (NcFile * ncfile, char *file)
{
  // data is on a 2.5 x 3.75 degree grid.
  // temperature and geopotential height are +90->-90 (73 values), 
  // while u,v,w are 88.75->-88.75 (72 values), and are staggered.  We 
  // fudge this a little to put everything on a similar grid, so skip the +90
  // values for T and Z and use the u,v,w grid values.  For Puff, this should
  // not introduce too much error. Data is on a 2.5-deg lat x 3.75-deg lon grid
  // with 25 pressure levels - so each slab of wind is of size 
  // (180/2.5)*(360/3.75) = 6912, and temp & geo-height have an extra
  // (360/3.75) = 96 points, which is 7008.  There is a junk value at the end
  // of each record, so we add an extra for that and discard it later.

  static const int dataSize = 6912;

  // header data is 64 integer values, but there are three junk at the end
  // so add three for that

  static const int hdrSize = 67;

  // temporary data-holding union so byte-swapping can occur if necessary
  union
  {
    int i;
    float f;
  } tmpData;

  // the netCDF file variable pointer
  NcVar *vp;

  // open an input file stream, or exit if it fails.
  std::ifstream ppFile (file, std::ios::in);
  if (!ppFile)
    {
      std::cerr << "failed to open pp file \"" << file << "\"\n";
      return;
    }

  // get the file reference time from the first few bytes of data
  std::string datetime = pp_reftime(&ppFile);
  
  // determine the number of records so we can append the value
  vp = ncfile->get_var("valtime"); 
  const long int recIdx = vp->num_vals();
  
  // write time variable attribute as "hours since YYYY-1-1
  std::string timeSince = "hours since ";
  timeSince.append(datetime,0,4);
  timeSince.append("-1-1");
  vp=ncfile->get_var("valtime");
  vp->add_att((NcToken)"units",timeSince.c_str());
  
  // create a YYYY MM DD HH:MM string for determining time values
  std::string reftime = datetime.substr(0,4);
  reftime.append(" 01 01 00:00");
  
  double hours =
    (unistr2time(datetime.c_str())-unistr2time(reftime.c_str()))/3600;
  // set the record for appending
  vp->set_rec(recIdx);
  // before adding this value, make sure it does not already exist
  if (vp->get_index(&hours) >= 0)
  {
    std::cout << "Values from file \"" << file << "\" already exist in the archive. Skipping.\n";
    return;
  } else {
    vp->put_rec(&hours);
  }
  
  // data is sequential with a header between each slab of data.  The order is
  // u, v, geo-height, temperature, w.  

  int lvl_idx;  // level index 

  vp = ncfile->get_var ("u");
  if (!vp)
    {
      std::cerr << "ERROR: failed to find variable \"u\" in file\n";
      return;
    }

  // values are read from file, converted if bigendian, and written one entire
  // record at a time.
  float *val = new float[levSize * lonSize * latSize];
  int valIdx = 0;

  // read the u wind values
  for (lvl_idx = 0; lvl_idx < levSize; lvl_idx++)
    {
      // skip header
//      dump_header(&ppFile, hdrSize);
      ppFile.seekg (hdrSize * sizeof (int), std::ios::cur);
      // sequential read so byte-swap can occur if necessary
      for (int dataIdx = 0; dataIdx < dataSize; dataIdx++)
	{
	  ppFile.read (reinterpret_cast < char *>(&tmpData.i),
		       sizeof (float));
#ifndef WORDS_BIGENDIAN
	  tmpData.i = ((((tmpData.i) & 0x00ff0000) >> 8) |
		       (((tmpData.i) & 0x0000ff00) << 8) |
		       (((tmpData.i) & 0xff000000) >> 24) |
		       (((tmpData.i) & 0x000000ff) << 24));
#endif
	  val[valIdx++] = tmpData.f;
	}

      // skip junk value at end
      ppFile.seekg (1 * sizeof (float), std::ios::cur);
    }
  // set the record for appending
  vp->set_rec(recIdx);
  // put a records worth of data into the netCDF file
  vp->put_rec (val);

  // read the v-wind values
  vp = ncfile->get_var ("v");
  if (!vp)
    {
      std::cerr << "ERROR: failed to find variable \"v\" in file\n";
      return;
    }
    
  valIdx = 0;  // reset the value index
  
  for (lvl_idx = 0; lvl_idx < levSize; lvl_idx++)
    {
      // skip header
//      dump_header(&ppFile, hdrSize);
      ppFile.seekg (hdrSize * sizeof (int), std::ios::cur);
      // sequential read so byte-swap can occur if necessary
      for (int dataIdx = 0; dataIdx < dataSize; dataIdx++)
	{
	  ppFile.read (reinterpret_cast < char *>(&tmpData.i),
		       sizeof (float));
#ifndef WORDS_BIGENDIAN
	  tmpData.i = ((((tmpData.i) & 0x00ff0000) >> 8) |
		       (((tmpData.i) & 0x0000ff00) << 8) |
		       (((tmpData.i) & 0xff000000) >> 24) |
		       (((tmpData.i) & 0x000000ff) << 24));
#endif
	  val[valIdx++] = tmpData.f;
	}
      // skip junk value at end
      ppFile.seekg (1 * sizeof (float), std::ios::cur);
    }
  // set the record for appending
  vp->set_rec(recIdx);
  // put a records worth of data into the netCDF file
  vp->put_rec (val);

  // read the geopotential height values
  vp = ncfile->get_var ("Z");
  if (!vp)
    {
      std::cerr << "ERROR: failed to find variable \"Z\" in file\n";
      return;
    }
    
  valIdx = 0;  // reset the value index
  
  for (lvl_idx = 0; lvl_idx < levSize; lvl_idx++)
    {
      // skip header
//      dump_header(&ppFile, hdrSize);
      ppFile.seekg (hdrSize * sizeof (int), std::ios::cur);
      // skip over the north pole values because u,v,w do not have values 
      // there and grids must be the same size; see comments at the top of
      // this function about grid sizes
      ppFile.seekg (96 * sizeof (float), std::ios::cur);
      // sequential read so byte-swap can occur if necessary
      for (int dataIdx = 0; dataIdx < dataSize; dataIdx++)
	{
	  ppFile.read (reinterpret_cast < char *>(&tmpData.i),
		       sizeof (float));
#ifndef WORDS_BIGENDIAN
	  tmpData.i = ((((tmpData.i) & 0x00ff0000) >> 8) |
		       (((tmpData.i) & 0x0000ff00) << 8) |
		       (((tmpData.i) & 0xff000000) >> 24) |
		       (((tmpData.i) & 0x000000ff) << 24));
#endif
	  val[valIdx++] = tmpData.f;
	}
      // skip junk value at end
      ppFile.seekg (1 * sizeof (float), std::ios::cur);
    }
  // set the record for appending
  vp->set_rec(recIdx);
  // put a records worth of data into the netCDF file
  vp->put_rec (val);

  // read the temperature values
  vp = ncfile->get_var ("T");
  if (!vp)
    {
      std::cerr << "ERROR: failed to find variable \"T\" in file\n";
      return;
    }
    
  valIdx = 0;  // reset the value index
  
  for (lvl_idx = 0; lvl_idx < levSize; lvl_idx++)
    {
      // skip header
//      dump_header(&ppFile, hdrSize);
      ppFile.seekg (hdrSize * sizeof (int), std::ios::cur);
      // skip over the north pole values because u,v,w do not have values 
      // there and grids must be the same size; see comments at the top of
      // this function about grid sizes
      ppFile.seekg (96 * sizeof (float), std::ios::cur);
      // sequential read so byte-swap can occur if necessary
      for (int dataIdx = 0; dataIdx < dataSize; dataIdx++)
	{
	  ppFile.read (reinterpret_cast < char *>(&tmpData.i),
		       sizeof (float));
#ifndef WORDS_BIGENDIAN
	  tmpData.i = ((((tmpData.i) & 0x00ff0000) >> 8) |
		       (((tmpData.i) & 0x0000ff00) << 8) |
		       (((tmpData.i) & 0xff000000) >> 24) |
		       (((tmpData.i) & 0x000000ff) << 24));
#endif
	  val[valIdx++] = tmpData.f;
	}
      // skip junk value at end
      ppFile.seekg (1 * sizeof (float), std::ios::cur);
    }
  // set the record for appending
  vp->set_rec(recIdx);
  // put a records worth of data into the netCDF file
  vp->put_rec (val);

  ppFile.close ();

  return;
}

////////////////////////////////////////////////////////////////////////////
char *
pp_reftime (std::ifstream * ppFile)
{
  char *dateString = new char[16];
  // nullify this string so concatenation begins at the first character
  dateString[0] = '\0';

  // read the date from the header of the first data slab, then reset the
  // file input pointer

  // read in four integer values that are YYYY MM DD HH, the first value is
  // junk
  int *date;
  date = new int[5];
  for (int i = 0; i < 5; i++)
    {
      ppFile->read (reinterpret_cast < char *>(&date[i]), 1 * sizeof (int));
#ifndef WORDS_BIGENDIAN
      date[i] = ((((date[i]) & 0x00ff0000) >> 8) |
		 (((date[i]) & 0x0000ff00) << 8) |
		 (((date[i]) & 0xff000000) >> 24) |
		 (((date[i]) & 0x000000ff) << 24));
#endif
    }
  // reset the input file pointer
  ppFile->seekg (0);

  // put the date into a char string for the Grid object, skipping junk value
  char *chunk = new char[4];
  for (int i = 1; i < 5; i++)
    {
      sprintf (chunk, "%d", date[i]);
      if (strlen (chunk) < 2)
	strcat (dateString, "0");
      strcat (dateString, chunk);
      if (i != 4)
	strcat (dateString, " ");
    }
  strcat (dateString, ":00");
  return dateString;
}

////////////////////////////////////////////////////////////////////////////
void
createOutputFile (NcFile * ncfile)
{
  NcDim *lat = ncfile->add_dim ("lat", latSize);
  NcDim *lon = ncfile->add_dim ("lon", lonSize);
  NcDim *lev = ncfile->add_dim ("level", levSize);
  NcDim *rec = ncfile->add_dim ("record");

  NcVar *vp = ncfile->add_var ("lat", ncFloat, lat);
  vp = ncfile->add_var ("lon", ncFloat, lon);
  vp = ncfile->add_var ("level", ncFloat, lev);
  vp = ncfile->add_var ("valtime", ncDouble, rec);
  vp = ncfile->add_var ("T", ncFloat, rec, lev, lat, lon);
  vp = ncfile->add_var ("u", ncFloat, rec, lev, lat, lon);
  vp = ncfile->add_var ("v", ncFloat, rec, lev, lat, lon);
  vp = ncfile->add_var ("Z", ncFloat, rec, lev, lat, lon);
  
    // set the coordinate values; it would be better to read these from the
  // header in case they change, or the file is corrupted

  // set latitude values
  vp = ncfile->get_var ("lat");
  if (!vp)
    {
      std::cerr << "ERROR: Failed to get \"lat\" variable\n";
      return;
    }
  float *latValues = new float[latSize];
  for (int i = 0; i < latSize; i++)
    {
      latValues[i] = 88.75 - 2.5 * i;
    }
  vp->put (latValues, &latSize);
  delete[]latValues;
  vp->add_att((NcToken)"units","degrees_north");

  // set longitude values
  vp = ncfile->get_var ("lon");
  if (!vp)
    {
      std::cerr << "ERROR: Failed to get \"lon\" variable\n";
      return;
    }
  float *lonValues = new float[lonSize];
  for (int i = 0; i < lonSize; i++)
    {
      lonValues[i] = 3.75 * i;
    }
  vp->put (lonValues, &lonSize);
  delete[]lonValues;
  vp->add_att((NcToken)"units","degrees_east");

  // set level values
  vp = ncfile->get_var ("level");
  if (!vp)
    {
      std::cerr << "ERROR: Failed to get \"level\" variable\n";
      return;
    }
    
  // assign the level values, which are 1000 * 10^(-i/6); i from 1..levSize
  // see badc documentation for more on this formula
  
  float *levValues = new float[levSize];
  for (int i = 0; i<levSize; i++)
  {
    double exponent = (double)i/6;
    levValues[i] = 1000 * pow(10,-exponent);
  }
  
  vp->put (levValues, &levSize);
  delete[]levValues;
  vp->add_att((NcToken)"units","millibars");

  // set the variables units
  vp=ncfile->get_var("T");
  vp->add_att((NcToken)"units","degK");
  vp=ncfile->get_var("u");
  vp->add_att((NcToken)"units","m/s");
  vp=ncfile->get_var("v");
  vp->add_att((NcToken)"units","m/s");
  vp=ncfile->get_var("Z");
  vp->add_att((NcToken)"units","gp m");

  
  return;
}
//////////////////////////////////////////////////////////////////////
// copied from original Puff source
//
// This routine takes a string in unidata format, "YYYY MM DD HH:MM" 
// and converts it into the standard C time_t variable (time.h)
// The time_t variable is put into UTC COORDINATES!
//
///////////////////////////////////////////////////////////////////////
time_t unistr2time(const char *unistr) {
    extern long timezone;
    struct tm date;
    
    if( sscanf(unistr, "%d %d %d %d:%d", &date.tm_year, &date.tm_mon, &date.tm_mday, &date.tm_hour, &date.tm_min) != 5 ) {
      std::cerr << "ERROR: date/time string " << unistr << "is not valid\n";
      return (time_t)NULL;
      }
    
    // There are no seconds:
    date.tm_sec = 0;
    
    // year is relative to 1900
    date.tm_year -= 1900;
    
    // month has zero index
    date.tm_mon -= 1;
        
    // Daylight savings -1 = unknown: LOCAL TIME ZONE IS USED WITH -1
    date.tm_isdst = 0;

    // Call tzset to set correct timezone offset:
    tzset();

    time_t time;
    if ( (time = mktime(&date)-timezone) == -1 ) {
        std::cerr << std::endl;
        std::cerr << "ERROR: unistr2time() : Invalid date string \"" 
             << unistr << "\"" 
             << std::endl;
	return 0;
    }

    return (time);

}

///////////////////////////////////////////////////////////////////////
// give usage information
///////////////////////////////////////////////////////////////////////
void 
usage(char *caller) 
{
  std::string prog=caller;
  unsigned int loc = prog.find_last_of('/');
  if (loc != std::string::npos)
  {
    prog=prog.substr(loc+1,prog.length());
  }
  
  std::cout << prog << ": a converter from UK Met office 'pp' format to netCDF for use with the Puff Volcanic Ash Tracking Model\n";
  std::cout << "usage:\n\t" << prog << " [add <ncfile] pp_file(s)\n";
  return;
}
///////////////////////////////////////////////////////////////////////
// open the 'pp' file, get the timestamp, close, and return a netCDF file name
// based on it.
///////////////////////////////////////////////////////////////////////
char* 
getOutfileName(char *file)
{
  // open an input file stream, or exit if it fails.
  std::ifstream ppFile (file, std::ios::in);
  if (!ppFile)
    {
      std::cerr << "failed to open pp file \"" << file << "\"\n";
      return (char*)NULL;
    }

  // get the file reference time from the first few bytes of data
  char *datetime = new char [16];
  strcpy(datetime,pp_reftime(&ppFile));
  ppFile.close();
  
  // make the new file YYYYMMDD_badc.nc
  char *outFile = new char[17];
  strncpy(outFile,datetime,4);
  strncpy(&outFile[4],&datetime[5],2);
  strncpy(&outFile[6],&datetime[8],2);
  strncpy(&outFile[8],&datetime[11],2);
  strncpy(&outFile[10],"_badc.nc",9);
  
  return outFile;
}
///////////////////////////////////////////////////////////////////////
// this function is not called by default.  Uncomment it in the above program
// and comment out the skip header line.  This is used for debugging since
// the header does contain some information about the binary files, which is
// useful when the format changes
///////////////////////////////////////////////////////////////////////
void 
dump_header(std::ifstream *ppFile, int hdrSize)
{
  union 
  {
    int i;
    float f;
  } data;
  
  // print out header for vebosity
  for (int hIdx = 0; hIdx < hdrSize; hIdx++)
  {
    (*ppFile).read(reinterpret_cast<char *>(&data), sizeof(int));
#ifndef WORDS_BIGENDIAN
    data.i = ((((data.i) & 0x00ff0000) >> 8) |
                  (((data.i) & 0x0000ff00) << 8) |
		  (((data.i) & 0xff000000) >> 24) |
		  (((data.i) & 0x000000ff) << 24));
#endif
    std::cout << data.i << " ";
  }
  // add a newline
  std::cout << std::endl;
   return;
}   
