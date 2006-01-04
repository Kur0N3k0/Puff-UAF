#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <string.h>	/* strcat */
#include <unistd.h>	/* access() */

#include "ashxp.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "my_getopt.h"
#endif

/* get the version number via autoconf and config.h */
static const char* ashxp_version_number=VERSION;

/* local functions */
void parse_label_file(char *);

void ashxp_set_defaults(struct Ash *ash) {
  char *env;  /* pointer to environmental variables */
  
    /* Default values. */
  arguments.airborne = 0;
  env = (char*)getenv("ASHXP_BG_FILE_NAME");
  if (!env) env = (char*)getenv("ASHXP_BG_FILENAME");
  if (env) 
  {
    arguments.bgfile = strdup(env);
  } else {
    arguments.bgfile = 0x0;
  }
     
  arguments.border = 1;
  arguments.color = "gray";
	arguments.colorbar_size = 0x0;
  arguments.fallout = 0;
  /* autoconf should have defined this macro */
  arguments.fontfile = strdup(ASHXP_FONTFILE);
  arguments.fontsize = 12;
  arguments.grayscale = 0;
  arguments.hgtmax = -1.0;
  arguments.include_out_of_bounds = 0;
	arguments.labelv = (char**)calloc(100,sizeof(char*));
	arguments.labelc = 0;
	arguments.labelfile = 0x0;
  arguments.llstr = 0x0;
  arguments.magnify = 0;
  arguments.minsize = 256;
  arguments.mpeg = 0;
  arguments.nobg = 0;
  arguments.output_file = 0x0;
  arguments.patch = PATCH_NO;
  arguments.print_datetime_stamp = 1;
  arguments.projection = MERCATOR;
  arguments.quiet = 0;
  arguments.range = 0x0;
  arguments.report = 0;
  arguments.sizemin = 0x0;
  arguments.sizemax = 0x0;
  arguments.sorted = 0;
  arguments.temp = 0;
  arguments.verbose = 0;
  arguments.pixels = 2;
  arguments.whole = 0;
  arguments.xgridline_pixels = 1;
  arguments.xgridlines = -1;
  arguments.ygridline_pixels = 1;
  arguments.ygridlines = -1;
  
}

void ashxp_parse_options(int argc, char** argv)
{
  int opt;
  char * opt_sng;
  extern char *optarg;
  static const char *color_options = "gray red rainbow isopach default";
  static struct option opt_lng[] = {
    {"airborne",no_argument,0,'a'},/*only display airborne particles*/
    {"background",required_argument,0,'b'},/*background JPEG image*/
    {"bgfile",required_argument,0,'b'},/*background JPEG image*/
    {"border",required_argument,0,'B'},/*minimum border pixels*/
    {"color",required_argument,0,'c'},/*coloring of cloud*/
		{"colorbar-size",required_argument,0,135},/*x&y size of colorbar*/
    {"debug",no_argument,0,'D'},/*print debugging information*/
    {"fallout",no_argument,0,'f'},/*only display grounded particles*/
    {"fontfile",required_argument,0,133},/*location of font file*/
    {"font-file",required_argument,0,133},/*location of font file*/
    {"fontsize",required_argument,0,'F'},/*font size in pixels*/
    {"font-size",required_argument,0,'F'},/*font size in pixels*/
    {"grayscale",no_argument,0,'g'},/*make background grayscale*/
    {"gridlines",required_argument,0,'G'},/*degrees per gridline*/
    {"hgt-range",required_argument,0,'r'},/*filter this height range*/
    {"height-range",required_argument,0,'r'},/*filter this height range*/
    {"help",no_argument,0,'?'},/* show help */
    {"include-out-of-bounds",no_argument,0,129},/*plot out-of-bounds ash*/
    {"magnify",required_argument,0,'m'},/*magnify image*/
    {"max-height",required_argument,0,'h'},/*maximum height for color-coding*/
		{"label",required_argument,0,'L'},/*label text on background*/
		{"label-file",required_argument,0,134},/*labels in this file*/
		{"labelfile",required_argument,0,134},/*labels in this file*/
		{"labels-file",required_argument,0,134},/*labels in this file*/
		{"labelsfile",required_argument,0,134},/*labels in this file*/
    {"lonlat",required_argument,0,132},/* set lonmin/lonmax/latmin/latmax*/
    {"latlon",required_argument,0,'l'},/*set latmin/latmax/lonmin/lonmax*/
    {"no-stamp",no_argument,0,128},/*do not print a date/time stamp*/
    {"output",required_argument,0,'o'},/*Output to this file*/
    {"patch",optional_argument,0,130},/*try to patch projected image*/
    {"pixels",required_argument,0,'p'},/*number of pixels in ash particles*/
    {"projection",required_argument,0,'P'},/*projection type*/
    {"quiet",no_argument,0,'q'},/*Don't produce any output*/
    {"report",no_argument,0,'R'},/*produce report*/
    {"size",required_argument,0,131},/*plot only this size range*/
    {"size-range",required_argument,0,131},/*plot only this size range*/
    {"sorted",no_argument,0,'s'},/*sort ash particles before plotting*/
    {"sort",no_argument,0,'s'},/*sort ash particles before plotting*/
    {"temp",no_argument,0,'t'},/*use temporary file for output*/
    {"timestamp",no_argument,0,'T'},/*print a timestamp*/
    {"usage",no_argument,0,'u'},/*show usage message */
    {"verbose",no_argument,0,'v'},/*Produce verbose output*/
    {"version",no_argument,0,'V'},/*version */
    {"whole",no_argument,0,'w'},/*use whole image*/
    {"min-size",required_argument,0,'x'},/*minimum pixel dimensions*/
    {"no-background",no_argument,0,'X'},/*do not use background image*/
    {0,0,0,0}
    }; /* end opt_lng */
    int opt_idx=0; /* Index of current long option into opt_lng array */
  /* Parse our arguments; every option seen by parse_opt will
     be reflected in arguments. */
  opt_sng = "ab:B:c:gG:F:fh:l:m:o:P:p:qr:Rstvwx:XV";
  while((opt = getopt_long(argc,argv,opt_sng,opt_lng,&opt_idx)) != EOF){
  switch (opt)
    {
    case 'a':
      if (arguments.fallout)
      {
        puts("ERROR: cannot specify both \"--airborne\" and \"--fallout\"\n");
	exit(1);
      }
      arguments.airborne = 1;
      break;
    case 'b':
			/* if this is already set, free it first */
			if (arguments.bgfile) free(arguments.bgfile);
      arguments.bgfile = strdup(optarg);   
      break;
    case 'B':
      arguments.border = atoi(optarg);
      strcpy(arguments.llstr, "");
      break;
    case 'c':
      if (strcmp(optarg, "red") == 0 ) coloring_scheme=COLOR_PROTOCOL_RED;
      else if (strcmp(optarg, "default") == 0 ) coloring_scheme=COLOR_PROTOCOL_DEFAULT;
      else if (strcmp(optarg, "gray") == 0 ) coloring_scheme=COLOR_PROTOCOL_GRAY;
      else if (strcmp(optarg, "grey") == 0 ) coloring_scheme=COLOR_PROTOCOL_GRAY;
      else if (strcmp(optarg, "rainbow") == 0 ) coloring_scheme=COLOR_PROTOCOL_RAINBOW;
      else if (strcmp(optarg, "isopach") == 0 ) coloring_scheme=COLOR_PROTOCOL_ISOPACH;
      else {
        printf("unknown color: %s\n",optarg);
        printf("options are: %s\n",color_options);
        exit(EXIT_FAILURE);
	}
      break;
    case 'D':
#define DEBUG(x,y) printf(x,y);
      break; 
    case 'F':
      if (sscanf(optarg, "%i", &arguments.fontsize) != 1) 
      {
        printf("bad options --fontsize=\"%s\".  Value must be an integer, using default value.\n",optarg);
      }
      if (arguments.fontsize < 1)
      {
        puts("ERROR: font size must be positive.");
	exit(1);
      }
      break;
    case 'f':
      if (arguments.airborne)
      {
        puts("ERROR: cannot specify both \"--airborne\" and \"--fallout\"\n");
	exit(1);
      }
      arguments.fallout = 1;
      break;
    case 'g':
      arguments.grayscale = 1;
      break;
    case 'G':
      gridline_color = GRIDLINE_WHITE;
      if (strstr(optarg,"black")) gridline_color = GRIDLINE_BLACK;
      if (strstr(optarg,"blue")) gridline_color = GRIDLINE_BLUE;
      if (strstr(optarg,"green")) gridline_color = GRIDLINE_GREEN;
      if (strstr(optarg,"purple")) gridline_color = GRIDLINE_PURPLE;
      if (strstr(optarg,"red")) gridline_color = GRIDLINE_RED;
      if (strstr(optarg,"white")) gridline_color = GRIDLINE_WHITE;
      if (strstr(optarg,"yellow")) gridline_color = GRIDLINE_YELLOW;
      if (sscanf(optarg,"%f:%ix%f:%i",
        &arguments.xgridlines,&arguments.xgridline_pixels,
        &arguments.ygridlines,&arguments.ygridline_pixels) > 3)
      {
        break;
      } else if (sscanf(optarg,"%fx%f",
        &arguments.xgridlines,&arguments.ygridlines) > 1) {
	break;
      } else if (sscanf(optarg,"%f:%i",&arguments.xgridlines,
		 &arguments.xgridline_pixels) > 0) {
	arguments.ygridlines = arguments.xgridlines;	
	arguments.ygridline_pixels = arguments.xgridline_pixels;
	break;
      } else {
        printf("ERROR: bad gridline specification: \"%s\"\n",optarg);
	puts("Use --gridlines=DG[:INT][COLOR] or --gridlines=DG[:INT]xDG[:INT][COLOR]");
	puts("where DG is degrees per gridlines and INT is pixel thickness");
	exit(0);
      }
      break;
    case 'h':
      arguments.hgtmax = (float)atof(optarg);
      break;
		case 'L':
			arguments.labelv[arguments.labelc] = strdup(optarg);
			arguments.labelc++;
			break;
    case 'l':
      arguments.llstr = (char*)strdup(optarg);
      arguments.border = 0;
      break;
    case 'm':
      if (sscanf(optarg,"%i",&arguments.magnify) != 1)
      {
        printf("ERROR: invalid argument for --magnify option: \"%s\"\n",optarg);
	exit(0);
      }
      break;
    case 'o':
      arguments.output_file = (char*)strdup(optarg);
      break;
    case 'P':
      /* check just enough letters in the string to be unambiguous */
      if (strncmp(optarg,"lam",3) == 0) /* lambert conformal */
      {
        arguments.projection = LAMBERT;
      } else if (strncmp(optarg,"mer",3) == 0) {
        arguments.projection = MERCATOR;
      } else if (strncmp(optarg,"pol",3) == 0) {
        arguments.projection = POLAR;
      } else {
        printf("ERROR: unrecognized projection: %s.\nOptions are: cylindrical, lambert, mercator, polar\n",optarg);
	exit(1);
      }
      break;
    case 'p':
      arguments.pixels = atoi(optarg);
      break;
    case 'q':
      arguments.quiet = 1;
      break;
    case 'r':
      arguments.range = strdup(optarg);
     break;
    case 'R':
      arguments.report = 1;
      /* create a large buffer for the report */
      arguments.rpt_txt = calloc(2000,sizeof(char));
      break;
    case 's':
      arguments.sorted = 1;
      break;
    case 'T':
      arguments.print_datetime_stamp = 1;
      break;
    case 't':
      arguments.temp = 1;
      break;
    case 'v':
      arguments.verbose = 1;
      break;
    case 'w':
      arguments.whole = 1;
      break;
    case 'X':
      arguments.nobg = 1;
      break;
    case 'x':
      arguments.minsize = atoi(optarg);
      break;
    case 'u':
      ashxp_usage();
      exit(EXIT_SUCCESS);
      break;
    case 'V':
      printf("%s\n",ashxp_version_number);
      exit(EXIT_SUCCESS);
      break;
    case '?':
      show_help();
      exit(EXIT_SUCCESS);
      break;
    case 128:
      arguments.print_datetime_stamp = 0;
      break;
    case 129:
      arguments.include_out_of_bounds = 1;
      break;
    case 130: /* --patch */
      if (optarg)
      {
        if (strcmp(optarg,"yes") == 0)
	{
	  arguments.patch = PATCH_YES;
	} else if (strcmp(optarg,"no") == 0 ) {
	  arguments.patch = PATCH_USER_NO;
	} else {
	  printf("ERROR: value for --patch option: \"%s\"\n",optarg);
	  exit(0);
	}
      } else {
        arguments.patch = PATCH_YES;
      }
      break;
    case 131: /*--size */
      if (sscanf(optarg, "%f/%f",&arguments.sizemin, &arguments.sizemax) != 2)
      {
        printf("ERROR: Invalid option value: --size=%s\n",optarg);
	printf("Usage: --size=min/max [mm]\n");
	exit(EXIT_FAILURE);
      }
      arguments.sizemin /= 1000.;
      arguments.sizemax /= 1000.;
      break;
    case 132: /* --lonlat */
      arguments.llstr = (char*)calloc(strlen(optarg)+2,sizeof(char));
      strcpy(arguments.llstr, optarg);
      /* signal that lat/lon are switched */
      strcat(arguments.llstr, "!");
      arguments.border = 0;
      break;
    case 133: /* -fontfile */
      if ( access(optarg, R_OK))
      {
        printf("Cannot read fontfile \"%s\"\n",optarg);
      } else {
        free(arguments.fontfile);
	arguments.fontfile = strdup(optarg);
      }
      break;
		case 134: /* --label-file */
			parse_label_file(optarg);
			break;	
		case 135: /* --colorbar-size */
		  arguments.colorbar_size = strdup(optarg);
			break;
    default:
      printf("unrecognized option: %s\n",optarg);
      exit(1);
      break;
    } /* end switch */
    } /* end while loop */
}  

/***************************************************/
void parse_label_file(char *optarg)
{
	FILE *in_file;
	char *label; /* convenience pointer to current label */
	int skip;  /* indicate whether to skip this line */
	const int buf_size = 1024;

		in_file = fopen(optarg, "r");
		if (in_file == NULL) 
		{
			printf("Cannot read label-file '%s'\n",optarg);
		} else {
			label = (char*)calloc(buf_size,sizeof(char));
			while (fgets(label,buf_size,in_file) != (char*)NULL) 
			{
				skip = 0;
				/* some simple things to skip and not bother warning about*/
				if (label[0] == '#') skip = 1;
				if (label[0] == '\n') skip = 1;
				if (!skip)
				{
					/* there is a newline on the end, we don't want it */
					label[strlen(label)-1] = '\0';
					/* assign this label */
					arguments.labelv[arguments.labelc]=label;
					arguments.labelc++;
					label=(char*)calloc(buf_size,sizeof(char));
				}
			}
		}
	free(label);
	return;
}

/***************************************************/

void show_help() {
  printf("ashxp version %s\n",ashxp_version_number);
  puts("valid options are:");
  puts("--airborne            -a               only show airborne");
  puts("--bgfile=FILE         -b FILE          background file to use");
  puts("--border=INT          -B INT           number of pixels on border");
  puts("--color=COLOR	      -c COLOR         coloring scheme for ash");
	puts("--colorbar-size=XxY                    colorbar size in X by Y pixels");
  puts("--fallout             -f               only show fallout");
  puts("--fontsize=INT        -F INT           only works with libfreetype");
  puts("--grayscale           -g               make background grayscale");
  puts("--gridlines=DG[xINT][COLOR]  -G DG[COLOR]  DG degrees per gridline");
  puts("--gridlines=DG:INTxDG:INT[COLOR]       as above, and INT pixels wide");
  puts("--hgt-range=H1/H2     -r H1/H2         (meters)");
  puts("--include-out-of-bounds                plot out-of-bounds ash");
  puts("--latlon=y1/y2/x1/x1  -l y1/y2/x1/x1   set bounding box");
	puts("--label=X,Y,STRING     L X,Y,STRING    put STRING at (X,Y)");    
	puts("--label-file=FILE                      labels are in file FILE");
  puts("--magnify=[+/-]INT    -m [+/-]INT      (de)magnify image");
  puts("--max-height=NUM      -h NUM           (meters)");
  puts("--min-size=INT        -x INT           (pixels)");
  puts("--no-background       -X               show ash on a black background");
  puts("--no-stamp                             do not print date/time stamp");
  puts("--output=FILE         -o FILE          print output to FILE");
  puts("--patch[=yes/no]                       try to patch projected image");
  puts("--pixels=INT          -p INT           particle size in pixels");
  puts("--projection=VALUE    -P VALUE         use VALUE as the projection");
  puts("--quiet");
  puts("--report              -R               generate a report");
  puts("--size=min/max                         show only this size range in mm");
  puts("--sorted              -s               sort by height");
  puts("--temp                -t               put output in temporary file");
  puts("--timestamp           -T               print a time/date stamp");
  puts("--usage");
  puts("--verbose             -v");
  puts("--version             -V");
  puts("--whole               -w               use the entire bgfile");
  
  return;
  }
  
void ashxp_usage() {
  puts("ashxp [options] ashfile(s)");
  return;
  }    
