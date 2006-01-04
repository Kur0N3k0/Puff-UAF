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
#include <vector>
#include <string>
#include <fstream>
#include <cstdio>
#include "datetime.h"

//////////////////////////////////////////////////////////////////////
//
// This routine takes a string in unidata format, "YYYY MM DD HH:MM" 
// and converts it into the standard C time_t variable (time.h)
// The time_t variable is put into UTC COORDINATES!
//
///////////////////////////////////////////////////////////////////////
time_t unistr2time(const char *unistr) {
//    extern long timezone;
    struct tm date;
    
    if( sscanf(unistr, "%d %d %d %d:%d", &date.tm_year, &date.tm_mon, &date.tm_mday, &date.tm_hour, &date.tm_min) != 5 ) {
      cerr << "ERROR: date/time string " << unistr << "is not valid\n";
      return (time_t)NULL;
      }
    
    // die if pre-epoch (1970-1-1 UTC) until fix the fact that mktime() below
    // won't do it
    if (date.tm_year < 1970)
    {
      std::cerr << "ERROR: Pre-1970 eruptions are not currently supported\n";
      exit(0);
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
//    setenv ("TZ", "UTC0UTC",1);
//    tzset();

    time_t time;
    if ( (time = mktime(&date)) == -1 ) {
        cerr << endl;
        cerr << "ERROR: unistr2time() : Invalid date string \"" 
             << unistr << "\"" 
             << endl;

	return 0;
    }

    return (time);

}

//////////////////////////////////////////////////////////////////////
//
// This routine takes a standard C time_t variable in UTC and converts
// it into a string in unidata format, "YYYY MM DD HH:MM" in UTC 
//
///////////////////////////////////////////////////////////////////////
char *time2unistr(time_t time) {
    static char *str = new char[18];
    for (int i=0; i<18; i++) str[i] = '\0';
    
    static struct tm *tmnow;

    // Call tzset to correct lof local timezone:
//    tzset();

//    time += timezone;

    tmnow = gmtime(&time);
    if (tmnow->tm_isdst) (tmnow->tm_hour)--;
    strftime(str, 17, "%Y %m %d %H:%M", tmnow);

    return str;
}

//////////////////////////////////////////////////////////////////////
//
// This routine takes a string in unidata format, "YYYY MM DD HH:MM" 
// and converts it into julian date string "JJJ"
//
///////////////////////////////////////////////////////////////////////
char *unistr2jstr(char *unistr) {
//    extern long timezone;
    static time_t unitime;
    static char *str = new char[5];

    for (int i=0; i<5; i++) str[i] = '\0';

    unitime = unistr2time(unistr);

    static struct tm *tmnow = new  tm;

    // this is not necessary since we only use UTC.  Anyway, unless 'timezone'
    // is declared, it won't be set properly.
    // Call tzset to set correct timezone offset from UTC:
//    tzset();

//    unitime += timezone;

    tmnow = gmtime(&unitime);
    if (tmnow->tm_isdst) (tmnow->tm_hour)--;
    strftime(str, 17, "%j", tmnow);

    return str;
}
//////////////////////////////////////////////////////////////////////
//
// This routine takes a string in unidata file format, "YYMMDDHH" 
// and converts it a time_t variable
//
///////////////////////////////////////////////////////////////////////
time_t unifilestr2time(char *unistr) {

//    extern long timezone;
    struct tm date;
    char *ttemp = "\0\0\0";
    
    // There are no seconds:
    strcpy(ttemp, "00");
    date.tm_sec = atoi(ttemp);
    
    // There are no minutes:
    strcpy(ttemp, "00");
    date.tm_min = atoi(ttemp);
    
    // hours since midnight [0,23]
    strncpy(ttemp, unistr+6, 2);
    ttemp[2] = '\0';
    date.tm_hour = atoi(ttemp);
    
    // day of the month [1,31]
    strncpy(ttemp, unistr+4, 2);
    ttemp[2] = '\0';
    date.tm_mday = atoi(ttemp);
    
    // months since January [0,11] 
    // subtract one since the uni string assumes [1,12]
    strncpy(ttemp, unistr+2, 2);
    ttemp[2] = '\0';
    date.tm_mon = atoi(ttemp) - 1;
    
    // years aince 1900:
    strncpy(ttemp, unistr, 2);
    ttemp[2] = '\0';
    date.tm_year = atoi(ttemp);
    
    // Daylight savings -1 = unknown: LOCAL TIME ZONE IS USED WITH -1
    date.tm_isdst = 0;

    // Call tzset to set correct timezone offset:
//    tzset();

    time_t time;
    if ( (time = mktime(&date)) == -1 ) {
        cerr << endl;
        cerr << "ERROR: unifilestr2time() : Invalid date string \"" 
             << unistr << "\"" 
             << endl;
	return 0;
    }
        
    return (time);

}
//////////////////////////////////////////////////////////////////////
//
// This routine takes a standard C time_t variable in UTC and converts
// it into a string in unidata file format, "YYMMDDHH" in UTC 
//
///////////////////////////////////////////////////////////////////////
char *time2unifilestr(time_t time) {
//    extern long timezone;

    static char *str = new char[10];
    for (int i=0; i<10; i++) str[i] = '\0';
    
    static struct tm *tmnow = new  tm;

    // Call tzset to correct lof local timezone:
//    tzset();

//    time += timezone;

    tmnow = gmtime(&time);
    if (tmnow->tm_isdst) (tmnow->tm_hour)--;
    strftime(str, 9, "%y%m%d%H", tmnow);

    return str;
}

///////////////////////////////////////////////////////////////////////
void Tokenize(const string& str,
              vector<string>& tokens,
              const string& delimiters = " ")
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}
///////////////////////////////////////////////////////////////////////
// try to find a corresponding file by replacing the characters in *repl
// with the characters in *tok, one at a time.  Return that filename as a
// string if it exists, otherwise, return the original string.
// TODO: if *filename has a path and name on it, stop replacing when a '/'
// is encountered and look from the back forward.
///////////////////////////////////////////////////////////////////////
string correspondingFile(string *filename, char *tok, char *repl) 
{
  unsigned int repIdx, tokIdx;  // counters through repl and tok
  int loc;  // this must be an integer to start it at -1.
  string newFile = filename->data();
  
  for (repIdx = 0; repIdx < strlen(repl); repIdx++) { // loop through repl
  for (tokIdx = 0; tokIdx < strlen(tok); tokIdx++) { // loop through tokens
    loc = -1 ; // reset location in string to the beginning
    while (( loc = (int)newFile.find( repl[repIdx], loc+1 ) ) != (int)string::npos) {
      newFile.replace(loc, 1, &(tok[tokIdx]) );
      // see if this is a readable file
      ifstream file;
      file.open(newFile.data(), ios::in);
      if (file) { // found a readable file
        return newFile;
      }
      newFile = filename->data();  // start with a fresh copy
    } // end while
  }  // end token loop
  }  // end repl loop
  // return the original string if the replacement fails
  return "";
}

        
    
