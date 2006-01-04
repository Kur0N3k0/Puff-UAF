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

#include <string.h> /* strcmp() */
#include <math.h>  /* log() */
#include <unistd.h> /*access() */
#include "ashxp.h"
#include "color.h"

#ifdef HAVE_LIBFREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
void get_ft_bitmap(struct bg_img *, char *);
#else
#include "numbers.dat"
#endif

#ifdef HAVE_CEILF
extern float ceilf(float);
#endif

/***************************************************/
/* weighted color */
/***************************************************/
void wgt_color(JSAMPLE* rgb, float* wgt) {
  /* available color options */

  if (coloring_scheme==COLOR_PROTOCOL_GRAY) {
    rgb[0]=*wgt*255;
    rgb[1]=*wgt*255;
    rgb[2]=*wgt*255;
    }
  else if (coloring_scheme==COLOR_PROTOCOL_RED) {
    rgb[0]=*wgt*255;
    rgb[1]=0;
    rgb[2]=0;
    }
  else if (coloring_scheme==COLOR_PROTOCOL_ISOPACH) {
    if (*wgt < 0.0316) {
       /* black */
       rgb[0] = rgb[1] = rgb[2] = 0;
       }
     else if (*wgt < 0.10) {
       /* blue */
       rgb[0] = rgb[1] = 0;
       rgb[2] = 255;
       }
     else if (*wgt < 0.316) {
       /* green */
       rgb[0] = rgb[2] = 0;
       rgb[1] = 255;
       }
     else {
       /* red */
       rgb[0] = 255;
       rgb[1] = rgb[2] = 0;
       }
     }
  else if (coloring_scheme==COLOR_PROTOCOL_RAINBOW) {
    if (*wgt < 0.125) {
      /* purple */
      rgb[0]=255;
      rgb[1]=0;
      rgb[2]=255;
      }
    else if (*wgt < 0.25) {
    /* blue */
      rgb[0]=0;
      rgb[1]=0;
      rgb[2]=255;
      }
    else if (*wgt < 0.375) {
    /* lt blue */
      rgb[0]=0;
      rgb[1]=191;
      rgb[2]=255;
      }
    else if (*wgt < 0.5) {
    /* cyan */
      rgb[0]=127;
      rgb[1]=255;
      rgb[2]=255;
      }
    else if (*wgt < 0.625) {
    /* green */
      rgb[0]=0;
      rgb[1]=255;
      rgb[2]=0;
      }
    else if (*wgt < 0.75) {
    /* yellow */
      rgb[0]=255;
      rgb[1]=255;
      rgb[2]=0;
      }
    else if (*wgt < 0.875) {
    /* orange */
      rgb[0]=255;
      rgb[1]=127;
      rgb[2]=63;
      }
    else {
     /* red */
      rgb[0]=255;
      rgb[1]=0;
      rgb[2]=0;
      }
    }
  else {
     puts("ERROR: unknown coloring scheme");
    exit(EXIT_FAILURE);
     }
  return;
  }
/***************************************************/
void scale_shading(int *index, struct bg_img *img) {
  int i;
  double max, min;
  double ln_max, ln_min, frac;
  const static int nlevels = 4;  
  float *interval;
  max = 0;
  min = 999999;
  interval = calloc((nlevels+1),sizeof(float));
    
  /* find min/max values, skipping first value because of initialization */
  for (i=1; i<img->xsize*img->ysize; i++) {
    if (index[i] != 0) {
      if (max < (double)index[i]) max=(double)index[i]; 
      if (min > (double)index[i]) min=(double)index[i];
      }
    }
  
  ln_max = log(max);
  ln_min = log(min);
  
  /* find level intervals */
  for (i=0; i <= nlevels; i++) {
    frac = (double)i/nlevels;
    interval[i] = (float)exp(frac*(ln_max-ln_min)+ln_min);
    }
 
  /* isopach map */
  for (i=0; i<img->xsize*img->ysize; i++) {
    if (index[i] > interval[1]) {
      if (index[i]<interval[2]) { 
        /* blue */
        img->rgb[i*3] = img->rgb[i*3+1] = 0;
	img->rgb[i*3+2] = 255;
      } else if (index[i]<interval[3]) {
        /* green */
        img->rgb[i*3] = img->rgb[i*3+2] = 0;
	img->rgb[i*3+1] = 255;
      } else if (index[i]<=interval[4]) {
        /* red */
        img->rgb[i*3]=255;
	img->rgb[i*3+1] = img->rgb[i*3+2] = 0;
	} else {
        puts("Error determining isopach level");
	exit(1);
	}
    }
  }

  return;
  } 
  
/***************************************************/
/* color one pixel this color */

void color1(JSAMPLE *rgb)
{
  switch (gridline_color)
  {
    case GRIDLINE_BLACK:
      rgb[0]=0;
      rgb[1]=0;
      rgb[2]=0;
      break;
    case GRIDLINE_BLUE:
      rgb[0]=0;
      rgb[1]=0;
      rgb[2]=0xFF;
      break;
    case GRIDLINE_GREEN:
      rgb[0]=0;
      rgb[1]=0xFF;
      rgb[2]=0;
      break;
    case GRIDLINE_PURPLE:
      rgb[0]=0xFF; 
      rgb[1]=0x66;
      rgb[2]=0xCC; 
      break;
    case GRIDLINE_RED:
      rgb[0]=0xFF;
      rgb[1]=0;
      rgb[2]=0;
      break;
    case GRIDLINE_WHITE:
      rgb[0]=0xFF;
      rgb[1]=0xFF;
      rgb[2]=0xFF;
      break;
    case GRIDLINE_YELLOW:
      rgb[0]=0xFF;
      rgb[1]=0xFF;
      rgb[2]=0x00;
      break;
    default: /* default to white */
      rgb[0]=0xFF;
      rgb[1]=0xFF;
      rgb[2]=0xFF;
      break;
  }
  return;
}
#ifdef HAVE_LIBFREETYPE
/***************************************************/
/* add a colorbar to the background image.  The min/max ash heights are used */
void add_colorbar(struct bg_img *img) 
{
  int i, j, index; /* index counters */
  int temphgt;
  int barx = 0, bary = 0;  /* color bar dimensions */
  float wgt;
	struct bg_img bm_text; /* bitmap image */
	char *text = calloc(15, sizeof(char)); /* string for print characters */
	int default_colorbar = 1;
	int user_set_colorbar_size = 0;

	/* let user set colorbar size */
	if (arguments.colorbar_size)
	{
		default_colorbar = 0;
		user_set_colorbar_size = 1;
	  sscanf(arguments.colorbar_size, "%ix%i", &barx, &bary);
	}
	/* check colorbar sizes */		
	if (barx < 0 || barx > img->xsize) default_colorbar = 1; 
	if (bary < 0 || bary > img->ysize) default_colorbar = 1; 
	/* make the colorbar scale with the image size */
	if (default_colorbar)
	{
	  barx = img->xsize / 4;
	  bary = barx / 4;
		if (user_set_colorbar_size)
			printf("WARNING: bad values for option --colorbar-size: \"%s\"\n",arguments.colorbar_size);
	}
  
  if (img->ysize < 25 || barx < 10 )
  {
    puts("WARNING: image is too small to create a colorbar.  Try using the ");
    puts("--magnify option");
    return;
  }
  
  index = img->cc*img->xsize*(img->ysize-bary);
  wgt = 0;
  for (i=0; i<barx; i++) {
    for (j=img->ysize-bary; j<img->ysize; j++) {
      wgt_color(&(img->rgb[index]),&wgt);
      index+=(img->cc)*(img->xsize);
      }
    index = (img->cc)*(img->xsize*(img->ysize-bary)+i);
    wgt = (float)i/barx;
    }

/* print the minimum value */  
  index = (img->cc)*( (img->xsize) * (img->ysize - bary - arguments.fontsize)+1 ); 
  temphgt = floorf(img->hgtmin/1000);
	sprintf(text, "%d", temphgt);
	/* isopach maps just print 0% */
	if (coloring_scheme==COLOR_PROTOCOL_ISOPACH) strcpy(text, "0%");
	get_ft_bitmap(&bm_text, text);
	put_bitmap(&bm_text, img, index);
	/* free memory */
	free(bm_text.rgb);

  /* print the maximum value */
  index = (img->cc)*( (img->xsize) * (img->ysize - bary - arguments.fontsize) + (barx-10) ); 
  /* use ceilf so color bar extends just beyond the highest particle */
  temphgt = ceilf(img->hgtmax/1000);
	sprintf(text, "%dkm", temphgt);
	/* isopach maps just print 99% */
	if (coloring_scheme==COLOR_PROTOCOL_ISOPACH) strcpy(text, "99%");
	get_ft_bitmap(&bm_text, text);
	put_bitmap(&bm_text, img, index);
	/* free memory */
	free(bm_text.rgb);

  return;
  
}

/***************************************************/
void add_date_time(struct bg_img *img, char* dtstr) 
{
  int i, j; /* index counters */
  struct bg_img bm_text;
  int b_idx, t_idx;
  static const int cc = 3;
  char *dtstr_new;
  /* make a pretty date/time out of dtstr if it is YYYYMMDDHHMM */
  if (strlen(dtstr) == 12)
  {
     dtstr_new = (char*)calloc(17,sizeof(char));
     strncpy(dtstr_new, dtstr, 4);
     strcat(dtstr_new, " ");
     strncat(dtstr_new, &dtstr[4], 2);
     strcat(dtstr_new, " ");
     strncat(dtstr_new, &dtstr[6], 2);
     strcat(dtstr_new, " ");
     strncat(dtstr_new, &dtstr[8], 2);
     strcat(dtstr_new, ":");
     strncat(dtstr_new, &dtstr[10], 2);
   } else {
     dtstr_new = strdup(dtstr);
   }
  
  /* get a bitmap image of the text 'dtsrt' into the bitmap image 'bm_text' */
  get_ft_bitmap(&bm_text, dtstr_new);
  b_idx = (img->xsize*(img->ysize-bm_text.ysize) - bm_text.xsize)*cc;
	put_bitmap(&bm_text, img, b_idx);
	/* free memory */
	free(bm_text.rgb);
  
  return;
}
/****************************************************
 * print the text 'text' as a freetype bitmap, and return the dimensions in
 * 'xsize' and 'ysize' of the bg_img struct.  Ignore the other parts. Space is
 * allocated here because we do not know how big the bitmap image is yet
 */
void get_ft_bitmap(struct bg_img *bmap, char *text)
{
  static FT_Library library;
  static FT_Face face;
  FT_UInt glyph_index;
  static int initialized = 0;
  int space_width = arguments.fontsize / 3;
  
  int i, j, k,	/* counters for the 2D bitmap for each character */
      loc,	/* location with the text bitmap */
      height,	/* height of a single character in pixels */
      x_idx,	/* index in the x-direction */
      c_idx;	/* character index in the text */

  static const int cc = 3;
  
  if (!initialized)
  {
    /* make sure the font file is available */
    if (access(arguments.fontfile, R_OK))
    {
      printf("Cannot read fontfile \"%s\".",arguments.fontfile);
      exit(1);
    }
    FT_Init_FreeType(&library);
    FT_New_Face(library, 
                arguments.fontfile,
  	        0,
	        &face);
    FT_Set_Pixel_Sizes(face,0,arguments.fontsize);
    initialized = 1;
  }
  FT_GlyphSlot slot = face->glyph;
  bmap->xsize = 0;
  bmap->ysize = 0;
  
  /* first get the size, then we'll allocate the space and actually write it */
  height = 0;
  for (c_idx = 0; c_idx < strlen(text); c_idx++)
  {
    glyph_index = FT_Get_Char_Index(face, text[c_idx]);
    FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
    FT_Render_Glyph( face->glyph, ft_render_mode_normal );
    /* add up width of all characters, spaces are zero-width, so add our own
     * value */
    if (slot->bitmap.width > 0)  bmap->xsize += (slot->bitmap.width);
    else bmap->xsize += space_width;
		/* height is the maximum height of all characters */
    if (height < slot->bitmap_top) height = slot->bitmap_top;
  }
  
  /* use fontsize as the vertical height needed */
  bmap->ysize = arguments.fontsize;
  /* allocate the space */
  bmap->rgb = (JSAMPLE*)calloc(bmap->xsize*bmap->ysize*cc,sizeof(JSAMPLE));
  bmap->length = bmap->xsize*bmap->ysize*cc;
  
  /* location within the text bitmap */
  x_idx = 0;
  
  /* print each character one at a time */
  for (c_idx = 0; c_idx < strlen(text); c_idx++)
  {
    loc = x_idx;
    glyph_index = FT_Get_Char_Index(face, text[c_idx]);
    FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
    FT_Render_Glyph( face->glyph, ft_render_mode_normal );
      /* advance down enough rows to start marking */
    for (j=0; j<height-slot->bitmap_top; j++) 
        { loc+=cc*(bmap->xsize); }
    for (i = 0; i < slot->bitmap.rows; i++) 
    {
      for (j = 0; j < slot->bitmap.width; j++) 
      {
        k = i*slot->bitmap.width + j; /* index within this char bitmap */
        if (slot->bitmap.buffer[k] != '\0') 
        {
          bmap->rgb[loc] = slot->bitmap.buffer[k]; 
	  bmap->rgb[loc+1]=slot->bitmap.buffer[k];
	  bmap->rgb[loc+2]=slot->bitmap.buffer[k];
        }
	loc += 3; /* advance one pixel to the right */

      }
      /* advance one row down */
      loc+=cc*(bmap->xsize - slot->bitmap.width);
    }
  if (slot->bitmap.width > 0) x_idx += cc*(slot->bitmap.width);
  else x_idx += cc*space_width;
  }  
  return;
}
/***************************************************/
/* put the bitmap 'map' onto the background 'bg' with the 
 * upper left-hand corner at 'idx'
 */
void put_bitmap(struct bg_img *map, struct bg_img *bg, int idx)
{
  int m_idx = 0; /* map index */
	int b_idx = idx; /* background index */
	int i, j; /* counting indecies */
	int off_page; /* indicate if we're off the page */

  for (i = 0; i < map->ysize; i++)
  {
		off_page = 0;
    for (j = 0; j < map->xsize; j++) 
    {
			/* make sure we don't run off the right side of page */
			if ((b_idx/bg->cc)%(bg->xsize) == 0) 
			{
				off_page = 1;
			}
			
      if ((map->rgb[m_idx] != '\0') && !off_page)
      {
				bg->rgb[b_idx] = map->rgb[m_idx];
				bg->rgb[b_idx+1] = map->rgb[m_idx];
				bg->rgb[b_idx+2] = map->rgb[m_idx];
      }
      /* adnvace one pixel to the right */
      m_idx += 3;
      b_idx += 3;
    }
    /* advance one pixel down */
    b_idx += (bg->xsize-map->xsize)*bg->cc;
  }

	return;
}

/***************************************************/
void add_labels(struct bg_img *bg)
{
	if (arguments.labelc == 0) return;
	float lat, lon;
	double x, y;
	int index, l_idx;
	struct bg_img bm; /* bitmap image of this text */
	char *text = calloc(128,sizeof(char));
	char *comma; /* hack to allow spaces in the text string */
	for (l_idx = 0; l_idx < arguments.labelc; l_idx++)
	{
	if (sscanf(arguments.labelv[l_idx],"%f,%f,%s", &lon, &lat, text) != 3)
	{
		if (!arguments.quiet)
			printf("WARNING: bad label specified: '%s'\n",arguments.labelv[l_idx]);
	} else {
	/* find the last comma, and make the rest the text string. This is
	 * a work-around to allow spaces in the text */
  comma = strrchr(arguments.labelv[l_idx],',');
	comma++;
	strcpy(text,comma);
  cll2xy(&bg->stcprm, lat, lon, &x, &y);

	if ((x < 0) || (x > bg->xsize-1) ||
			(y < 0) || (y > bg->ysize-1))
		return;
	
	index = xy2idx(bg, (int)x, (int)y);

	get_ft_bitmap(&bm, text);
	put_bitmap(&bm, bg, index);
	/* free this memory */
	free(bm.rgb);
	}
	}
	return;
}
#else /* not HAVE_LIBFREETYPE */
/***************************************************/
/* add a colorbar to the background image.  The min/max ash heights are used */
void add_colorbar(struct bg_img *img) {
  int i, j, digit, index;
  int temphgt;
  int barx;  /* color bar dimensions */
  int bary = 15 ;
  float wgt;

//  barx = (bg->xsize > 115 ? 100 : bg->xsize - 15);
  barx = (img->xsize > 115 ? 100 : img->xsize - 15);
  
  if (img->ysize < 25 || barx < 10 )
  {
    puts("WARNING: image is too small to create a colorbar.  Try using the ");
    puts("--magnify option");
    return;
  }
  
  index = img->cc*img->xsize*(img->ysize-bary);
  wgt = 0;
  for (i=0; i<barx; i++) {
    for (j=img->ysize-bary; j<img->ysize; j++) {
      wgt_color(&(img->rgb[index]),&wgt);
      index+=(img->cc)*(img->xsize);
      }
    index = (img->cc)*(img->xsize*(img->ysize-bary)+i);
    wgt = (float)i/barx;
    }

  /* if this is an isopach map, print percentages */
  if (coloring_scheme==COLOR_PROTOCOL_ISOPACH) {
    /* print the zero */
    index = (img->cc)*( (img->xsize) * (img->ysize - bary - 2) ); 
    print_number(0, index, img);
    /* print the 99 */
    index = (img->cc)*( (img->xsize) * (img->ysize - bary - 2) + (barx-10) ); 
    print_number(9, index, img);
    index = (img->cc)*( (img->xsize) * (img->ysize - bary - 2) + (barx-4) ); 
    print_number(9, index, img);
    /* print the percentage sign */
    index = (img->cc)*( (img->xsize) * (img->ysize - bary - 2) + (barx+2) );
    print_number(13, index, img);
    return; 
    }
    
/* print the first digit of the minimum value */  
  index = (img->cc)*( (img->xsize) * (img->ysize - bary - 2) ); 
  temphgt = floorf(img->hgtmin/1000);
  digit = temphgt/10;
  /* only print the left-hand digit if non-zero */
  if (digit != 0) {
    print_number(digit, index, img);
    /* advance the pen to the right */
    index = (img->cc)*( (img->xsize) * (img->ysize - bary - 2) + 6 ); 
    }

/* print the second digit of the minimum value */  
  digit = temphgt%10;
  print_number(digit, index, img);

  /* print the first digit of the maximum value */
  index = (img->cc)*( (img->xsize) * (img->ysize - bary - 2) + (barx-10) ); 
  /* use ceilf so color bar extends just beyond the highest particle */
  temphgt = ceilf(img->hgtmax/1000);
  digit = temphgt/10;
  print_number(digit, index, img);

  /* print the second digit of the maximum value */
  index = (img->cc)*( (img->xsize) * (img->ysize - bary - 2) + (barx-4) ); 
  digit = temphgt%10;
  print_number(digit, index, img);

  /* print 'KM' for kilometers */
  index = (img->cc)*( (img->xsize) * (img->ysize - bary - 2) + (barx+2) );
  print_number(10, index, img);
  index = (img->cc)*( (img->xsize) * (img->ysize - bary - 2) + (barx+8) );
  print_number(11, index, img);
  
  return;
  
  }

/***************************************************/
void add_date_time(struct bg_img *img, char* dtstr) {
  int index;
  int number; /* integer number to print */
  int c_idx = 0; /* character index */
  static const int nchar = 12; /* number of characters to print */
  static const int cc = 3;  /* color components */
  static const int dx = 7; /* x-pixels per character, including space */
  
  static const int padx = 5; /* pixel padding on the right */
  static const int pady = 9; /* pixel padding from the bottom */
  char *c;
  
  
  /* make sure there is enough room to write a stamp, which is nchar+4 
   * characters long, each dx pixels wide, with some padx & pady padding 
   */ 
  if ((img->xsize < (nchar+4)*dx+padx+pady) ||
      (img->ysize < dx+pady) )
  {
    puts("WARNING: image is too small to write a time stamp.  Try using the --magnify=INT option");
    return;
  }
    
  c = (char*)calloc(2,sizeof(char));
  /* print YYYY MM DD HH:MM which is 16 characters, or nchar+4 */
  /* put pen at the lower,left character */
  
  index = cc*( img->xsize*(img->ysize - pady)  - dx*(nchar+4) - padx);
  while (c_idx < nchar) {
    strncpy(c, &dtstr[c_idx], 1);
    if (sscanf(c, "%i", &number) == 0 || number > 9 || number < 0) {
      printf("Error creating date/time stamp, %s doesn't look correct\n",dtstr);
      return;
      }
    print_number(number, index, img);
    index += cc*dx;
    if (c_idx == 3 || c_idx == 5 || c_idx == 7) index += cc*dx;
    /* add the colon ':' between hours and minutes */
    if (c_idx == 9) 
    { 
      print_number(12, index, img);
      index += cc*dx;
    }
    c_idx++;
    }
  free(c);
  return;
  }
/***************************************************/
void print_number(int n, int index, struct bg_img *img) 
{
  int i, j, k = 0;
  static const int xdim = 5;
  static const int ydim = 7;
  static const int cc = 3;  /* jpeg cc number */

  /* move index from lower, left corner to upper, left corner */
  index-=cc*img->xsize*ydim;
    
  for (i=0; i<ydim; i++) {
    for (j=0; j<xdim; j++) {
      if (num[n][k] != 0) {
        img->rgb[index] = 255;
        img->rgb[index+1] = 255;
        img->rgb[index+2] = 255;
        }
      k++;
      index+=cc;
      }
    index+=cc*(img->xsize - xdim);
    }
  return;
}


/***************************************************/
void add_labels(struct bg_img *bg)
{
	return;
}
/***************************************************/
#endif /* HAVE_LIBFREETYPE */
