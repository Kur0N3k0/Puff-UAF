#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <netcdf.h>
#include <string.h>
#include "ashxp.h"

float* set_range(char*);
#define UNDEFINED_FLOAT -99999.
#define READASH_ERROR 1
#define READASH_OK 0

/*
 ***************************************************
 * read ash file data into the Ash structure.  Memory is allocated as necessary
 * and exits if any of the necessary fields (lat, lon, hgt) are not read
 * properly.  After reading, find the lat,lon,hgt limits for this group.  It 
 * tries to be intelligent when ash crosses the meridian for finding the most
 * applicable min/max values.  
 */
int read_ash(struct Ash* ash, char* filename) {

  int error, i;
  int ncid, varid;
  size_t length_ash;
  int first_valid_index = -1;

  void sort(struct Ash*);
  
  /* allocate space for filename */
/*  ash->filename = (char*)calloc(strlen(basename(filename))+1,sizeof(char)); */
  ash->filename = strdup(basename(filename) );
  
  error = nc_open(filename, NC_NOWRITE, &ncid);
  if (error != NC_NOERR) {
    printf("could not open %s.\n",filename);
    }
  /* get ash id */
  error = nc_inq_dimid(ncid, "nash", &varid);
  if (error != NC_NOERR){
    printf("error getting ash ID, exit\n");
    exit(EXIT_FAILURE);
    }
		
  /* find length of ash variable */
  error = nc_inq_dimlen(ncid, varid, &length_ash);
  if (error != NC_NOERR){
    printf("error getting length of ash, exit\n");
    exit(EXIT_FAILURE);
    }
	
  if (arguments.verbose) printf("%s: Number of ash particles: %d\n",filename,length_ash);

  ash->n = length_ash;	/* switching type: size_t -> int */
	
  /* allocate dynamic array */
  ash->lat  = (float *) calloc(ash->n, sizeof(float));
  ash->lon  = (float *) calloc(ash->n, sizeof(float));
  ash->hgt  = (float *) calloc(ash->n, sizeof(float));
  ash->gnd  = (char *)calloc(ash->n, sizeof(char));
  ash->exists  = (char *)calloc(ash->n, sizeof(char));
  ash->age  = (int *)  calloc(ash->n, sizeof(int)); 
  ash->valid= (int *)  calloc(ash->n, sizeof(int)); 
  ash->size = (float *) calloc(ash->n, sizeof(float));
  
  /* get lat ID */
  error = nc_inq_varid(ncid, "lat", &varid);
  if (error != NC_NOERR){
    printf("error getting lat ID. exit.\n");
    }

  /* get lat data */
  error = nc_get_var_float(ncid, varid, ash->lat);
  if(error != NC_NOERR){
    printf("read error for lat, exit.\n");
    exit(EXIT_FAILURE);
    }

  /* get lon ID */
  error = nc_inq_varid(ncid, "lon", &varid);
  if (error != NC_NOERR){
    printf("error getting lon ID. exit.\n");
    } 

  /* get lon data */
  error = nc_get_var_float(ncid, varid, ash->lon);
  if(error != NC_NOERR){
    printf("read error for lon, exit.\n");
    exit(EXIT_FAILURE);
    }
		
  /* get hgt ID */
  error = nc_inq_varid(ncid, "hgt", &varid);
  if (error != NC_NOERR){
    printf("error getting hgt ID. exit.\n");
    }

  /* get hgt data */
  error = nc_get_var_float(ncid, varid, ash->hgt);
  if(error != NC_NOERR){
    printf("read error for hgt, exit.\n");
    exit(EXIT_FAILURE);
    } 	
    
  /* get size ID */
  error = nc_inq_varid(ncid, "size", &varid);
  if (error != NC_NOERR){
    printf("error getting size ID. exit.\n");
    }

  /* get size data */
  error = nc_get_var_float(ncid, varid, ash->size);
  if(error != NC_NOERR){
    printf("read error for size, exit.\n");
    exit(EXIT_FAILURE);
    } 	
    
  /* get age ID */
  error = nc_inq_varid(ncid, "age", &varid);
  if (error != NC_NOERR){
    printf("error getting age ID. exit.\n");
    }

  /* get age data */
  error = nc_get_var_int(ncid, varid, ash->age);
  if(error != NC_NOERR){
    printf("read error for age, exit.\n");
    exit(EXIT_FAILURE);
    } 	

  /* get clock_time and origin_time to determine actual age of particle */
  /* since age is a time_t value */
  error = nc_inq_varid(ncid, "clock_time", &varid);
  if (error != NC_NOERR) {
    printf("error getting \"clock_time\" ID, exit.\n");
    exit(EXIT_FAILURE);
    }
    
  error = nc_get_var_double(ncid, varid, &ash->clock_time);
  if (error != NC_NOERR) {
    printf("error getting clock_time values, exit.\n");
    exit(EXIT_FAILURE);
    }
    
  error = nc_inq_varid(ncid, "origin_time", &varid);
  if (error != NC_NOERR) {
    printf("error getting \"origin_time\" ID, exit.\n");
    exit(EXIT_FAILURE);
    }
    
  error = nc_get_var_double(ncid, varid, &ash->origin_time);
  if (error != NC_NOERR) {
    printf("error getting origin_time values, exit.\n");
    exit(EXIT_FAILURE);
    }
      
  /* get grounded ID */
  error = nc_inq_varid(ncid, "grounded", &varid);
  /* only continue if 'grounded' is there */
  if (error == NC_NOERR) {
    /* get grounded data */
    error = nc_get_var_uchar(ncid, varid, ash->gnd);
    if (error != NC_NOERR) {
      printf("read error for grounded, exit.\n");
      exit(EXIT_FAILURE);
      }
    }

  /* get exists ID */
  error = nc_inq_varid(ncid, "exists", &varid);
  /* only continue if 'exists' is there */
  if (error == NC_NOERR) 
  {
    /* get 'exists' data */
    error = nc_get_var_uchar(ncid, varid, ash->exists);
    if (error != NC_NOERR) {
      printf("read error for variable \"exists\", exit.\n");
      exit(EXIT_FAILURE);
      }
  } else {
    /* if 'exists' is not there, make them all exist. calloc() set */
    /* set everything to zero above */
    for (i=0;i<ash->n;i++) 
    {
      ash->exists[i]=1;
    }
  }
  
  /* get date_time attribute if it exists */  
  nc_inq_attlen(ncid, NC_GLOBAL, "date_time", &length_ash);
  if (error == NC_NOERR) 
  {
    ash->date_time = (char*)calloc(length_ash+1, sizeof(char));
    error = nc_get_att_text(ncid, NC_GLOBAL, "date_time", ash->date_time);
  }  
  
  error = nc_close(ncid);  
  if (error != NC_NOERR) {
    printf("could not close %s.\n",filename);
    }

  if (arguments.sorted) sort(ash);

  /* check for invalid (Not-a-Number) values so we don't choke later on */
  for (i = 0; i<ash->n; i++) {
    if (!(ash->lon[i] <= 0 || ash->lon[i] >= 0)) {
     printf("bad value: lon[%i] = %f\n",i,ash->lon[i]);
     exit(EXIT_FAILURE);
     }
    if (!(ash->lat[i] <= 0 || ash->lat[i] >= 0)) {
     printf("bad value: lat[%i] = %f\n",i,ash->lat[i]);
     exit(EXIT_FAILURE);
     }
    if (!(ash->hgt[i] <= 0 || ash->hgt[i] >= 0)) {
     printf("bad value: hgt[%i] = %f\n",i,ash->hgt[i]);
     exit(EXIT_FAILURE);
     }
     
  }    
     
 /* use filters to set 'valid' for whether this particle is plotted */
 for (i=0; i<(ash->n); i++) 
 {
   /* fix longitude if it is not -180 <= lon <= 360 */
   while(ash->lon[i] > 360) ash->lon[i] -= 360;
   while(ash->lon[i] < -180) ash->lon[i] += 360;
   /* skip grounded particles when plotting only airborne */
   if ( (ash->gnd[i]) && arguments.airborne) continue;
   /* skip non-grounded particles when plotting fallout */
   if ( (!ash->gnd[i]) && arguments.fallout) continue;
   if (ash->age[i] > ash->clock_time) continue;
   /* skip particles not in hgt-range */
   if (ash->hgt[i] > ash->ceiling || ash->hgt[i] < ash->floor) continue;
   /* skip particles outside size range if one is specified */
   if ((arguments.sizemax) && ash->size[i] > arguments.sizemax) continue;
   if ((arguments.sizemin) && ash->size[i] < arguments.sizemin) continue;
   /* passed all tests, so mark it as valid for plotting */
   ash->valid[i] = 1;
   if (first_valid_index < 0) first_valid_index = i;
 }
 
 /* if no particles are valid, set ash-> to zero */
 if (first_valid_index < 0) 
 {
   ash->n = 0;
   return READASH_ERROR;
 } 

 (void)get_limits(ash, first_valid_index);
 
 /* if ash appears to cross the meridian, try  to get a better range */
 if (ash->lonmax - ash->lonmin > 180)
 {
   /* make all values > 180 be negative values */
   for (i=0; i<ash->n; i++)
   {
     if (ash->lon[i] > 180) ash->lon[i]-=360;
   }
   /* now redo the limit-finding routine */
   get_limits(ash, first_valid_index);
 }
 
  return READASH_OK;
}
/***************************************************/

void get_limits(struct Ash *ash, int first_valid_index)
{
  int i;
  /* set min/max at first valid particle */
  ash->hgtmax = ash->hgt[first_valid_index];
  ash->hgtmin = ash->hgt[first_valid_index];
  ash->lonmax = ash->lon[first_valid_index];
  ash->lonmin = ash->lon[first_valid_index];
  ash->latmax = ash->lat[first_valid_index];
  ash->latmin = ash->lat[first_valid_index];
  ash->sizemax = ash->size[first_valid_index];
  ash->sizemin = ash->size[first_valid_index];
 
  for (i=first_valid_index; i<(ash->n); i++) 
  {
    if (ash->valid[i]) 
    {
      if (ash->hgtmax < ash->hgt[i]) ash->hgtmax = ash->hgt[i];
      if (ash->hgtmin > ash->hgt[i]) ash->hgtmin = ash->hgt[i];
      if (ash->lonmax < ash->lon[i]) ash->lonmax = ash->lon[i];
      if (ash->lonmin > ash->lon[i]) ash->lonmin = ash->lon[i];
      if (ash->latmax < ash->lat[i]) ash->latmax = ash->lat[i];
      if (ash->latmin > ash->lat[i]) ash->latmin = ash->lat[i];
      if (ash->sizemax < ash->size[i]) ash->sizemax = ash->size[i];
      if (ash->sizemin > ash->size[i]) ash->sizemin = ash->size[i];
    }
  }
  return;
}
/*
 ***************************************************
 * get bounding box by finding min/max lon/lat in all files by repeatedly
 * doing read_ash()
 ***************************************************
 */
void get_bbox(char** cmdline, int index, struct Ash *ash, struct bg_img *bg) {
 
  char *file;
  float all_lonmax = -9999, all_lonmin = 9999;
  float all_latmax = -9999, all_latmin = 9999;
  float all_hgtmax = -9999, all_hgtmin = 9999999;
  float MAX_CLIP_HEIGHT = 30000;
  float *range;
  
  ash->limits_set = 0;
  ash->floor = -1e30;
  ash->ceiling = 1e30;
  if (arguments.range) {
    range = set_range(arguments.range);
    if (range[0] != UNDEFINED_FLOAT) ash->floor = range[0];
    if (range[1] != UNDEFINED_FLOAT) ash->ceiling = range[1];
    }
  
  if (cmdline[index] == NULL) {
    printf("no file(s) specified\n");
    ashxp_usage();
    exit(EXIT_FAILURE);
    }
    
    while( (file = cmdline[index]) ) {
    index++;
    if ( read_ash(ash, file) == READASH_OK)  
    {
      if (ash->lonmax > all_lonmax) all_lonmax = ash->lonmax;
      if (ash->latmax > all_latmax) all_latmax = ash->latmax;
      if (ash->hgtmax > all_hgtmax) all_hgtmax = ash->hgtmax;
      if (ash->lonmin < all_lonmin) all_lonmin = ash->lonmin;
      if (ash->latmin < all_latmin) all_latmin = ash->latmin;
      if (ash->hgtmin < all_hgtmin) all_hgtmin = ash->hgtmin;
      /* done with lat/lon/hgt data, so free it */
      free(ash->lat);
      free(ash->lon);
      free(ash->hgt);
      free(ash->gnd);
      free(ash->exists);
      free(ash->age);
      free(ash->valid);
      free(ash->size);
      free(ash->filename);

    }
  }
  
  /* reset min/max of ash.lat/lon for clip_bg() */
  bg->lonmin = all_lonmin;
  bg->latmin = all_latmin;
  bg->hgtmin = all_hgtmin;
  bg->lonmax = all_lonmax;
  bg->latmax = all_latmax;
  bg->hgtmax = all_hgtmax;
 
  if (bg->hgtmax > MAX_CLIP_HEIGHT) bg->hgtmax = MAX_CLIP_HEIGHT;
  /* if a maximum height was set and the current max height is greater than
   * this, make the maximum height the specified value 
   */
  if ( (arguments.hgtmax > 0) && (bg->hgtmax > arguments.hgtmax))
  {
    bg->hgtmax = arguments.hgtmax;
  }
  
  ash->limits_set = 1;
  return;
  }
/***************************************************/
void set_bbox(char* llstr, struct bg_img *bg) {
  void bbox_fail(char*);
  float lonmin, lonmax, latmin, latmax, swap;
  
  /* return if a bounding box was not really set */
  if (!strcmp(llstr,"default")) return;
  if (!strcmp(llstr,"///")) return;
  
  if (sscanf(llstr,"%f/%f/%f/%f",&latmin,&latmax,&lonmin,&lonmax) != 4) bbox_fail(llstr);
  
  /* lat/lon may be switched because both --latlon and --lonlat are possible */
  if (strchr(llstr, '!'))
  {
    swap=latmin; latmin=lonmin; lonmin=swap;
    swap=latmax; latmax=lonmax; lonmax=swap;
  }
  /* fix lon values outside 0-360 */
  while (lonmax >=360) { lonmax -= 360; }
  while (lonmax < 0)   { lonmax += 360; }
  while (lonmin >=360) { lonmin -= 360; }
/*  while (lonmin < 0)   { lonmin += 360;  } */
  
  /* check minimum longitude */
	/* this can be negative to cross the dateline */
    bg->lonmin = lonmin;
    
  /* check maximum longitude */
  if (lonmax > 0 && lonmax < 360) {
    bg->lonmax = lonmax;
    }
  else {
    printf("bad maxmimum longitude value %f\n",lonmax);
    printf("LatMax must be between 0 and 360");    
    exit(EXIT_FAILURE);
    }
    
  /* check minimum latitude */
  if (latmin > -90 && latmin < 90) {
    bg->latmin = latmin;
    }
  else {
    printf("bad minimum latitude value %f\n",latmin);
    exit(EXIT_FAILURE);
    }

  /* check maximum latitude */
  if (latmax > -90 && latmax < 90) {
    bg->latmax = latmax;
    }
  else {
    printf("bad maximum latitude value %f\n",latmax);
    exit(EXIT_FAILURE);
    }

  }
  
/***************************************************/
  
void bbox_fail(char* llstr) {
  printf("bad argument --latlon=%s\n",llstr);
  printf("format: --latlon=LatMin/LatMax/LonMin/LonMax\n");
  exit(EXIT_FAILURE);
  return;
  }

/*
 ***************************************************
 * set the vertical range by parsing the command-line argument --hgt-range
 ***************************************************
 */
float* set_range(char *rangestr) 
{
  void range_fail(char*);
  char orig[256];  /* copy of original string */
  char *tok;
  float *range = NULL;
  static float rangemin = UNDEFINED_FLOAT, rangemax = UNDEFINED_FLOAT;
  
  range = calloc(2,sizeof(float));

  if (rangemin == UNDEFINED_FLOAT && rangemax == UNDEFINED_FLOAT) {  
  /* make copy of original string since strtok changes it */
  strcpy(orig, rangestr);
  /* get first parameter */
  if (! (tok = strtok(rangestr, "/") )) range_fail(orig);
  if (strcmp(tok, "-") && sscanf(tok, "%f", &rangemin) == 0 ) range_fail(orig);
/*  rangemin = (float)atof(tok); */
  if (! (tok = strtok(NULL, "/"))  ) range_fail(orig);
  if (strcmp(tok, "-") && sscanf(tok, "%f", &rangemax) == 0 ) range_fail(orig);
  
  if (rangemin < 0 && rangemin != UNDEFINED_FLOAT) {
    printf("bad minimum range value %f\n", rangemin);
    exit(EXIT_FAILURE);
    }
  if (rangemax < 0 && rangemax != UNDEFINED_FLOAT) {
    printf("bad maximum range value %f\n", rangemax);
    exit(EXIT_FAILURE);
    }
  if (rangemin > rangemax  
   && rangemax != UNDEFINED_FLOAT 
   && rangemin != UNDEFINED_FLOAT) {
    printf("no values possible within range of %f to %f\n",rangemin,rangemax);
    exit(EXIT_FAILURE);
    }
  }
  
  range[0]=rangemin;
  range[1]=rangemax;
  
  return range;
}
  
 void range_fail(char* range) {
   printf("bad argument --hgt-range=%s\n",range);
   printf("format: --hgt-range=rangemin/rangemax");
   exit(EXIT_FAILURE);
   return;
   }

/*  
 ***************************************************
 * bubble sort 
 ***************************************************
 */
void sort (struct Ash *ash) {
  int i,j, swap;
  float x,y,z;
  
  j=1;	/* start so (j-1) is the first element */
  
  for (j=1; j<(ash->n); j++) {
    swap=0;	/* flag for when last swap was done */
    for (i=1; i<(ash->n); i++) {
      if (ash->hgt[i] < ash->hgt[i-1]) {
        /* swap */
	swap = 1;
	x=ash->lon[i];
	y=ash->lat[i];
	z=ash->hgt[i];
	ash->lon[i]=ash->lon[i-1];
	ash->lat[i]=ash->lat[i-1];
	ash->hgt[i]=ash->hgt[i-1];
        ash->lon[i-1]=x;
	ash->lat[i-1]=y;
	ash->hgt[i-1]=z;
        }
      }
    if ( swap == 0 ) return;	/* done sorting */
    }
  return;
  }
/***************************************************/
