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
#include "puff.h"
#include "volc_utils.h"

//const double	Re = 6371220.0; // earth radius in meters
//const double	pi = 3.14159;
const double	Deg2Rad = M_PI/180.0;
const double	Rad2Deg = 180.0/M_PI;
const double	GravConst = (2./9.)*(1.08e9);

const int PUFF_ERROR = FG_ERROR;
const int PUFF_OK = FG_OK;

void removeTrailingWhitespace(std::string *s);
void removeTrailingWhitespace(char *s);
//////////////////////////////////////////////////////////////////////
//
// This routine parses the volcano args to return the volcano lat and 
// lon as double values
//
///////////////////////////////////////////////////////////////////////
void get_lon_lat(char *volc, double &lon, double &lat) {
    int i;

  if (strlen(volc) <= 0)
  {
    std::cerr << "ERROR: No volcano name specified.\n";
    exit(0);
  }
  
    lon = puff_undef_dbl;
    lat = puff_undef_dbl;
	
    int nvolc = strlen(volc);
    char *tmparg = new char[nvolc+1];
	
    // uppercase arg:
    strcpy(tmparg, volc);
    for (i=0; i<nvolc; i++) {
#ifdef HAVE_TOUPPER
      tmparg[i] = toupper(tmparg[i]);
#else
      tmparg[i] = tmparg[i] & 223;
#endif
    }
	
// from AFWA version
  VOLCANO_DATA volcList[MAXVOLCS];
          
  int volcCount = readVolcList(volcList, MAXVOLCS);
        
  std::vector<std::string> matches;
  
  for (int i = 0; i < volcCount; i++) 
  {
    if ( strncmp(tmparg, volcList[i].name, nvolc) == 0 ) {
      lon = volcList[i].lon;
      lat = volcList[i].lat;
      if (lon < 0) { lon += 360.0; }	   
			// remove trailing white space for comparison below
			removeTrailingWhitespace(volcList[i].name);
			// if this is a perfect match, stop
			if (strcmp(tmparg, volcList[i].name) == 0)
			{
        matches.push_back(volcList[i].name);
				i=volcCount;
			} else {
			// otherwise keep it as a possibility and keep looking
        matches.push_back(volcList[i].name);
			}
    }
	} 

  delete[] tmparg;
  
  // if only one match, copy this name if they are not the same length, so
  // short names are spelled out, such as 'bezy' -> 'bezymianny'
  if (matches.size() == 1)
  {
    if (strlen(volc) != matches[0].size())
    {
//      removeTrailingWhitespace(&matches[0]);
			strcpy(volc, (char*)matches[0].c_str());
    }
  // report ambigous arguments and exit
  } else if (matches.size() > 1)
  {
    std::cerr << "ERROR: volcano name \"" << volc << "\" is ambiguous.  Possible matches include:\n";
    for (unsigned int i = 0; i < matches.size(); i++)
    {
      std::cout << matches[i].c_str() << std::endl;
    }
    exit(0);
  }
  return;
}

//////////////////////////////////////////////////////////////////////
//
//  make sure lat and lon specified are valid
//
//////////////////////////////////////////////////////////////////////

int check_lon_lat(char *vname, double lon, double lat) {

  if ( lon < -180 || lon > 360 || lat < -90 || lat > 90) {
    
    std::cerr << "ERROR: Bad volcano site: \"" << vname
	      << "\" ( " << lon << " , " << lat << " )\n\n"
	      << "Expected (-180 < lon < 360) and (-90 < lat < 90)\n"
	      << "Explicitely set -volcLon and -volcLat or use a " 
	      << " predefined site with -volc" 
	      << std::endl;

	return PUFF_ERROR;
    }
  
    return PUFF_OK;
}


////////////////////////////////////////////////////////////////////////
//
// SHOW_VOLCS
// This routine displays the possible volcanos defined in volcsites.args
//
////////////////////////////////////////////////////////////////////////
void show_volcs() {

//    std::cout.setf(ios::showpoint);
    
    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    
    // Read volcano list
    
  VOLCANO_DATA volcList[MAXVOLCS];
          
  int volcCount = readVolcList(volcList, MAXVOLCS);
        
  for (int i=0; i<volcCount; i++) {
    
    std::cout.unsetf(std::ios::right);
    std::cout.setf(std::ios::left);
    std::cout.width(23);
    std::cout << volcList[i].name;
   
    std::cout.width(23);
    std::cout << volcList[i].location;
	
    std::cout.unsetf(std::ios::left);
    std::cout.setf(std::ios::right);
    
    std::cout.width(6);
    if ((int)volcList[i].elevation == -99999)
      std::cout << "?" << " m";
    else
      std::cout << (int)volcList[i].elevation << " m";
    
    std::cout.setf(std::ios::showpoint);
    std::cout.width(8);
    std::cout.precision(2);
    std::cout << volcList[i].lat << " N";
	
    std::cout.width(8);
    std::cout.precision(2);
    std::cout << volcList[i].lon << " E";
	
    std::cout << std::endl;
	
    }
  }

//////////////////////////////////////////////////////////////////////
//
// This routine takes a value X with a node spacing DX and returns
// The nearest node value (integral of DX).
//
///////////////////////////////////////////////////////////////////////
float nrst_grid(float x, float dx) {
    double junk;
    float xx = fabs(x/dx);
    float ctr_x = dx*(floor(xx) + floor(2.*modf(xx, &junk)));
    if (x < 0) ctr_x = -ctr_x;
    
    return ctr_x;
}

/////////////////////////////////////////////////////////////////////////
//
// CREATE AN ERUPTION DATE STRING FROM THE STRING eruptDate
// THIS WILL CREATE A RUN_TIME DATE GIVEN CERTAIN CONDITIONS
//
/////////////////////////////////////////////////////////////////////////
int get_eruptDate(char *erupt_date, char *arg_eruptDate) {

  // FIRST CHECK STRING LENGTH:
  if ( strlen(arg_eruptDate) != 16 && arg_eruptDate[0] != '-' ) {
    std::cerr << std::endl;
    std::cerr << "ERROR: Bad eruption date specified : \""
         << arg_eruptDate << "\""  << std::endl;
    std::cerr << "        Options:" << std::endl;
    std::cerr << "        Absolute date: -eruptDate=\"YYYY MM DD HH:MM\"" 
	 << std::endl;
    std::cerr << "        Relative time: -eruptDate=-nhours" << std::endl;
    std::cerr << std::endl;
    return PUFF_ERROR;
    }


  switch (arg_eruptDate[0]) {
  // CREATE A RUN-TIME DATE: (FROM -HOURS)
    case ('-') : { 
      time_t tnow = time(NULL);
      double offhrs = strtod(arg_eruptDate, (char **)NULL);
      tnow += time_t(offhrs*3600.0);
      strcpy(erupt_date, time2unistr(tnow));
      break;
      }
    // USE THE ARG DATE:
    case ('1') : { // YEAR 19....
      strcpy(erupt_date, arg_eruptDate);
      break;
      }
    case ('2') : { // YEAR 2000?!
      strcpy(erupt_date, arg_eruptDate);
      break;
      }
    // BAD SET:
    default : {
      std::cerr << std::endl;
      std::cerr << "ERROR: Bad eruption date specified : \""
           << arg_eruptDate << "\"" << std::endl;
      std::cerr << "        Options:"  << std::endl;
      std::cerr << "        Absolute date: -eruptDate=\"YYYY MM DD HH:MM\"" 
           << std::endl;
      std::cerr << "        Relative time: -eruptDate=-nhours" << std::endl;
      std::cerr << std::endl;
      return PUFF_ERROR;
      } 
    } /* switch */
    
  return PUFF_OK;
  }

/////////////////////////////////////////////////////////////////////////
//
// convert delta-meters to delta-degrees.  When near the poles, there are
// approximations to give somewhat decent results
//
/////////////////////////////////////////////////////////////////////////
void meter2sphere(double &dx, double &dy, double &y) {

  double Re = PUFF_EARTH_RADIUS;
  // calculate dy first so we can add/subt to/from it later
  dy = (double)Rad2Deg*dy/Re;
  // Modified by D.S.Tillman to avoid division by zero at poles
  if ( (y > 89.999) || (y < -89.999) ) {
    dx = 0.0;
    }
  else {
    // temporary value of what dx "might" be
    double dx1 = Rad2Deg*dx/(Re*cos(Deg2Rad*y));
    // magnitude of dx1
    double mdx = sqrt(dx1*dx1);
    // if dx is too large, it needs to be adjustsed
    if ( mdx > 1) {
      if (mdx < 45) { 
        dy -= sqrt(dx*dx*2)*Rad2Deg/Re;
	dx = dx1;
	}
      else {
        dx = 90*dx1/mdx;
        dy -= Rad2Deg*sqrt(dx*dx)/Re;
        }
      }              
    else {
      dx = dx1;
      } 
    }
  return;
  }

/////////////////////////////////////////////////////////////////////////
void removeTrailingWhitespace(std::string *s)
{
  int loc;
  
  // remove trailing whitespace from sue
  loc = s->find_last_of(" ");
  while (s->length() != 0 && loc == (int)(s->length()-1)) 
  {
    *s=s->substr(0,s->length()-1);
    loc = s->find_last_of(" ");
  }
  return;
}
/////////////////////////////////////////////////////////////////////////
void removeTrailingWhitespace(char *s)
{
	int loc = strlen(s);
  loc--;
	while (loc > 0)
	{
		if (s[loc] == ' ')
		{
			s[loc] = (char)NULL;
			loc--;
		}
		else loc = 0;
	}
	return;
}
