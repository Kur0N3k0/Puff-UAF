// adapted from the AFWA version by DS Tillman
#include <iostream>
#include <string>
#include <cstdio> /* fopen */
#include "volc_utils.h"
#include "puff_options.h"

#define MAXPATHLENGTH 255

extern Argument argument;

// Function to read volcano list at run-time

// Returns number of volcanos read; exits on error.
// "volcList" is allocated by the calling routine and
// its max size passed in.

// Locations are reported in terms of North Latitude
// and East Longitude (-180 < lon <= 180)

int readVolcList(VOLCANO_DATA volcList[], int maxListSize) {
   
	double tempLat;
	double tempLon;

   char volcFilename[MAXPATHLENGTH + 1] = "";
   char buffer[MAXLINELENGTH + 1];
   FILE *volcFilePtr;
   
//   double tempLat;
//   double tempLon;
   
   char latDir[MAXVOLCTEXTLEN + 1] = {""};
   char lonDir[MAXVOLCTEXTLEN + 1] = {""};
   
   int volcCount = 0;

   // use env variable if set
   if (getenv("PUFF_VOLCANO_LIST") ) {
     strncpy(volcFilename,getenv("PUFF_VOLCANO_LIST"), MAXPATHLENGTH+1 );
     }
     
//   strncpy(volcFilename, arg.str(volcPath), MAXPATHLENGTH + 1);
   // override filename with command-line argument if given
   if ( argument.volcFile ) {
     strncpy(volcFilename, argument.volcFile, MAXPATHLENGTH + 1);
     }
   if (strlen(volcFilename) == 0 ) {
     if(!argument.quiet && !argument.silent) {
     puts("WARNING: A volcano listing has not been specified and name");
     puts("lookup will not be possible.  Either set the environment");
     puts("variable PUFF_VOLCANO_LIST or use the command-line option");
     puts("\"-volcFile\".\n");
     }
     return 0;
     }
     
// fixme - use a c++ file opening protocol
   if ( (volcFilePtr = fopen(volcFilename, "r")) == NULL ) {
      fprintf(stderr, 
            "Error opening volcano list file %s - terminating\n",
            volcFilename);
      exit(-1);
   }

   // read in volcano listing header in case a 'bad' file is specified to
   // eliminate seg faults later on
   fgets(buffer, MAXLINELENGTH, volcFilePtr);
   if (strncmp(buffer, "Puff Volcano Listing File",25) ) {
     printf("ERROR: \"%s\" is not a valid volcano listing file (missing header)\n",
            volcFilename);
     exit(2);
     }
     
   while ((fgets(buffer, MAXLINELENGTH, volcFilePtr) != NULL) &&
			   (strlen(buffer) > 40)) { // Try to eliminate blank lines at eof
      
      if (volcCount == maxListSize) {
         fprintf(stderr,"Error: Number of volcano records exceeds"
                        " memory allocation (%d)\n", maxListSize);
			exit(-1);
      }
      
      strncpy(volcList[volcCount].name,     strtok(buffer, ":"), MAXVOLCTEXTLEN + 1);	
      strncpy(volcList[volcCount].location, strtok(NULL, ":"), MAXVOLCTEXTLEN + 1);
            
      tempLat = strtod(strtok(NULL, ":"), NULL);
      strncpy(latDir, strtok(NULL, ":"), MAXVOLCTEXTLEN + 1);
      
      tempLon = strtod(strtok(NULL, ":"), NULL);
      strncpy(lonDir, strtok(NULL, ":"), MAXVOLCTEXTLEN + 1);
      
      volcList[volcCount].elevation = strtod(strtok(NULL, ":"), NULL);
      
      
      // Translate South Latitudes and West Longitudes to
      // North Latitudes and East Longitudes, if necessary
      
      if (strncmp(latDir, "S", 1) == 0) {
         volcList[volcCount].lat = -tempLat;
      } else {
         volcList[volcCount].lat = tempLat;
      }
      
      
      if (strncmp(lonDir, "W", 1) == 0) {
         volcList[volcCount].lon = -tempLon;
      } else {
         volcList[volcCount].lon = tempLon;
      }
      

		// Test last position in each string for a null character.
		// If there isn't one, it means the input was longer than the
		// space available.  "strncpy" pads extra space in the receiving
		// end with null characters if the source is shorter than the
		// destination, but in the reverse case, the string will end
		// in a truncated fashion without null termination.

		if (volcList[volcCount].name[MAXVOLCTEXTLEN] != '\0') {
			fprintf(stderr,"Error: Volcano name in record %d"
			" exceeds max allowable length %d.\n", volcCount, MAXVOLCTEXTLEN);
			exit(-1);
		}

		if (volcList[volcCount].location[MAXVOLCTEXTLEN] != '\0') {
			fprintf(stderr,"Error: Volcano location in record %d"
			" exceeds max allowable length %d.\n", volcCount, MAXVOLCTEXTLEN);
			exit(-1);
		}

      
      volcCount++;
   }
   
   fclose(volcFilePtr);
   
   return volcCount;
}
      
   
   
