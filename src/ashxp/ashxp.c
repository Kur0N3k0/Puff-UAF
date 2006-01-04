#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "my_getopt.h"
#endif
#include "ashxp.h"


int main (int argc, char **argv){
  
  extern int optind; /* argv[] index, used after parsing options */
  int opt_index;  /* personal copy of optind */
  char *file;  /* input filename */
  struct Ash ash;
  struct bg_img bg;

      
  /* set defaults before parsing command line */
  ashxp_set_defaults();
  ashxp_parse_options(argc, argv);
  
  opt_index = optind;
    
  /* get bounding box by finding min/max lon/lat/hgt 
   * do this even if the bounding box is set because the min/max height 
   * is determined here as well
   */
  get_bbox(argv, opt_index, &ash, &bg);
  
  /* re-set lateral bounding box if user-specified */
  if (arguments.llstr) {
    set_bbox(arguments.llstr,&bg);
    }
  
  /* clip out the portion of bgimage we need */
  clip_bg(&bg);
  
  /* loop through all file remaining on the command line */
  while (argv[optind] != NULL ) {
    /* make a working copy of tbe file named to process */
    file = (char*)calloc(strlen(argv[optind])+1,sizeof(char));
    strcpy(file, argv[optind]);
    
    (void)read_ash(&ash, file);
    
    if (!arguments.output_file ) 
    {
      /* allocate space for output file, allowing for a new suffix */
      arguments.output_file = (char*)calloc(strlen(file)+5,sizeof(char));
      strcpy(arguments.output_file, create_ofile(file) );
    }   
    frame(&ash, &bg);
    
    /* advance the counter to process the next file */
    optind++;
    /* free space holding file names */
    free(file); 
    free(arguments.output_file);
    arguments.output_file = 0x0;
    }
    
  /* generate report if requested */
  if (arguments.report) puts(arguments.rpt_txt);
  free(bg.rgb);  
  return 0;
  
  }
