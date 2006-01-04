#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> /* itoa */
#include "ashxp.h"

char* create_ofile(char* file) {
  int len;  /* length of input file */
  int ret;  /* return value */
  char tempfile[] = "/tmp/ashxpXXXXXX"; /* template for temporary file */

/* TODO: use mkstemp() if we have it, then tmpnam() if we have it */
/* and finally just create our own I guess.  Use autoconf's defined */
/* macros HAVE_MKSTEMP, HAVE_TMPNAM, etc */

  if (arguments.temp) {
    ret = mkstemp(tempfile);
    if (ret < 0) {
      printf("ERROR: failed to create temporary file with template %s\n",tempfile);
      exit(1);
      }
    if (arguments.verbose) printf("creating temporary file %s\n",tempfile);
    
    /* copy local variable into one passed into this funtion */
    strcpy(file, tempfile);
  } else {
  
    file = basename(file);
    len = strlen(file);
    if ( file[len-4]=='.' &&
         file[len-3]=='c' && 
         file[len-2]=='d' && 
         file[len-1]=='f'   ) {
       file[len-4]='\0';
    } else if 
       ( file[len-3]=='.' &&
         file[len-2]=='n' &&
         file[len-1]=='c'   ) {
       file[len-3]='\0';
    }
    strcat(file,".jpg");

  /* check whether we are overwriting an existing file */
  if(! access(file, F_OK) && 
     ! arguments.quiet ) printf("%s exists, overwriting.\n",file);
  }
  
  /* add file name to report */
  if (arguments.report) {
    strcat(arguments.rpt_txt,"ofile:");
    strcat(arguments.rpt_txt,file);
    strcat(arguments.rpt_txt," ");
    }
    
  return file; 
  }

