/****************************************************************************
    puff - a volcanic ash tracking model
    Copyright (C) 2004-6 Rorik Peterson <rorik@gi.alaska.edu>

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
#include <fstream>	// ifstream
#include <iostream>	// cout, cerr
#include <glob.h>	// glob()
#include <cmath>
#include "planes.h"

#include "puff_options.h"
extern Argument argument; 
////////////////////////////////////////////
// constructor, take a vector of strings that are file name or file globs.
// open each one and read the records from the file, creating a vector of 
// flights.
Planes::Planes(std::vector<std::string> file_v)
{
  if (file_v.size() == 0) return;
  std::cout << "calculating plane exposure ... \n" << std::flush;
  for (unsigned int f_idx = 0; f_idx < file_v.size(); f_idx++)
  {
    std::string file = file_v[f_idx];
  
  
  // 'file' could actually be a glob of filenames
  glob_t fileGlob;
  (void)glob(file.c_str(), 0, NULL, &fileGlob);
  for (unsigned int i = 0; i < fileGlob.gl_pathc; i++)
  {    
    std::ifstream planesFile ( fileGlob.gl_pathv[i], std::ios::in);
  
    if (!planesFile)
    {
      std::cerr << "Failed to open Planes file " << fileGlob.gl_pathv << std::endl;
    }
    readPlanesFile(&planesFile);
  }
  if (fileGlob.gl_pathc == 0)
    std::cerr << "WARNING: planes file '" << file << "' matches nothing.\n";
    
  globfree(&fileGlob);
  
  }
  
  // set the start and end times
  setEndTimes();
  return;
}

////////////////////////////////////////////
// destructor
Planes::~Planes()
{
	return;
}
////////////////////////////////////////////
// set the start and end times tags assuming the times are in sequential order
void Planes::setEndTimes()
{
  for (unsigned int f_idx = 0; f_idx < flight.size(); f_idx++)
  {
    flight[f_idx].start_time = flight[f_idx].location[0].time;
    flight[f_idx].end_time = flight[f_idx].location[flight[f_idx].location.size()-1].time;
  }
  return;
}
////////////////////////////////////////////
// Take this filestream and read data records from it.  Add the data from
// each line into the vector of locations for each flight.  Create new flights
// when the flight numbers change.  
void Planes::readPlanesFile(std::ifstream *planesFile)
{
  struct Flight* cur = NULL;
  const static int LINE_LENGTH = 100;
  char *line;
  
  line = (char*)calloc(LINE_LENGTH, sizeof(char)); 
  while (planesFile->getline(line, LINE_LENGTH, '\n') != NULL)
  {
    // skip bad data, or useless header stuff
    flData data = parseLine(line);
    if (data.empty) {
			clearData(&data);
			continue;
			}
		// new flights when call name (cname) changes.  Do not use fltnum since
		// date could change for overnight flights
    if (cur == NULL) newFlight(&data);
    else if (strcmp(data.cname, cur->cname.c_str())) newFlight(&data);

    // now point 'cur' to the last flight in the list
    cur = &(*this).flight[(*this).flight.size()-1];
    cur->location.push_back(data.loc);
//    std::cout << "added location, now " << cur->location.size() << std::endl;
    clearData(&data);
  }
  free(line); 
  return;
}
////////////////////////////////////////////
// create a new flight and add it to the vector.  Initialize some data using
// this lines data
void Planes::newFlight(const struct flData* data)
{
  
  struct Flight *flight = new Flight;
  flight->orig = data->origin;
  flight->dest = data->dest;
  flight->make = data->make;
  flight->fltnum = data->fltnum;
  flight->cname = data->cname;
  
  (*this).flight.push_back(*flight);
	if (argument.verbose)
    std::cout << "Added flight: " << data->fltnum << std::endl;
  delete flight;
  
  return;
} 
////////////////////////////////////////////
// attempt to put this lines data into a flight data 'flData' structure, 
// returning a reference to it.  Success only occurs if every field gets
// filled properly, in which case 'flData.empty' becomes false
flData parseLine(char* line)
{
  flData data;
  data.empty = true; // assume data is bad at first
  char *field[10];  // there are 10 possible data fields in each line
  
  // tokenize the string by making the 'field' pointers point to locations
  // within 'line', _not_ a copy of line.  Then, replace commas with '\0'.
  // This way, missing fields will be simply '\0'
  int ret = 0;
  field[ret++] = line;
  char *c = strchr(line, ',');
  while (c != NULL)
  {
    *c = '\0';
    field[ret]=++c;
    c=strchr(field[ret], ',');
    ret++;
  }

  // return if we don't have the correct number of fields.
  if (ret < 10) return data;
  
	// fltnum will be the flight number and the date since the same flight
	// number may occur on several days.
  data.fltnum = (char*)calloc(strlen(field[0])+strlen(field[1])+2,sizeof(char));
  strcpy(data.fltnum, field[0]);
  strcat(data.fltnum, "-");
  strcat(data.fltnum, field[1]);
  data.cname = strdup(field[0]);
  data.origin = strdup(field[3]);
  data.dest = strdup(field[4]);
  data.make = strdup(field[9]);
  
  // make lat, lon decimal values and positive east and north only
  int l_deg, l_min;
  char l_dir;
  if (sscanf(field[7],"%2i%2i%c",&l_deg, &l_min, &l_dir) != 3) return data;
  data.loc.lat = (float)l_deg + (float)l_min/60;
  if (l_dir == 'S') data.loc.lat = -data.loc.lat;
  if (sscanf(field[8],"%3i%2i%c",&l_deg, &l_min, &l_dir) != 3) return data;
  data.loc.lon = (float)l_deg + (float)l_min/60;
  if (l_dir == 'W') data.loc.lon = 360 - data.loc.lon;
  
  // create a time_t value from date (field[1]) and time fields
  struct tm date;
  char monthAbr[5];
  // parsing the time is hard because of the zeros
  int h, hh, m, mm;
  ret = sscanf(field[2],"%1i%1i%1i%1i", &hh, &h, &mm, &m);
  if (ret != 4) return data;
  date.tm_hour = 10*hh+h;
  date.tm_min  = 10*mm+m;
  // replace / with space for parsing
  while (char *c = strchr(field[1], '/')) *c = ' ';
	// same difficulty with zeros as above 
  //ret = sscanf(field[1],"%i %s %i", &date.tm_mday, monthAbr, &date.tm_year);
  ret = sscanf(field[1],"%1i%1i %s %1i%1i", &hh, &h, monthAbr, &mm, &m);
  if (ret != 5) return data;
	date.tm_mday = 10*hh+h;
	date.tm_year = 10*mm+m;
  // date is relative to 1900, so '99' is ok, but '01' should be 101
  if (date.tm_year < 50) date.tm_year += 100;
  date.tm_mon = monthAbrToNum(monthAbr); 
  date.tm_sec = 0;
  date.tm_isdst = 0;
	//date.tm_zone = "UTC";
  
  data.loc.time = mktime(&date);
	// whew! should be done with the date now
  
  // fill in flight level (100's feet) and speed (knots?)
  int feet;
  ret = sscanf(field[6], "%i", &feet);
  if (ret != 1) return data;
  // convert level to meters to jive with the rest of puff
  data.loc.level = (float)feet * 100.0 / 3.281;
  ret = sscanf(field[5], "%i", &data.loc.speed);
  if (ret != 1) return data;
	// convert knots (nautical miles per hour) to meters per second
	// 1 naut.mile = 1852 meters and 1 hr = 3600 s
	// so naut.mile per hour is 0.5144444444 meters per second
	data.loc.speed = 0.514444444 * data.loc.speed;
  
  // all field are now full
  data.empty = false;
  
  return data;
}
  
////////////////////////////////////////////
// calculate the exposure for each flight by summing up the dose at each
// time step in the flights location vector.  
void Planes::calculateExposure (CCloud *cc)
{
  // loop thorugh all the flights
  std::vector<Flight>::const_iterator f;
  for (f=flight.begin(); f != flight.end(); f++)
  {
    // skip this flight if no times overlap
    if ((*f).start_time > (time_t)cc->tValues[cc->tSize-1]) continue;
    if ((*f).end_time < (time_t)cc->tValues[0]) continue;
    float exp = 0; // the net exposure for this flight
		float exp_c = 0; // conc. exposure for stationary sites
    float e_time = 0; // time this flight is exposed, used for debugging
		                  // when some flights have sparse data
    
    for (unsigned int i = 0; i < (*f).location.size()-1; i++)
    {
      float dose = 0.5*(abs_conc(cc, &((*f).location[i])) + abs_conc(cc, &((*f).location[i+1])));
			// exposure increases by dose * time * speed
			// g/m^3 * s * m/s = g/m^2
      exp += dose*((*f).location[i+1].time-(*f).location[i].time) * (*f).location[i].speed;
      exp_c += dose*((*f).location[i+1].time-(*f).location[i].time);
      e_time += (*f).location[i+1].time - (*f).location[i].time;
    }
    // only report non-zero values
//    if (exp > 1) 
//						exposure.insert(fs_mmap::value_type(exp, (*f).fltnum));      
//   std::cout << (*f).fltnum << "(" << e_time << ")\n";
		std::cout << (*f).fltnum << " => ";
	// stationary sites have zero exp due to zero speed, but exp_c is nonzero
	if ((exp == 0) and (exp_c != 0)) {std::cout <<exp_c<<" g*s/m^3\n";}
	else {std::cout <<exp<< " g/m^2\n";}

   }
   // print results
   for (fs_mmap::const_iterator i = exposure.begin(); i != exposure.end(); i++)
   {
//     std::cout << i->first << " => " << i->second << "\t" << "\n";
   }
  return;
}
////////////////////////////////////////////
// delete allocated space in an flData structure
void Planes::clearData(flData* data)
{

    if (data->fltnum) free(data->fltnum);
    if (data->cname) free(data->cname);
    if (data->origin) free(data->origin);
    if (data->dest) free(data->dest);
    if (data->make) free(data->make);

  return;
}
////////////////////////////////////////////
// convert three letter month abreviation into struct tm tm_mon int value.
int monthAbrToNum(char *s)
{
  if (strncmp(s, "JAN", 3) == 0) return 0;
  if (strncmp(s, "FEB", 3) == 0) return 1;
  if (strncmp(s, "MAR", 3) == 0) return 2;
  if (strncmp(s, "APR", 3) == 0) return 3;
  if (strncmp(s, "MAY", 3) == 0) return 4;
  if (strncmp(s, "JUN", 3) == 0) return 5;
  if (strncmp(s, "JUL", 3) == 0) return 6;
  if (strncmp(s, "AUG", 3) == 0) return 7;
  if (strncmp(s, "SEP", 3) == 0) return 8;
  if (strncmp(s, "OCT", 3) == 0) return 9;
  if (strncmp(s, "NOV", 3) == 0) return 10;
  if (strncmp(s, "DEC", 3) == 0) return 11;
  
  std::cerr << "Month abreviation " << s << "does not match anything I know\n";
  exit(1);
  // give a return value to eliminate needless compiler warnings
  return 0;
}

////////////////////////////////////////////////////////////////////////
// calculate the concentration
float Planes::abs_conc(CCloud *cc, const Location *loc)
{
	float loc_lon = loc->lon;
	/* loc->lon may be a large east value such as 350, when the ash cloud
	 * bounds are something like -20 -> +20.  Adjust the local value for
	 * the case where loc->lon is outside but loc->lon - 360 is inside
	 */
	if ((loc_lon > cc->xValues[cc->xSize-1]) &&
			(loc_lon-360 > cc->xValues[0] && loc_lon-360 < cc->xValues[cc->xSize-1]))
	{
		loc_lon = loc_lon - 360.0;
	}

  // return NULL if out side bounds
  if (loc_lon < cc->xValues[0]) return (float)NULL;
  if (loc_lon > cc->xValues[cc->xSize-1]) return (float)NULL;
  if (loc->lat < cc->yValues[0]) return (float)NULL;
  if (loc->lat > cc->yValues[cc->ySize-1]) return (float)NULL;
  if (loc->level < cc->zValues[0]) return (float)NULL;
  if (loc->level > cc->zValues[cc->zSize-1]) return (float)NULL;
  if (loc->time < cc->tValues[0]) return (float)NULL;
  if (loc->time > cc->tValues[cc->tSize-1]) return (float)NULL;
  
  float dx = cc->xValues[1]-cc->xValues[0];
  float dy = cc->yValues[1]-cc->yValues[0];
  float dz = cc->zValues[1]-cc->zValues[0];
  float dt = cc->tValues[1]-cc->tValues[0];
  
  const int xIdx = (int)floor((loc_lon-cc->xValues[0])/dx);
  const int yIdx = (int)floor((loc->lat-cc->yValues[0])/dy);
  const int zIdx = (int)floor((loc->level-cc->zValues[0])/dz);
  const int tIdx = (int)floor((loc->time-cc->tValues[0])/dt);

  const int cIdx = xIdx + yIdx*cc->xSize + zIdx*cc->xSize*cc->ySize + tIdx*cc->xSize*cc->ySize*cc->zSize;
  
  return cc->abs_air_conc_avg[cIdx];
}
  
