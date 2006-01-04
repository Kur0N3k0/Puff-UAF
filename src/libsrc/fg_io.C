/*
 * -- puff : software for ash tracking and simulation
 *    Copyright (C) 1999 Craig Searcy 
 *                  2001-2003 Rorik Peterson
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
/////////////////////////////////////////////////////////////////////
//
// WRITE SWITCH:
// This the public call, routes to the appropriate type 
// private subroutine
//
/////////////////////////////////////////////////////////////////////
int Grid::write(char *file) {

//  type = CDF;
  return write_cdf(file);
  }
/////////////////////////////////////////////////////////////////////
int Grid::write(std::string *file)
{
  return write_cdf((char*)file->c_str());
}
/////////////////////////////////////////////////////////////////////
//
// READ SWITCH:
// This the public call, routes to the appropriate type 
// private subroutine
//
/////////////////////////////////////////////////////////////////////
int Grid::read(std::string* file) {
//   string file_copy = file->data();  // copy of file to work with
//   int loc = file_copy.find_first_of(":");
//     // look at only the first file in the colon-delimited string
//   if (loc != string::npos) file_copy.erase(loc);
//   //  pseudo-switch by extension
//   if (file_copy.substr(file_copy.length()-4,4) == ".cdf")  {
//     return read_cdf(file);  }
//   else if (file_copy.substr(file_copy.length()-3,3) == ".nc") {
//     return read_cdf(file); }
//   else if (file_copy.substr(file_copy.length()-3,3) == ".pp") {
//     return read_pp(file); }
// 
//   // if we get here, we couldn't determine the filetype by extension
//   cerr << "ERROR: unable to determine data type of \"" << file_copy <<
//        " by extension\n";
  std::cerr << "obsolete function Grid::read()\n";
  return FG_ERROR;
  
 }
  
/////////////////////////////////////////////////////////////////////
// redirection for char* -> string
////////////////////////////////////////////////////////////////////////////
int Grid::read(char *file, const char *eDate, const double runHours) {
  std::string s = file;
  return read_cdf(&s, eDate, runHours);
}
