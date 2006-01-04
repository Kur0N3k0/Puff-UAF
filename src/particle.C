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
    
    This program is an extension of point.C from Craig Searcy's original version
    
****************************************************************************/

#include "particle.h"

Particle::Particle() {
    x = 0;
    y = 0;
    z = 0;
    size = 0x0;
    startTime = 0x0;
    order = 0x0;
    grounded = false;
    exists = true;
}

Particle::Particle(double xx, double yy, double zz) {
    x = xx;
    y = yy;
    z = zz;
}

Particle::~Particle() {
    // do nothing
}

Particle & Particle::operator=(const Particle & pnt) {
    x = pnt.x;
    y = pnt.y;
    z = pnt.z;
    return *this;
}

Particle & Particle::operator+(Particle & pnt) {
    x = x + pnt.x;
    y = y + pnt.y;
    z = z + pnt.z;
    return *this;
}

Particle & Particle::operator-(Particle & pnt) {
    x = x - pnt.x;
    y = y - pnt.y;
    z = z - pnt.z;
    return *this;
}

