

#ifndef VOLC_UTILS_H
#define VOLC_UTILS_H


#define MAXVOLCS			2000
#define MAXVOLCTEXTLEN	100
#define MAXLINELENGTH	255

#include <cstring>
#include <cstdlib>

typedef struct {
   char		name[MAXVOLCTEXTLEN + 1];
   char		location[MAXVOLCTEXTLEN + 1];
   double	lat;
   double	lon;
   double	elevation; 
} VOLCANO_DATA;


int readVolcList(VOLCANO_DATA volcList[], 
                 int          maxListSize);



#endif
      
      
	
