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

#ifndef CLOUD_H
#define CLOUD_H

// 'iseed' is declared in the Ash class already. delete if/when integrated
// we may also not need the ran_utils.h included above

struct Segment {
  float x1, y1, x2, y2, slope, intercept;
  };
    
class Cloud {
  int inside, nElev, nSegments;  
  std::vector<Particle> elevPoint;
  double *lonlatValues, mean, sdev;
  float testX, testY, elevEqnA, elevEqnB, elevEqnC;
  float lengthScale;
  std::vector<Segment> segment;
  char *restartFile;

  void build();
  double elevation(double, double);
  void  readPoints();
  int   inCloud(double, double);  
  void  makeSlopes();
  int   define(double, double);
  float distance(Particle);
  void  setLengthScale();
  void  sortElevations();
  bool  validRegion();
  
public:
  Cloud(long, char*);
  ~Cloud();
  void fill(int);
  double* customCloud(long);
  float size();

};

#endif 
