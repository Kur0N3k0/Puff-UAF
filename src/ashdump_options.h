/****************************************************************************
    puff - a volcanic ash tracking model
    Copyright (C) 2001-2003 Rorik Peterson <rorik@gi.alaska.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
****************************************************************************/

#ifndef ASHDUMP_OPTIONS_H
#define ASHDUMP_OPTIONS_H
#include <cstring>
#include <cstdlib>

struct Argument {
  bool age, airborne, fallout, feet, hdr, lat, lon, stats, showHeight;
  bool showParams, showSize;
  char *height, *infile, *range, *size; 
  int precision, width;
  };
    
void parse_options(int argc, char **argv);
void set_defaults(struct Argument *argument);
void show_help();
void ashdump_usage();

enum keyWords { ASHDUMP_AGE, AIRBORNE, INFILE, FALLOUT, FEET, HDR, HEIGHT, HELP, ASHDUMP_LAT, ASHDUMP_LON, PRECISION, RANGE, SHOWPARAMS, SIZE, STATS, ASHDUMP_SZ, USAGE, VARIABLE_LIST, VERSION_ASHDUMP, WIDTH, ASHDUMP_Z };

#endif /* ASHDUMP_OPTIONS_H */
