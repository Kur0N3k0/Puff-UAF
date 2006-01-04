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

#ifndef UNI2PUFF_OPTIONS_H
#define UNI2PUFF_OPTIONS_H
struct Argument {
  char Hfile[256], Hvar[256], path[256], range[256], unifile[256], var[256];
  double Hconst, P0;
  bool Hsimple, ncep, nmc, noPatch, shiftWest;
  };
    
void parse_options(int argc, char **argv);
void set_defaults(struct Argument *argument);
void show_help();
void uni2puff_usage();

enum keyWords { HELP, HCONST, HFILE, HSIMPLE, HVAR, NCEP, NMC, NOPATCH, PZERO, PATH, RANGE, SHIFTWEST, UNIFILE, VARIABLE, UNI2PUFF_VERSION };

static const char uni2puff_version_number[] = "2.0";

#endif /* UNI2PUFF_OPTIONS_H */
