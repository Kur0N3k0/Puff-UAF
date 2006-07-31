/*
 * -- puff : software for ash tracking and simulation
 *    Copyright (C) 1999 Craig Searcy 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 
 * 
 * See also the file license.terms
 * 
 * Craig Searcy   
 * National Weather Service, Forecast Office
 * 6930 Sand Lake Road
 * Anchorage, AK 99502-1845
 *
 * craig.searcy@noaa.gov
 *
 * April 1999
 *
 */
#include <cstdlib>
#include <iostream>
#include <ctime>
#include <cmath>

#include "ran_utils.h"

////////////////////////////////////////////////////////////////////////
// This is the random number generator from Numerical Recipes in C,
// update with some minimal C++ stuff
////////////////////////////////////////////////////////////////////////
float ran1(int &idum) {
	static long ix1,ix2,ix3;
	static float r[98];
	float temp;
	static int iff=0;
	int j;

	if (idum < 0 || iff == 0) {
		iff=1;
		ix1=(IC1-(idum)) % M1;
		ix1=(IA1*ix1+IC1) % M1;
		ix2=ix1 % M2;
		ix1=(IA1*ix1+IC1) % M1;
		ix3=ix1 % M3;
		for (j=1;j<=97;j++) {
			ix1=(IA1*ix1+IC1) % M1;
			ix2=(IA2*ix2+IC2) % M2;
			r[j]=(ix1+ix2*RM2)*RM1;
		}
		idum=1;
	}
	ix1=(IA1*ix1+IC1) % M1;
	ix2=(IA2*ix2+IC2) % M2;
	ix3=(IA3*ix3+IC3) % M3;
	j= 1 + int(((97*ix3)/M3));
	if (j > 97 || j < 1) {
		std::cerr << "RAN1: This cannot happen.\n" ;
		exit(1);
	}
	temp=r[j];
	r[j]=(ix1+ix2*RM2)*RM1;
	return temp;
};

#undef M1
#undef IA1
#undef IC1
#undef RM1
#undef M2
#undef IA2
#undef IC2
#undef RM2
#undef M3
#undef IA3
#undef IC3


////////////////////////////////////////////////////////////////////////////
// This routine is also from Numerical Recipes in C, it returns
// a gaussian distribution with 0 mean
////////////////////////////////////////////////////////////////////////////

float gasdev(int &idum) {
        static int iset=0;
        static float gset;
        float fac,r,v1,v2;

        if  (iset == 0) {
                do {
                        v1=2.0*ran1(idum)-1.0;
                        v2=2.0*ran1(idum)-1.0;
                        r=v1*v1+v2*v2;
                } while (r >= 1.0 || r == 0.0);
                fac=sqrt(-2.0*log(r)/r);
                gset=v1*fac;
                iset=1;
                return v2*fac;
        } else {
                iset=0;
                return gset;
        }
};

////////////////////////////////////////////////////////////////////////////
// This routine inits the random seed by the system time(NULL)
// It returns an integer between -1000 and +1000
////////////////////////////////////////////////////////////////////////////

void init_seed(int& iseed, int set) {
    time_t now = time(NULL);
    long trunc = now/1000;
    int tmpseed;
    if (set) { tmpseed = set; }
    else { tmpseed = int(now - 1000*trunc); }
    
    if (tmpseed > 1000 || tmpseed < -1000) {
	std::cout << "init_seed() WARNING: out of bounds,  set iseed = 123\n";
	iseed = 123;
    }
    else {
	int tmpdum = tmpseed;
	float pm = ran1(tmpdum);
	if (pm < 0.5) {
	    iseed = -tmpseed;
	}
	else {
	    iseed = tmpseed;
	}
    }
}

////////////////////////////////////////////////////////////////////////////
// This routine is also from Numerical Recipes in C, it returns
// a exponential distribution with unit mean
////////////////////////////////////////////////////////////////////////////
float expdev(int &idum) {
    static float dum;
    do
	dum = ran1(idum);
    while (dum == 0.0);
    return -log(dum);
}

////////////////////////////////////////////////////////////////////////////
// This routine is also from Numerical Recipes in C, it returns
// a poisson distribution with unit mean
////////////////////////////////////////////////////////////////////////////
float gammln(float xx) {
	double x,tmp,ser;
	static double cof[6]={76.18009173,-86.50532033,24.01409822,
		-1.231739516,0.120858003e-2,-0.536382e-5};
	int j;

	x=xx-1.0;
	tmp=x+5.5;
	tmp -= (x+0.5)*log(tmp);
	ser=1.0;
	for (j=0;j<=5;j++) {
		x += 1.0;
		ser += cof[j]/x;
	}
	return -tmp+log(2.50662827465*ser);
}


float poidev(float xm, int &idum) {
	static float sq,alxm,g,oldm=(-1.0);
	float em,t,y;


	if (xm < 12.0) {
		if (xm != oldm) {
			oldm=xm;
			g=exp(-xm);
		}
		em = -1;
		t=1.0;
		do {
			em += 1.0;
			t *= ran1(idum);
		} while (t > g);
	} else {
		if (xm != oldm) {
			oldm=xm;
			sq=sqrt(2.0*xm);
			alxm=log(xm);
			g=xm*alxm-gammln(xm+1.0);
		}
		do {
			do {
				y=tan(PI*ran1(idum));
				em=sq*y+xm;
			} while (em < 0.0);
			em=floor(em);
			t=0.9*(1.0+y*y)*exp(em*alxm-gammln(em+1.0)-g);
		} while (ran1(idum) > t);
	}
	return em;
}
////////////////////////////////////////////////////////////////////////////
// return a possion distribution of over a range double the mean
// Poisson definition is the following:
// P = x^m/m! * exp[-x]
////////////////////////////////////////////////////////////////////////////
float poi_dist(float xm, int &idum) {
  double val1, val2, prob;
  
  while (1) {
    // get a random number between 0 and 3-times the average, which covers
    // a reasonable range for our purposes here.
    val1=3*xm*ran1(idum);
    prob = pow((double)xm,val1)*exp(-xm)/factorial(val1);
    val2 = ran1(idum);
    if (prob > val2) break;
    continue;
    }
    return (float)val1;
  }
  
float factorial(float f) {
  float value = 0;
  value = f*exp(gammln(f));
  // HUGE should be defined in cmath header
  if (value > HUGE) {
    std::cerr << "overflow error while calculating factorial\n";
    exit(1);
    }
  return value;
  }
