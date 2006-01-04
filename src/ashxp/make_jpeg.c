#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

/* jpeglib.h seems to need these to compile */
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_JPEGLIB_H
#include <jpeglib.h> /* JSAMPLE */
#else
#include "jpeg-6b/jpeglib.h"
#endif /* HAVE_JPEGLIB_H */

#include <math.h>  /* ceil */
#include <string.h> /* strmcp */
#include "ashxp.h"
#include "make_jpeg.h"
#include "projection.h"

/*
 **************************************************
 * write the JPEG image to disk using JPEG library routines.  
 ***************************************************
 */
 
 int make_jpeg(struct bg_img *img) {
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE * outfile ;
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;
  char *filename;
  
  filename = arguments.output_file;

   /* Initialize the JPEG compression object with default error handling. */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  if ((outfile = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    exit(1);
  }
 
   if (arguments.verbose) {
     printf("writing %s\n",filename);
     fflush(stdout);
     }
     
  jpeg_stdio_dest(&cinfo, outfile);

  cinfo.image_width = img->xsize;	/* image width and height, in pixels */
  cinfo.image_height = img->ysize;
  cinfo.input_components = 3;		/* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */

  jpeg_set_defaults(&cinfo);

  jpeg_start_compress(&cinfo, TRUE);

  row_stride = img->xsize * 3;	/* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
    row_pointer[0] = &img->rgb[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);
  /* After finish_compress, we can close the output file. */
  fclose(outfile);
 
  return 0;
}

/***************************************************/
/* read in the original background image and clip it according to the */ 
/* min/mat lat/lon pair of all imput files.                           */
/***************************************************/

void clip_bg(struct bg_img *bg) {
  FILE *bgfile;
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPLE *image_buffer = NULL;
  int i,j,k;			/* counters */
  int xleft, ytop;		/* pixel values of top-left of orig. bgfile */
  int xright, ybottom;          /* pixel value of the lower-right */
  int xsize, ysize, border;	/* new image dimensions, border */
  float shiftedlon;		/* lonmin shifted to correspond with bgimage*/
  int offset;
  char *filename;
  char gray;			/* grayscale rgb value */
  char *report ;		/* report text [optional] */
  int done_reading_jpeg = 0;
  
  if (!(arguments.bgfile)) 
  {
    puts("No background file specified.  Either use the --bgfile option or set the environment variable ASHXP_BG_FILE_NAME");
    exit(0);
  } else {
    filename = arguments.bgfile;
  }
  
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  if ((bgfile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    exit(1);
  }
  jpeg_stdio_src(&cinfo, bgfile);

  jpeg_read_header(&cinfo,TRUE);

  /* degrees per pixel in bg_image */
  bg->dlon = 360.0/cinfo.image_width;
  bg->dlat = 180.0/cinfo.image_height;
  
  /* add border in degrees */
  border = arguments.border;
  bg->lonmin -= border;
  bg->lonmax += border;
  if ((bg->latmin-=border) < -90) bg->latmin = -90.0;
  if ((bg->latmax+=border) > 90) bg->latmax = 90.0;
 
  /* set the pseudo-lon value that corresponds to the image */ 
  shiftedlon = bg->lonmin-180;
  if (shiftedlon < 0) shiftedlon +=360;
  
  /* set the lefthand x-pixel value */
  xleft = (int) (shiftedlon/bg->dlon);
  
  /* re-set the pseudo-lon value that corresponds to the image */ 
  shiftedlon = bg->lonmax-180;
  if (shiftedlon < 0) shiftedlon +=360;
  
  /* set the righthand x-pixel value */
  xright = (int)(shiftedlon/bg->dlon);
  
  /* increment in blocks of 16 pixels, helps with MPEGs */
  xsize = xright - xleft + 1;
  
  /* image may wrap around, so fix that */
  if (xsize < 0)
  {
    xsize = (360.0/bg->dlon) - xleft + xright + 1;
  }
  
  offset = xsize%16;
  if ( offset > 0)
  {
    xright += (16 - offset );
    xsize += (16 - offset );
    bg->lonmax += (float)offset*bg->dlon;
  }
      
  while (xsize < arguments.minsize)
  {
    xright += 16;
    xsize += 16;
    bg->lonmax += 16*bg->dlon;
  }
  
  /* set the top y-pixel value */
  ytop = (int)( (90 - bg->latmax)/bg->dlat );
  
  /* set the bottom y-pixel value */
  ybottom = (int)( (90-bg->latmin)/bg->dlat);
  
  /* make it divisible by 16 */
  offset = (ybottom-ytop + 1)%16;
  if (offset > 0)
  {
    ybottom += (16 - offset );
    bg->latmin -= (float)offset*bg->dlat;
  }
  
  /* make minimum size */
  while ((ybottom - ytop) < arguments.minsize - 1)
  {
    ybottom += 16;
    bg->latmin -= (16.0)*bg->dlat;
  }
  
  /* check if ash crosses orig. image right-hand boundary */  
  /* if it does, set 'offset' for proper wrapping of the image */
  offset = 0;
  if (xleft > xright) offset = cinfo.image_width;
  
  if (arguments.whole) {
  /* use the whole image, and redefine bg min/max */
    xsize = cinfo.image_width;
    ysize = cinfo.image_height;
    xleft = (int)cinfo.image_width/2;
    ytop = 0;
    bg->lonmin = 0;
    bg->lonmax = 360;
    bg->latmax = 90;
    bg->latmin = -90;
    arguments.border=0;
  } else {
    /* find minimum size */

    /* the background min/max latitude are not the same as ash's */
    xsize=xright-xleft + offset + 1;
    
    if (xsize > cinfo.image_width)
    {
      printf("error finding image x dimensions\n");
      exit(EXIT_FAILURE);
    } 
         
    ysize = ybottom - ytop + 1;
    
    if (ysize > cinfo.image_height)
    {
      printf("error finding image y dimensions\n");
      exit(EXIT_FAILURE);
    }      
  }

  bg->xsize = xsize;
  bg->ysize = ysize;

  if (arguments.verbose)printf("image size: %dx%d\n",xsize,ysize);
   
/* this must be called before allocating memory to set output_components */
  jpeg_start_decompress(&cinfo);

 /* dynamically allocate buffer for the new image */
   bg->length = xsize*ysize*3;
   bg->cc = 3;
   bg->rgb = calloc(bg->length,sizeof(JSAMPLE));
   if (!bg->rgb) {
     printf("failed to allocate new image buffer\n");
     exit(EXIT_FAILURE);
     }
   
  /* dynamically allocate single-line image buffer for reading */
     image_buffer=calloc(
       cinfo.image_width*
       cinfo.output_components,sizeof(JSAMPLE));
    if (! image_buffer) {
      printf("failed to allocate image buffer\n");
      exit(EXIT_FAILURE);
      }
  
  i=0; 
  if (!arguments.nobg) {
    /* read in background image 1 line at a time */ 
    if (arguments.verbose) {
      printf("reading in background image %s...",filename);
      fflush(stdout);
      }
    while (cinfo.output_scanline < cinfo.output_height && !(done_reading_jpeg)) {
      /* read a single line from the image */
      jpeg_read_scanlines(&cinfo, &image_buffer, 1);
      /* shift so first value is lefthand side of new (smaller) image */
      shift_line(xleft,cinfo.image_width,cinfo.output_components,image_buffer);
      /* store in new-image buffer if line an included latitude value (ytop) */
      if (cinfo.output_scanline >= ytop && cinfo.output_scanline < ytop+ysize) {
        for (j=0; j<xsize*(cinfo.output_components); j++) {
	  for (k=3; k>=(cinfo.output_components); k--) {
            bg->rgb[i] = image_buffer[j];
	    i++;
	    }
	  }
       }
      /* flag to quite reading data, we have all we need */
      if (cinfo.output_scanline >= ytop+ysize) done_reading_jpeg = 1;
      }
    if (arguments.verbose) { printf("done.\n"); fflush(stdout); }
    }
  else{
    if (arguments.verbose) { printf("not using background image\n"); }
    /* set 'output_scanline' to the last value to kick out of the loop */
    cinfo.output_scanline = cinfo.output_height;
    }

  jpeg_abort_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  free(image_buffer);

  /* do a grayscale conversion of the background */
  if (arguments.grayscale) {
    for (i=0; i<(3*bg->xsize*bg->ysize); i+=3) {
      gray = (bg->rgb[i]*0.299)+(bg->rgb[i+1]*0.587)+(bg->rgb[i+2]*0.114);
      bg->rgb[i] = gray;
      bg->rgb[i+1] = gray;
      bg->rgb[i+2] = gray;
      }
    }
    
  
  /* add gridlines */
  add_gridlines(bg);
  
  /* skew the image into a new projection */
  make_projection(bg);

	/* add labels */
	add_labels(bg);

  /* report size */
  if (arguments.report) {
    report=calloc(128,sizeof(char));
    sprintf(report,"size:%dx%d ",bg->xsize, bg->ysize);
    strcat(arguments.rpt_txt, report);
    sprintf(report,"limits:%5.2f/%5.2f/%5.2f/%5.2f ", bg->lonmin, bg->lonmax, bg->latmin, bg->latmax);
    strcat(arguments.rpt_txt, report);
    }
 
  return; 	/* create_image */
  }

/***************************************************/
/* create a single JPEG frame using the background *bg and ash data *ash */
/**************************************************/  
void frame(struct Ash *ash, struct bg_img *bg) {
  int pix = arguments.pixels; /* shorthand notation */
  int i, j, *index;
  int *sh_index=NULL;  /* array that simply marks the pixel indicies that */
                        /* that will be shade-marked */
  int xpixel, ypixel; /* (x,y) location with (0,0) in upper left! */
  float wgt, delta;  /* weight and a delta-degree holding variable */
	struct bg_img frame;
//  JSAMPLE *img = NULL; /* this image */
  double xFloat, yFloat; /* (x,y) location returned from conformal mapping */
  
  /* allocate shading index if necessary*/
  if (coloring_scheme == COLOR_PROTOCOL_ISOPACH ) 
  { 
    sh_index=calloc(bg->xsize*bg->ysize,sizeof(int)); 
    if (!sh_index)
    {
    puts("ERROR: failed to allocate memory for shading index");
    exit(EXIT_FAILURE);
    }
  }
    
  /* allocate new image */
	frame.rgb = calloc(bg->length,sizeof(JSAMPLE));
  if (!frame.rgb) 
  {
    printf("failed to allocate memory for image.\n");
    exit(EXIT_FAILURE);
  }
  
  /* copy background into new image */
	memcpy(frame.rgb, bg->rgb, bg->length);
	frame.length = bg->length;
	frame.xsize = bg->xsize;
	frame.ysize = bg->ysize;
	frame.cc = bg->cc;
	frame.hgtmax = bg->hgtmax;
	frame.hgtmin = bg->hgtmin;
  
  /* allocate space for index numbers for each pixel that gets marked */
  index=calloc(pix*pix,sizeof(int));
  
  /*
   * loop through all ash particles, marking only those that meet the criteria
   * necessary, such as in bounds, valid, etc.
   */
  for (i=0; i<ash->n; i++) {
    /* check criteria for plotting this particle */
    if (!ash->valid[i]) continue;
    if (ash->hgt[i] < bg->hgtmin || ash->hgt[i] > bg->hgtmax) continue;
    if (!ash->exists[i] && !arguments.include_out_of_bounds) continue; 
    if (ash->lon[i] <= bg->lonmin ||
        ash->lon[i] >= bg->lonmax ||
				ash->lat[i] <= bg->latmin ||
				ash->lat[i] >= bg->latmax) continue;
    cll2xy(&bg->stcprm, ash->lat[i], ash->lon[i], &xFloat, &yFloat);
		xpixel = (int)floorf(xFloat);
    ypixel = (bg->ysize-(int)floorf(yFloat) );
    /* weight by the height of the particle */
    wgt = (ash->hgt[i]-bg->hgtmin)/(bg->hgtmax-bg->hgtmin);
    if (wgt > 1.0) wgt = 1.0;  /* clipping */
    /* only plot the particle if all pixel values map onto the image */
    if (xpixel+pix < bg->xsize && xpixel-pix > 0 &&
        ypixel+pix < bg->ysize && ypixel-pix > 0) 
      {
      /* return in the array *index a list of the index values to be marked*/
      get_indices(xpixel, ypixel, bg->xsize, bg->ysize, index, pix);
      /* loop through the number of pixels to be marked for this particle */
      for (j=0; j<pix*pix; j++) {  /* indices are RGBRGBRGB... */
      /* mark the shading index if it is being used */
	if (sh_index) { sh_index[index[j]/3]++; }
	/* otherwise color the pixel by wgt */
	else { wgt_color(&frame.rgb[index[j]],&wgt);}
        }
      }
    } /* end loop over all ash particles */
  
  /* pixels are marked, so free ash memory */
  free(index);
  free(ash->lat);
  free(ash->lon);
  free(ash->hgt);
  free(ash->gnd);
  free(ash->exists);
  free(ash->age);
  free(ash->valid);
  free(ash->size);
  
  /* if an isopach (shaded) image is being created, the pixels have not been */
  /* marked yet because we need to know the total maximum concentration */
  /* before makring.  Now mark them */
  if (sh_index) 
  {
    scale_shading(sh_index,&frame); 
  }
  
  /* add colorbar */
  add_colorbar(&frame);
  
  if (arguments.print_datetime_stamp)
    add_date_time(&frame, ash->date_time);
  /* only free filename after printing the time/date stamp */
  free(ash->filename);

  /* create and write this image as a jpeg */
  make_jpeg(&frame);
  
  /* free the memory */
  free(frame.rgb);
  if (sh_index) free(sh_index);

  return;
  }

/***************************************************/
/* create an array of values (indices) that will be marked as ash particles */
/* Value 'n' is the number of pixels wide (and high) for this ash particle  */
/***************************************************/

void get_indices(int x, int y, int xsize, int ysize, int* index, int n) {

  int i, j, k; /* counter */
  int sign = -1;  /* sign toggle for moving left/right of the x-pixel */
  
  /* mark first pixel */
  index[0]=(y*xsize+x)*3;
  
  j = 1; /* initialize counter for marking futher pixels */
  /* loop through layers of neighbors */
  for (i=1; i<n; i++) {
    x+=sign;  /* move left/right */
    index[j]=(y*xsize+x)*3;	/* mark pixel */
    j++;			/* advance counter */
    sign*=-1;			/* prepare to move up/down */
    for (k=0; k<i; k++) {	/* loop for moving up/down */
      y+=sign;			/* move up/down */
      index[j]=(y*xsize+x)*3;	/* mark pixel */
      j++;			/* advance counter */
      }
    for (k=0; k<i; k++) {	/* loop for moving left/right */
      x+=sign;			/* move left/right */
      index[j]=(y*xsize+x)*3;	/* mark pixel */
      j++;			/* advance counter */
      }
    }      
    
      
  return;
  }

/***************************************************/
/* shift the data in the scanned lined of data so the first (left-hand)  */
/* value corresponds to the leftmost edge of the new (smaller) image     */
/* Needs its own subroutine since the new image may scroll off the right */
/* edge of the original image */
/***************************************************/
void shift_line(int xleft, int size, int cc, JSAMPLE *buf) {
  JSAMPLE *temp;
  int i, j;
  
  j = xleft*cc;
    
  temp = calloc(size*cc,sizeof(JSAMPLE));
  for (i=0; i<size*cc; i++) {
    temp[i]=buf[j];
    j++;
    if (j >= size*cc) j=0;
    }
  
  for (i=0; i<size*cc; i++) {
    buf[i]=temp[i];
    }
  free(temp);
  
  return;
  }
      
/***************************************************/
void add_gridlines(struct bg_img *bg) {
  
  float dgx, dgy;  /* degrees per gridline */
  double lon, lat;  /* current lat/lon values */
  int i; /* counter */
  int init_idx; /* initial index of the upper, left line starting point */
  int idx = 0;  /* index of the rgb values */
  int idx_local; /* local index when creating thick gridlines */
  int scanx, scany = 0; /* local values for x and y size scanning */
  
  /* set degrees per gridline from argument, or return if not defined */
  if (arguments.xgridlines <= 0 && arguments.ygridlines <= 0) 
  {
    return;
  } else {
    dgx = arguments.xgridlines;
    dgy = arguments.ygridlines;
  }
  
  /* VERTICAL LINES */
  /* set lat to the top line in the image */
  lat = bg ->latmax;
  
  /* determine the first lon value to mark, all ashfile lons are +'ive */
  lon = ceilf(bg->lonmin/dgx) * dgx;
        
  /* mark this initial index to this value */
  init_idx = bg_index(bg, lon, lat);
  
  /* move the index to the initial index value */
  idx = init_idx;
  
  /* scanx is the pixel value in the x-direction */
  scanx = init_idx/(bg->cc);
  
  /* perform a horizontal scan, doing a vertical line at each point */
  /* idx is -1 if the index is out-of-bounds */
  while (scanx < bg->xsize && idx >= 0)
  {
    /* make a vertical line at constant lon */
    while (idx < bg->length)
    {
      /* mark the pixel */
      for (i=(int)floorf(-arguments.xgridline_pixels/2); 
           i<(int)floorf(arguments.xgridline_pixels/2)+1;
	   i++)
      {
        idx_local = idx+i*bg->cc;
        if ( (idx_local >= 0) && 
	      (idx_local < bg->length-1) )color1(&(bg->rgb[idx_local]));
      }
      /* advance the index to the next line */
      idx+=bg->xsize*bg->cc;  
    }
    
    /* advance to the next longitude which needs a vertical line */
    scanx += (int)ceilf(dgx/bg->dlon);
    /* set the index value */
    idx = scanx*bg->cc;
  }  
  
  /* HORIZONTAL LINES */
  /* start lon on the left side */
  lon = bg->lonmin;

  /* find the first latitude to mark */
  lat=90.0;
  while (lat > bg->latmax) lat -= dgy;

  /* mark this initial index to this value */
  init_idx = bg_index(bg, lon, lat);
  
  /* move the index to the initial index value */
  idx = init_idx;
  
  /* scany is the pixel value in the y-direction */
  scany = init_idx / bg->xsize / bg->cc;
  
  /* perform a vertical scan, drawing a horizontal line at each location */
  while (scany < bg->ysize && idx >= 0 )
  {
    /* scanx is the x-direction pixel value */
    scanx = 0;
    
    /* make a horizontal line */
    while (scanx < bg->xsize)
    {
      /* mark the pixel */
      for (i=(int)floorf(-arguments.ygridline_pixels/2); 
           i<(int)floorf(arguments.ygridline_pixels/2)+1;
	   i++)
      {
        idx_local = idx+bg->xsize*i*bg->cc;
        if ( (idx_local >= 0) &&
	     (idx_local < bg->length-1) ) color1(&(bg->rgb[idx_local]));
      }
	      /* advance to the next pixel */
      scanx++;
      idx+=bg->cc;
    }
    /* advance to the next latitude value */
    scany += (int)ceilf(dgy/bg->dlat);
    /* set the index value */
    idx = scany * bg->xsize * bg->cc;    
  }
  
  return; 
}

/***************************************************/
int bg_index (struct bg_img *bg, const float lon, const float lat) 
{
  int idx = -1;
  float xlat, xlon; /* local values */
  
  /* return -1 if this lat/lon is not in the image */
  if (lon < bg->lonmin || lat < bg->latmin ||
      lat > bg->latmax || lon > bg->lonmax) return idx;
      
  /* initialize the local value */
  xlon = bg->lonmin;
  xlat = bg->latmax;
  idx = 0;

  /* loop while lon is to the left of the local value and 'lat' is below */
  /* the local lat value */
  for (;;)
  {
     if (xlon >= lon && xlat <= lat) break;
     idx+=3;
     xlon += bg->dlon;
     if (xlon > bg->lonmax)
     {  
       xlon = bg->lonmin;
       xlat -= bg->dlat;
     }  /* if... */
      
    }

  if (idx > bg->length-1)
  {
    puts("ERROR: bg_index: Invalid index value computed.");
    exit(1);
  }
  
  return idx;
}
/***************************************************/
void patch_image(struct bg_img *bg)
{
  int cnt, idx, avg_cnt;
  float frac;
  char avg;
  
  for (cnt = 0; cnt <bg->length; cnt++ )
  {
    if (bg->mark[cnt] == 0)
    {
      avg = 0;
      avg_cnt = 0;
      /* use the 8 neighbors */
      /*
       *     x**
       *     * *
       *     ***
       */
      idx = cnt-(bg->xsize-1)*bg->cc;
      if (idx >= 0 && idx < bg->length && bg->mark[idx])
      {
        frac = avg_cnt/(avg_cnt+1);
        avg = frac*avg+(1-frac)*bg->rgb[idx];
        avg_cnt++;
      }
      /* use the 8 neighbors */
      /*
       *     *x*
       *     * *
       *     ***
       */
      idx = cnt-bg->xsize*bg->cc;
      if (idx >= 0 && idx < bg->length && bg->mark[idx])
      {
        frac = avg_cnt/(avg_cnt+1);
        avg = frac*avg+(1-frac)*bg->rgb[idx];
        avg_cnt++;
      }
      /* use the 8 neighbors */
      /*
       *     **x
       *     * *
       *     ***
       */
      idx = cnt-(bg->xsize+1)*bg->cc;
      if (idx >= 0 && idx < bg->length && bg->mark[idx])
      {
        frac = avg_cnt/(avg_cnt+1);
        avg = frac*avg+(1-frac)*bg->rgb[idx];
        avg_cnt++;
      }
      /* use the 8 neighbors */
      /*
       *     ***
       *     x *
       *     ***
       */
      idx = cnt-bg->cc;
      if (idx >= 0 && idx < bg->length && bg->mark[idx])
      {
        frac = avg_cnt/(avg_cnt+1);
        avg = frac*avg+(1-frac)*bg->rgb[idx];
        avg_cnt++;
      }
      /* use the 8 neighbors */
      /*
       *     ***
       *     * x
       *     ***
       */
      idx = cnt+bg->cc;
      if (idx >= 0 && idx < bg->length && bg->mark[idx])
      {
        frac = avg_cnt/(avg_cnt+1);
        avg = frac*avg+(1-frac)*bg->rgb[idx];
        avg_cnt++;
      }
      /* use the 8 neighbors */
      /*
       *     ***
       *     * *
       *     x**
       */
      idx = cnt+(bg->xsize-1)*bg->cc;
      if (idx >= 0 && idx < bg->length && bg->mark[idx])
      {
        frac = avg_cnt/(avg_cnt+1);
        avg = frac*avg+(1-frac)*bg->rgb[idx];
        avg_cnt++;
      }
      /* use the 8 neighbors */
      /*
       *     ***
       *     * *
       *     *x*
       */
      idx = cnt+bg->xsize*bg->cc;
      if (idx >= 0 && idx < bg->length && bg->mark[idx])
      {
        frac = avg_cnt/(avg_cnt+1);
        avg = frac*avg+(1-frac)*bg->rgb[idx];
        avg_cnt++;
      }
      /* use the 8 neighbors */
      /*
       *     x**
       *     * *
       *     ***
       */
      idx = cnt+(bg->xsize+1)*bg->cc;
      if (idx >= 0 && idx < bg->length && bg->mark[idx])
      {
        frac = avg_cnt/(avg_cnt+1);
        avg = frac*avg+(1-frac)*bg->rgb[idx];
        avg_cnt++;
      }
      bg->rgb[cnt]=avg;
    }
  } /* cnt loop */
}
