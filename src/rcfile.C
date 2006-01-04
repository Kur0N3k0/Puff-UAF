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
#include <string>
#include <list>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdlib>	// getenv()
#include <cstdio>  // sprintf()
#include <ctime>  // time_t
#include <cmath>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "rcfile.h"

extern void Tokenize(const std::string&, 
                     std::vector<std::string>&, 
		     const std::string&);
		     
extern time_t unistr2time(char*);  // from utils.C
extern char* time2unistr(time_t);  // from utils.C

///////////////////////////////////////////////////////////////////////
PuffRC::PuffRC() {
  fileName = new char[256];
  // set some defaults
  dataPath = "./";
  fourDdata = true;
  varUname = "u";
  varVname = "v";
  varZname = "Z";
  return;
  }
  
///////////////////////////////////////////////////////////////////////
std::string PuffRC::cleanString(std::string str) {
  while (str.substr(0,1) == " ") {
    str=str.substr(1,str.length()-1);
    }
  // remove whitespace at end
  while(str.substr(str.length()-1,1) == " ") {
    str=str.substr(0,str.length()-1);
    }
  return str;
  }
///////////////////////////////////////////////////////////////////////
PuffRC::~PuffRC() {
  return;
  }
///////////////////////////////////////////////////////////////////////
// initialize by finding a resource file
///////////////////////////////////////////////////////////////////////
bool PuffRC::init(char* rcfileArg) {
  int ret = -1;  // return code from finding file status
  struct stat fileStatus;  // structure storing file status

  // check specified resources file if given
  if (rcfileArg) {
    fileName = strdup(rcfileArg);
    ret = stat(rcfileArg, &fileStatus);
    if (ret == 0) return true;
    }
  
  // check the current directory first
  if (ret == -1) {
    strcpy(fileName, "./puffrc");
    ret = stat(fileName, &fileStatus);
    if (ret == 0) return true;
    }

  // check home directory next
  if (ret == -1) {
   strcpy(fileName,getenv("HOME") );
   strcat(fileName,"/.puffrc");  
   ret = stat(fileName, &fileStatus);
   if (ret == 0) return true;
   }

  // check global next
  if (ret == -1) {
    strcpy(fileName, "/usr/local/puff/etc/puffrc");
    ret = stat(fileName, &fileStatus);
    if (ret == 0) return true;
    }

  // leave if there is still no file found
  if (ret == -1) fileName = (char)NULL;
  
  return false;
}  

///////////////////////////////////////////////////////////////////////
// check for, and load resources
///////////////////////////////////////////////////////////////////////
int PuffRC::loadResources(char *modelArg, const char *type) {
  RCType rcType;
  int lineNumber = 0;
  
  // open rcfile, we already know it exists from PuffRC::init()
  std::ifstream rcFile(fileName, std::ios::in);
  
  // set type of resource we are looking for
  if (strncmp(type, "model=", 6) == 0) rcType = PUFFRC_MODEL;
  if (strncmp(type, "dem=", 5) == 0) rcType = PUFFRC_DEM;
  
  // begin parsing 
  char *line = new char[1024];	// buffer to hold each line from file 
  std::string text;  // string copy of the buffer; easier to parse
  std::string param, value, tmpMask;
  
  while (rcFile.getline(line, 1024, '\n') != NULL) {
    //  read the next line if it is a continuation of the previous
    while (line[strlen(line)-1] == '\\') {
      // create another buffer
      char *nextLine = new char[1024];
      // read the line in
      rcFile.getline(nextLine, 1024, '\n');
      // replace the '\' with a space
      line[strlen(line)-1] = ' ';
      // add a return to mark the end of the line
      line[strlen(line)] = '\0';
      // concantenate without overwriting the buffer
      strncat(line,nextLine,1024-strlen(line));
      }
    lineNumber++;
    text = line;
    // skip if not "model=..."
    param=type;
    param.append(modelArg);
    // append a space so two models do not have similar name (i.e avn
    // and avn-anl).
    param.append(" "); 
    if (text.find(param) == std::string::npos) continue;
    // tokenize the line
    std::vector<std::string> valuePairs;
    Tokenize(text, valuePairs, " ");
    // iterate through the vector of param/value pairs
    std::vector<std::string>::const_iterator p;
    for(p=valuePairs.begin(); p!=valuePairs.end(); ++p) {
      param = (*p).substr(0,(*p).find_first_of("="));
      value = (*p).substr((*p).find_first_of("=")+1,(*p).length());
      switch(rcType) {
        case PUFFRC_MODEL:
        // pseudo-switch on param
        if(param == "model") 
	{ 
	  modelName=value; 
	} else if(param == "path") { 
	  dataPath=value; 
	} else if(param == "mask") {
	  tmpMask=value; 
	} else if(param == "record") { 
	  if (value == "yes" || value == "true" ||
	      value == "Yes" || value == "True" ||
	      value == "YES" || value == "TRUE" )  
	  { 
	    fourDdata = true;
	  } else if (value == "no" || value == "false" ||
	             value == "No" || value == "False" ||
		     value == "NO" || value == "FALSE") { 
            fourDdata = false;
	  } else {
	    std::cerr << "unknown parameter/value pair: " << param <<"="<<value
	    << "\n Value should be \"yes\" or \"no\"" << std::endl;
	  }
	} else if(param == "var") {
          if (value.find("T") != std::string::npos) Tmask=tmpMask;
          if (value.find("u") != std::string::npos) umask=tmpMask;
          if (value.find("U") != std::string::npos) umask=tmpMask;
          if (value.find("v") != std::string::npos) vmask=tmpMask;
          if (value.find("V") != std::string::npos) vmask=tmpMask;
          if (value.find("z") != std::string::npos) zmask=tmpMask;
          if (value.find("Z") != std::string::npos) zmask=tmpMask;
	} else if(param == "varTname") {
	  varTname = value;
        } else if(param == "varUname") {
	  varUname = value;
	} else if(param == "varVname") {
	  varVname = value;
	} else if(param == "varZname") {
	  varZname = value;
	} else {
	  std::cerr << "ERROR: bad resource file specification\n"
	  << "unknown parameter value: " << param << " on line "
	  << lineNumber << std::endl;
	  return 1;
	  }  
	break;
	case PUFFRC_DEM:
	// pseudo-switch on param
	if(param == "dem") 
	{
	  demName=value;
	} else if(param == "path") {
	  demPath=value;
	} else {
	  std::cerr << "ERROR: bad resource file specification\n"
	  << "unknown parameter value: " << param << " on line "
	  << lineNumber << std::endl;
	  return 1;
	  }  
	break;
	default:
	break;
        }
      }

    } // end while reading file
    
  rcFile.close();
  
  delete[] line;

  // append '/' to path if necessary
  if (dataPath[dataPath.length()-1] != '/') dataPath.append("/");  
  if (modelName.length() == 0) {
    std::cerr << "ERROR: could not find resource information for model " << 
             modelArg << " in file \"" << fileName << "\"" << std::endl;
    return 1;
    }
    
  return 0;
}
///////////////////////////////////////////////////////////////////////
std::string PuffRC::getMask(char *var) {
  if (strcmp(var, "T") == 0) return Tmask;
  if (strcmp(var, "u") == 0) return umask;
  if (strcmp(var, "v") == 0) return vmask;
  if (strcmp(var, "z") == 0) return zmask;
  return "";
  }
  
///////////////////////////////////////////////////////////////////////
// return the type of dem specified in the RC file
const char *PuffRC::demType() {
  return demName.c_str();
  }
///////////////////////////////////////////////////////////////////////
const char *PuffRC::getDemPath() {
  return demPath.c_str();
  }  
///////////////////////////////////////////////////////////////////////
// find most recent data files using model masks and looking in path
// eruption date is stored in edate
///////////////////////////////////////////////////////////////////////
const std::string PuffRC::mostRecentFile(const char *edate, char *var, double runHours) {
  std::string retFile;  // most recent possible return file
  std::string eDateStr = edate;  // eruption date as string
  std::list<std::string> fileList;  // possible matching files
  std::string mask = getMask(var);  // local copy of file mask

  // if no mask specified, we cannot find a file, so return NULL
  if (mask.length() < 1) return "";
  
  // create a more general mask where Y,M,D,H are all replaced with a control
  // character which we ignore when looking for possible file matches.  This
  // will check for literal Y,M,D,H's and not replace those
  std::string genMask = mask;
  for (int i=0; i<(int)genMask.length();i++)
  {
    if (genMask[i] == '\\') 
    {
      genMask.erase(i,1);
    } else if (genMask[i] == 'Y' || genMask[i] == 'M' || genMask[i] == 'D'
            || genMask[i] == 'H') 
    {
      genMask[i] = '\a';
    }
  }
      
  // get listing of all file in path
  DIR *dp;
  dp = opendir(dataPath.c_str());
  if (dp == NULL) { 
    std::cerr << "ERROR: could not open " << dataPath << " directory\n";
    (void)closedir(dp);
    return "";
    }
  // loop through all files
  struct dirent *ep;  // pointer to directory entries
  while (( ep = readdir(dp) )) {
    std::string testFile = ep->d_name;
    bool valid = true;  // valid filename that matches the mask
    // check that the length is correct
    if (testFile.length() != genMask.length()) continue;
    // check that the mask matches
    for (int i=0; i < (int)genMask.length(); i++) {
      if ( genMask[i] != '\a' && genMask[i] != testFile[i])
      {
	// mark this file invalid
	valid = false;
	// quit comparing file to the mask since this file is invalid
	i = (int)genMask.length();
      }

      }
     // add this file to the list of possible
     if (valid) {
       fileList.push_back(testFile);
       }
    }
  (void)closedir(dp);
  
  // sort the remaining files from newest to oldest
  fileList.sort();
  fileList.reverse();

  

  // set up retFile
  retFile = "";
    
    std::string year = eDateStr.substr(0,4);
    std::string month = eDateStr.substr(5,2);
    std::string day = eDateStr.substr(8,2);
    std::string hour = eDateStr.substr(11,2);

    // create a possible filename based on eruption date and matches the mask
    // determine if this is a 2 or 4 digit year specification 
    std::string::size_type loc; // location in string
    
    loc = findNonLiteral(&mask,"Y");
    
    int nYear = 0;  // 2 or 4 year specification
    // count the size of the year specification
    while( loc  != std::string::npos ) 
    { 
      nYear++;
      loc = mask.find("Y", loc+1);
    }

    if (nYear == 2) year=year.substr(2,4);
      
    // create the most recent possible return file
    std::string testFile = mask;
    loc = findNonLiteral(&mask,"Y");
    if (loc != std::string::npos) testFile.replace(loc, nYear, year);
    loc = findNonLiteral(&mask,"M");
    if (loc != std::string::npos) testFile.replace(loc, 2, month);
    loc = findNonLiteral(&mask,"D");
    if (loc != std::string::npos) testFile.replace(loc,2,day);
    loc = findNonLiteral(&mask,"H");
    if (loc != std::string::npos) testFile.replace(loc,2,hour);

    //  remove the escape characters from testFile
    for (int i = 0; i<(int)testFile.length(); i++)
    {
      if (testFile[i] == '\\') testFile.erase(i,1);
    }
    
    // iterate through all possible files, returning the first that is less than
    // the most-recent possible
    std::list<std::string>::iterator fp;
    bool fileFound = false;
    for (fp=fileList.begin(); (fp!= fileList.end() && !fileFound); ++fp) {
      if (testFile >= *fp) {  // found the file we want
        retFile.append(dataPath);
        retFile.append(*fp);
	// remove this file and all files older than it
	fileList.erase(fp,fileList.end());
	fileFound = true;
	// add a file seperator if multiple files will be returned
        }
      }
    
    // if a file was not found the first time through, data is not available
    if (retFile == "" && !fileFound) {
      std::cout << "WARNING: no \"" << var << "\" data available in " <<
        dataPath << " prior or equal to " << edate << std::endl;
      return (char*)retFile.c_str();
      }
    
  return retFile;
  }
  
///////////////////////////////////////////////////////////////////////
// find the first character 'c' which is not preceeded with the
// escape character '\' and return the location
///////////////////////////////////////////////////////////////////////
int PuffRC::findNonLiteral(const std::string *mask, const char *c)
{
  int loc = mask->find(c);  // location in string
  // skip escaped literals
  while( (*mask)[loc-1] == '\\')
  {
    loc = mask->find(c,loc+1);
    if (loc == (int)std::string::npos) badMask(mask);
  }
  return loc;
}

///////////////////////////////////////////////////////////////////////
// error routine, terminates program
///////////////////////////////////////////////////////////////////////
void PuffRC::badMask(const std::string *mask) 
{
  std::cerr << "ERROR: bad mask specification:  " << *mask << std::endl;
  exit(1);
  return;
}  
///////////////////////////////////////////////////////////////////////
// return parameter value as a string
///////////////////////////////////////////////////////////////////////
std::string PuffRC::getString(const std::string *p)
{
  // get a local copy of this as a string
  std::string param = *p;
  
  if (param == "varU")
  {
    return varUname;
  } else if (param == "varV") {
    return varVname;
  } else if (param == "varZ") {
    return varZname;
  } else {
    std::cerr <<"ERROR: PuffRC::getString() does not recognize parameter "
    << param << std::endl;
    return (std::string)"";
  }
}
///////////////////////////////////////////////////////////////////////
// redirect
///////////////////////////////////////////////////////////////////////
std::string PuffRC::getString(char *p)
{
  std::string s;
  // deal with NULL pointer
  if (!p)
  {
     s= "";
  } else {
    s = p;
  }
  return getString(&s);
}
  
  
  
  
