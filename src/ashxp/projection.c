#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

/* conformal mapping library */
#ifdef HAVE_CMAPF_H
#include "cmapf.h"
#else
#include "../libsrc/dmapf-c/cmapf.h"
#endif

#include <math.h> /* floor() */

/* local prototypes */
#include "ashxp.h"
#include "projection.h"
#include "make_jpeg.h"

void make_projection (struct bg_img *bg)
{
  struct bg_img bg_p; /* new bg image with projection */
/*  int *bg_p_cnt = NULL; */ /* array for counting pixels when mapping */
  int o_idx, m_idx, p_idx; /* original, magnify, and projection index */
  double tang_lon, tang_lat; /* tangent lat/lon for projection */
  int grid_scale;
  double latToConvert, lonToConvert, xOut, yOut; /* temp variables */
  float frac; /* temp variable */
  int rgb_flag; /* 0,1 or 2 counter for keeping track of rgb index */
  double scale_lat, scale_lon, set_lat, set_lon, set_x, set_y; /* scaling */
  double x[4], y[4]; /* temporary variables */
  const int y_buf_pix = 2; /* buffer of pixels to accout for round-off err */
  
  /* cut the projection 180 opposite of the average longitude */
  tang_lon = (bg->lonmax+bg->lonmin)/2.0;

  /* each projection type is centered on a different latitude */
  if (arguments.projection == MERCATOR)
  {
    tang_lat = 0.0;
  } else if (arguments.projection == POLAR) {
    if (bg->latmin > 0) 
    {
      tang_lat = 90.0;
    } else if (bg->latmax < 0) {
      tang_lat = -90;
    } else {
      puts("ERROR: polar stereo projection is not possible.");
      exit(1);
    } 
  } else if (arguments.projection == LAMBERT) {
    tang_lat = (bg->latmax+bg->latmin)/2.0;
  } else {
    /* should never get here */
    puts("ERROR: make_projection(): arguments.projection has bad value");
    exit(1);
  }

  /* make a normal (aligned with Earth's axis) projection */
  stlmbr(&bg->stcprm, tang_lat, tang_lon);
  
  /* we want the smallest grid scaling possible for the best final resolution.
   * 'grid_scale' has units of km-per-grid, which corresponds to km-per-pixel
   * in out case.  Higher values are lower resolution.  The lowest resolution
   * we can get is at the equator where it is:
   * 40,000/360 * bg->dlon 
   * (Earth circumference in km) * (degrees) * (degrees/pixel in the bg_img)
   * In theory, we could decrease grid_scale near the poles because the bg_img
   * has greater resolution there, but a previous try at that resulted in 
   * some pixels not being mapped to the new image, leaving ugly black holes.
   */

   grid_scale = (int)ceilf(40000.0/360.0*bg->dlon);
   
   /* magnify the image */
   if (arguments.magnify > 0)
   {
     for (m_idx = 0; m_idx < arguments.magnify; m_idx++)
     {
       grid_scale--;
     }
   }
   if (arguments.magnify < 0)
   {
     for (m_idx = 0; m_idx > arguments.magnify; m_idx--)
     {
       grid_scale++;
     }
   }
   
  /* create a grid such that every lat/lon point of the original image will
   * be contained within it.  In the N. hemisphere, we use the values at the
   * minimum latitude; in the S. hemisphere, we use the values at the maximum
   * latitude.  If the bg_img has parts of both hemispheres, we use the most
   * equitorial values */

  if (bg->latmin >= 0) /* all N. hemisphere */
  {
    set_lat = bg->latmin;
    set_lon = (bg->lonmin+bg->lonmax)/2;
    set_x = bg->xsize/2;
    set_y = y_buf_pix; /* give outselves a buffer for round-off */
    scale_lat = set_lat;
    scale_lon = set_lon;
  } else if (bg->latmax < 0) { /* all S. hemisphere */
    set_lat = bg->latmax;
    set_lon = (bg->lonmin+bg->lonmax)/2;
    set_x = bg->xsize/2;
    set_y = bg->ysize; 
    scale_lat = set_lat;
    scale_lon = set_lon;
  } else { /*crosses equator */
    set_lat = bg->latmin;
    set_lon = (bg->lonmin+bg->lonmax)/2;
    set_x = bg->xsize/2;
    set_y = y_buf_pix;
    scale_lat = 0;
    scale_lon = (bg->lonmin+bg->lonmax)/2;
  }
  
  stcm1p(&bg->stcprm,     /* the map projection structure */
         set_x,         /* the 'x' point we are fixing */
         set_y,         /* the 'y' point we are fixing */ 
         set_lat,       /* the lat that (x,y) above correspond to */
         set_lon,       /* the lon that (x,y) above correspond to */
         scale_lat,       /* perform grid scaling here  */ 
         scale_lon,       /* perform grid scaling here */
         grid_scale,      /* grid scaling (km/grid == km/pixel */
         0);              /* degree orientation, set North upwards */


  /* determine the overall size of this new image by mapping the four corners*/
/*
 * 
 *       y3*********          *    y3               *y3*
 *       *          *        * **** *            ***    ***
 *      *            *      *        *         x0          x1
 *      x0          x1     *          *         *          *
 *      *            *    *            *         *        *
 *       *          *    *              *         *      *
 *        y2********     x0***      ***x1          y2****
 *                             *y2*              
 *        equatorial    Northern hemisphere   Southern hemisphere
 *        
 */
  if ( (bg->lonmax - bg->lonmin) <= 180) 
  {
  if (bg->latmin >= 0) /* N. hemisphere */
  {
    cll2xy(&bg->stcprm, bg->latmin, bg->lonmin, &x[0], &y[0]);  
    cll2xy(&bg->stcprm, bg->latmin, bg->lonmax, &x[1], &y[1]);  
    cll2xy(&bg->stcprm, bg->latmin, (bg->lonmin+bg->lonmax)/2, &x[2], &y[2]);  
    cll2xy(&bg->stcprm, bg->latmax, bg->lonmax, &x[3], &y[3]);  
  } else if (bg->latmax <= 0) { /* S. hemisphere */
    cll2xy(&bg->stcprm, bg->latmax, bg->lonmin, &x[0], &y[0]);  
    cll2xy(&bg->stcprm, bg->latmax, bg->lonmax, &x[1], &y[1]);  
    cll2xy(&bg->stcprm, bg->latmin, bg->lonmax, &x[2], &y[2]);  
    cll2xy(&bg->stcprm, bg->latmax, (bg->lonmin+bg->lonmax)/2, &x[3], &y[3]); 
    set_y = y[3]-y[2] + 2; 

  } else { /* crosses equator */
    cll2xy(&bg->stcprm, 0, bg->lonmin, &x[0], &y[0]);  
    cll2xy(&bg->stcprm, 0, bg->lonmax, &x[1], &y[1]);  
    cll2xy(&bg->stcprm, bg->latmin, bg->lonmin, &x[2], &y[2]);  
    cll2xy(&bg->stcprm, bg->latmax, bg->lonmin, &x[3], &y[3]);  
 ; 
  }
  } else {
/*
 *    slices greater than 180 degrees
 *    
 *     Northern hemisphere
 *     
 *   
 */
  if (bg->latmin >= 0)/* N. hemisphere */
  {
    cll2xy(&bg->stcprm, bg->latmin,(bg->lonmin+bg->lonmax)/2-90, &x[0], &y[0]);
    cll2xy(&bg->stcprm, bg->latmin,(bg->lonmin+bg->lonmax)/2+90, &x[1], &y[1]);
    cll2xy(&bg->stcprm, bg->latmin, (bg->lonmin+bg->lonmax)/2, &x[2], &y[2]);
    cll2xy(&bg->stcprm, bg->latmin, bg->lonmax, &x[3], &y[3]);
  } else if (bg->latmax <= 0) { /* S. hemisphere */
    cll2xy(&bg->stcprm, bg->latmin,(bg->lonmin+bg->lonmax)/2-90, &x[0], &y[0]);
    cll2xy(&bg->stcprm, bg->latmin,(bg->lonmin+bg->lonmax)/2+90, &x[1], &y[1]);
		cll2xy(&bg->stcprm, bg->latmax,bg->lonmin, &x[2], &y[2]);
		cll2xy(&bg->stcprm, bg->latmax, (bg->lonmin+bg->lonmax)/2, &x[3], &y[3]);
  } else { /* crosses equator */
		puts("ERROR: max(lon)-min(lon) > 180 AND crosses equator; I cannot handle that (yet?)");
				exit(0);
		}
	}
  bg_p.xsize = ceilf((x[1]-x[0])/16)*16;
  bg_p.ysize = ceilf((y[3]-y[2]+y_buf_pix)/16)*16;
  /* need this to find the index in xy2idx() */
  bg_p.cc = bg->cc;
  bg_p.length = bg_p.xsize*bg_p.ysize*bg->cc;

  /* re-create the grid, changing the x_scale point to the new image size,
   * which is probably smaller than the original 
   */
   
  set_x = bg_p.xsize/2;
  
  stcm1p(&bg->stcprm,     /* the map projection structure */
           set_x,       /* the 'x' point we are fixing */
	   set_y,       /* the 'y' point we are fixing */ 
	   set_lat,     /* the lat that (x,y) above correspond to */
           set_lon,     /* the lon that (x,y) above correspond to */
           scale_lat,     /* perform grid scaling here */ 
	   scale_lon,     /* perform grid scaling here */
	   grid_scale,    /* grid scaling (km/grid == km/pixel) */
	   0);            /* degree orientation, set North upwards */
	   
	   
  /* create a blank image that will store the newly projected image */
  bg_p.rgb = (JSAMPLE*)calloc(bg_p.length, sizeof(JSAMPLE));
  /* create an array that stores the number of old-image pixels that have 
   * been mapped to this pixel */
/*  bg_p_cnt = (int*)calloc(bg_p.length, sizeof(int)); */
  bg_p.mark = (int*)calloc(bg_p.length, sizeof(int));
  
  if (!bg_p.rgb || !bg_p.mark)
  {
    puts("ERROR: make_projection() failed to allocate memory");
    exit(0);
  }
  
  /* loop through all the pixels, mapping the original value to the new 
   * projected value.  If the new value has already been marked, weigh each
   * new value appropriately */
  
  for (o_idx = 0; o_idx<bg->length; o_idx++)
  {
    /* pixels start in the upper, right corner going across, so the last value
     * is the lower,right corner */
    idx2ll(bg, o_idx, &latToConvert, &lonToConvert, &rgb_flag);
    cll2xy(&bg->stcprm, latToConvert, lonToConvert, &xOut, &yOut);
    if (xOut < 0 || xOut > bg_p.xsize || yOut < 0 || yOut > bg_p.ysize)
    {
      puts("ERROR: make_projection(): cll2xy() return unusable values");
      exit(0);
    } else {
    p_idx = xy2idx(&bg_p, xOut, yOut) + rgb_flag;
    if (bg_p.mark[p_idx] == 0)
    {
      bg_p.rgb[p_idx] = bg->rgb[o_idx];
    } else {
      frac = (float)bg_p.mark[p_idx]/((float)bg_p.mark[p_idx]+1);
      bg_p.rgb[p_idx]=(char)( frac*bg_p.rgb[p_idx]+(1-frac)*bg_p.rgb[p_idx]);
      
    }
    bg_p.mark[p_idx]++;
  }
  }

  
  /* try to patch */
  if (arguments.patch != PATCH_USER_NO)
  {
    patch_image(&bg_p);
  }
  
  /* replace old image with new one */
  free(bg->rgb);
  bg->rgb = bg_p.rgb;
  bg->xsize = bg_p.xsize;
  bg->ysize = bg_p.ysize;
  bg->length = bg_p.xsize*bg_p.ysize*bg->cc;

  /* done with array, free it */
  free(bg_p.mark);
 
  return;
}

/***************************************************/
/* point x,y = (0,0) is upper left, which is (lonmin, latmax) */
/***************************************************/
void idx2xy(struct bg_img *bg, int idx, int *x, int *y)
{
  *y = bg->ysize - 1 - (int)(floorf(idx/(bg->cc*bg->xsize) ));
  *x = (int)((idx/3)%bg->xsize );
  if (*x < 0 || *x > bg->xsize-1 || *y < 0 || *y > bg->ysize-1)
  {
    puts("ERROR: indx2xy() failed");
    exit(0);
  }
  return;
}

/***************************************************/
void idx2ll(struct bg_img *bg, int idx, double *lat, double *lon, int *rgb_flag)
/***************************************************/
{
  int x, y;
  
  *rgb_flag = idx%3;
  idx2xy(bg, idx, &x, &y);
  *lat = bg->latmin + ((float)y/(float)bg->ysize)*(bg->latmax - bg->latmin);
  *lon = bg->lonmin + ((float)x/(float)bg->xsize)*(bg->lonmax - bg->lonmin);
  
  if (*lat < bg->latmin || *lat > bg->latmax ||
      *lon < bg->lonmin || *lon > bg->lonmax)
  {
    puts("ERROR: idx2ll() failed");
    exit(0);
  }
  return;
}
/***************************************************/
int xy2idx(struct bg_img *bg_p, int x, int y)
{
  int idx;
  idx = ((bg_p->ysize-y)*bg_p->xsize*bg_p->cc)+(x*bg_p->cc);
  if (idx < 0 || idx > bg_p->length-1)
  {
    puts("ERROR: xy2idx() failed");
    exit(0);
  }
  return idx;
}
/***************************************************/
void get_image_size(struct bg_img *bg, struct bg_img *bg_p)
{
  double x[4], y[4];

  if (bg->latmin >= 0) /* N. hemisphere */
  {
    cll2xy(&bg->stcprm, bg->latmin, bg->lonmin, &x[0], &y[0]);  
    cll2xy(&bg->stcprm, bg->latmin, bg->lonmax, &x[1], &y[1]);  
    cll2xy(&bg->stcprm, bg->latmin, (bg->lonmin+bg->lonmax)/2, &x[2], &y[2]);  
    cll2xy(&bg->stcprm, bg->latmin, bg->lonmax, &x[3], &y[3]);  
  } else if (bg->latmax <= 0) { /* S. hemisphere */
    cll2xy(&bg->stcprm, bg->latmax, bg->lonmin, &x[0], &y[0]);  
    cll2xy(&bg->stcprm, bg->latmax, bg->lonmax, &x[1], &y[1]);  
    cll2xy(&bg->stcprm, bg->latmin, bg->lonmax, &x[2], &y[2]);  
    cll2xy(&bg->stcprm, bg->latmax, (bg->lonmin+bg->lonmax)/2, &x[3], &y[3]);  

  } else { /* crosses equator */
    cll2xy(&bg->stcprm, 0, bg->lonmin, &x[0], &y[0]);  
    cll2xy(&bg->stcprm, 0, bg->lonmax, &x[1], &y[1]);  
    cll2xy(&bg->stcprm, bg->latmin, bg->lonmin, &x[2], &y[2]);  
    cll2xy(&bg->stcprm, bg->latmax, bg->lonmin, &x[3], &y[3]);  
  }

  bg_p->xsize = ceilf((x[1]-x[0])/16)*16;
  bg_p->ysize = ceilf((y[3]-y[2])/16)*16;
  /* need this to find the index in xy2idx() */
  bg_p->cc = bg->cc;
  bg_p->length = bg_p->xsize*bg_p->ysize*bg->cc;
    
  return;
}
/***************************************************/

