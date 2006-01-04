#ifndef ASHXP_H
#define ASHXP_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

/* jpeglib.h seems to need these to compile */
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_LIBGEN_H
#include <libgen.h>  /* basename() */
#endif

#ifdef HAVE_JPEGLIB_H
#include <jpeglib.h> /* JSAMPLE */
#else
#include "jpeg-6b/jpeglib.h"
#endif /* HAVE_JPEGLIB_H */

/* conformal mapping */
#ifdef HAVE_CMAPF_H
#include "cmapf.h"
#else
#include "../libsrc/dmapf-c/cmapf.h"
#endif

struct bg_img {
  JSAMPLE* rgb;
  int xsize, ysize, length;
  int cc;  /* number of color components i.e. red, green and blue */
  int *mark; /* count of times this pixel has been marked */
  float dlon, dlat;
  float hgtmin, hgtmax, lonmax, lonmin, latmax, latmin;
  maparam stcprm;
  };

struct arguments {
  int airborne, border, fallout, fontsize, grayscale;
  int include_out_of_bounds, labelc, magnify, minsize, mpeg, nobg;
  int pixels, print_datetime_stamp, quiet, report, sorted, temp, verbose, whole;
  int xgridline_pixels, ygridline_pixels;
  char *bgfile, *color, *colorbar_size, *fontfile, *labelfile, *llstr, *output_file, *range, *rpt_txt;
  float  hgtmax, xgridlines, ygridlines;
  float  sizemin, sizemax;
  enum { LAMBERT, MERCATOR, POLAR } projection;
  /* --patch options */
  enum { PATCH_YES, PATCH_NO, PATCH_USER_NO } patch;
	char **labelv;
  } arguments;

struct Ash {
  float *lat, *lon, *hgt, *size;
  int *age, n, *valid, limits_set;
  float lonmax, lonmin, latmax, latmin, hgtmax, hgtmin, sizemin, sizemax;
  float ceiling, floor;
  char *gnd, *exists;
  char *date_time, *filename;
  double clock_time, origin_time;
  };

/* coloring schemes */
enum { COLOR_PROTOCOL_GRAY, 
       COLOR_PROTOCOL_RED, 
       COLOR_PROTOCOL_RAINBOW, 
       COLOR_PROTOCOL_ISOPACH, 
       COLOR_PROTOCOL_DEFAULT } coloring_scheme;

/* grid colors */
enum { GRIDLINE_BLACK, 
       GRIDLINE_BLUE, 
       GRIDLINE_GREEN, 
       GRIDLINE_PURPLE, 
       GRIDLINE_RED,
       GRIDLINE_WHITE, 
       GRIDLINE_YELLOW } gridline_color;


extern void ashxp_usage();
extern void clip_bg(struct bg_img*);
extern int read_ash(struct Ash*, char*);
extern char* create_ofile(char*);
extern void frame(struct Ash*, struct bg_img*);
extern void get_bbox(char**, int, struct Ash*, struct bg_img*);
extern void set_bbox(char*, struct bg_img*);
extern void ashxp_set_defaults();
extern void ashxp_parse_options();
extern void show_help();
extern void get_limits(struct Ash*, int);

#ifndef HAVE_BASENAME
#define basename(s) (strrchr((s), '/') == NULL ? (s) : strrchr((s), '/') + 1)
#endif

#ifndef HAVE_FLOORF
#define floorf(x) floor((float)x)
#endif

#ifndef HAVE_CEILF
#define ceilf(x) ceil((float)x)
#endif
#endif /* ASHXP_H */
