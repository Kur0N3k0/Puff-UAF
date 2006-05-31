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
#include <cmath>
#include "Grid.h"

// UTILITY ROUTINES:
void fg_locate(float *xx, int n, float x, int &j);
int fg_get_nstart(float *x, int nx, int nint, float xx);

//////////////////////////////////////////////////////////////////////
//
// NUMERICAL RECIPES SPLINE ROUTINES: spline and splint
//
//////////////////////////////////////////////////////////////////////
void Grid::spline(float *x, float *y, int n, float yp1, float ypn,
		     float *y2) {
    int i,k;
    float p,qn,sig,un;

    float *u = new float[n];
    if (!u) {
	std::cerr << "ERROR: spline() new failed.\n";
    }
	
    if (yp1 > 0.99e30)
	y2[0]=u[0]=0.0;
    else {
	y2[0] = -0.5;
	u[0]=(3.0/(x[1]-x[0]))*((y[1]-y[0])/(x[1]-x[0])-yp1);
    }
    for (i=1;i<n-1;i++) {
	sig=(x[i]-x[i-1])/(x[i+1]-x[i-1]);
	p=sig*y2[i-1]+2.0;
	y2[i]=(sig-1.0)/p;
	u[i]=(y[i+1]-y[i])/(x[i+1]-x[i]) - (y[i]-y[i-1])/(x[i]-x[i-1]);
	u[i]=(6.0*u[i]/(x[i+1]-x[i-1])-sig*u[i-1])/p;
    }
    if (ypn > 0.99e30)
	qn=un=0.0;
    else {
	qn=0.5;
	un=(3.0/(x[n-1]-x[n-2]))*(ypn-(y[n-1]-y[n-2])/(x[n-1]-x[n-2]));
    }
    y2[n-1]=(un-qn*u[n-2])/(qn*y2[n-2]+1.0);
    for (k=n-2;k>=0;k--)
	y2[k]=y2[k]*y2[k+1]+u[k];
		
    delete[] u;
    return;
};

//////////////////////////////////////////////////////////////////////
void Grid::splint(float *xa, float *ya, float *y2a, int n, float x, 
		     float & y) {
    int klo,khi,k;
    float h,b,a;

    klo=0;
    khi=n-1;
    while (khi-klo > 1) {
	k=(khi+klo) >> 1;
	if (xa[k] > x) khi=k;
	else klo=k;
    }
    h=xa[khi]-xa[klo];
    if (h == 0) {
	std::cerr << "ERROR: splint(): Bad XA input\n";
    }

    a=(xa[khi]-x)/h;
    b=(x-xa[klo])/h;
    y=a*ya[klo]+b*ya[khi]+((a*a*a-a)*y2a[klo]+(b*b*b-b)*y2a[khi])*(h*h)/6.0;
    return;
};


//////////////////////////////////////////////////////////////////////
//
// Grid SPLINE ROUTINES:
//
//////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// SET NUMBER OF INDICE VALUE TO USE FOR SPLINING:
////////////////////////////////////////////////////////////////////////
//void Grid::set_nspline(unsigned int nx1) {
//  cerr << "A deprecated function was called" << endl;
//  exit(1);
//     if (fgNdims != 1) {
// 	fgErrorStrm << "set_nspline(int): expects a 1D object." << endl;
// 	fg_error();
//     }
//     
//     if (nx1 > fgData[FRTIME].size) nx1 = fgData[FRTIME].size;
//     if (nx1 < 2) nx1 = 2;
//     fgNspline[FRTIME] = nx1;
//}

//void Grid::set_nspline(unsigned int nx1, unsigned int nx2) {
//  cerr << "A deprecated function was called" << endl;
//  exit(1);
//     if (fgNdims != 2) {
// 	fgErrorStrm << "set_nspline(int,int): expects a 2D object." << endl;
// 	fg_error();
//     }
//     
//     if (nx1 > fgData[FRTIME].size) nx1 = fgData[FRTIME].size;
//     if (nx1 < 2) nx1 = 2;
//     fgNspline[FRTIME] = nx1;
//    
//     if (nx2 > fgData[LEVEL].size) nx2 = fgData[LEVEL].size;
//     if (nx2 < 2) nx2 = 2;
//     fgNspline[LEVEL] = nx2;
//     
//     // RESET THE SPLINE FLAG:
//     fgResetSpline = 1;
//}

//void Grid::set_nspline(unsigned int nx1, unsigned int nx2, 
//			  unsigned int nx3) {
//  cerr << "A deprecated function was called" << endl;
//  exit(1);
//     if (fgNdims != 3) {
// 	fgErrorStrm << "set_nspline(int,int,int): expects a 3D object." << endl;
// 	fg_error();
//     }
//     
//     if (nx1 > fgData[FRTIME].size) nx1 = fgData[FRTIME].size;
//     if (nx1 < 2) nx1 = 2;
//     fgNspline[FRTIME] = nx1;
//     
//     if (nx2 > fgData[LEVEL].size) nx2 = fgData[LEVEL].size;
//     if (nx2 < 2) nx2 = 2;
//     fgNspline[LEVEL] = nx2;
//     
//     if (nx3 > fgData[LAT].size) nx3 = fgData[LAT].size;
//     if (nx3 < 2) nx3 = 2;
//     fgNspline[LAT] = nx3;
//     
//     // RESET THE SPLINE FLAG:
//     fgResetSpline = 1;
//}

// void Grid::set_nspline(unsigned int nx1, unsigned int nx2, 
// 			  unsigned int nx3, unsigned int nx4) {
//     if (fgNdims != 4) {
// 	fgErrorStrm << "set_nspline(int,int,int,int): expects a 4D object." << endl;
// 	fg_error();
//     }
//     
//     
//     if (nx1 > fgData[FRTIME].size) nx1 = fgData[FRTIME].size;
//     if (nx1 < 2) nx1 = 2;
//     fgNspline[FRTIME] = nx1;
//     
//     if (nx2 > fgData[LEVEL].size) nx2 = fgData[LEVEL].size;
//     if (nx2 < 2) nx2 = 2;
//     fgNspline[LEVEL] = nx2;
//     
//     if (nx3 > fgData[LAT].size) nx3 = fgData[LAT].size;
//     if (nx3 < 2) nx3 = 2;
//     fgNspline[LAT] = nx3;
//     
//     if (nx4 > fgData[LON].size) nx4 = fgData[LON].size;
//     if (nx4 < 2) nx4 = 2;
//     fgNspline[LON] = nx4;
//     
//     // RESET THE SPLINE FLAG:
//     fgResetSpline = 1;
// }

//////////////////////////////////////////////////////////////////////
// GET START INDEX FOR INTERPOLATION:
// This returns the minimum start index for the interpolation routines:
//////////////////////////////////////////////////////////////////////
int fg_get_nstart(float *x, int nx, int nint, float xx) {

    // NUMERICAL RECIPE ROUTINE : LOCATE:
    unsigned int ju, jm, jl, ascnd;
    int n;
    
    jl = 0;
    ju = nx;
    ascnd = ( x[nx-1] > x[0] );
    
    while ( ju-jl > 1 ) {
	jm = (ju+jl) >> 1;
	if ( xx > x[jm] == ascnd ) {
	    jl = jm;
	}
	else {
	    ju=jm;
	}
    }
    n = jl;
    
    // MAKE MY OWN CONDITIONS:
    
    n = n - nint/2 + 1;
    
    if (n < 0) n = 0;
    if ( (n+nint) > nx ) n = nx - nint;
    
   return n;
}


////////////////////////////////////////////////////////////////////////
// 1D INTERPOLATION:
// Does NOT warn on extrapolation
////////////////////////////////////////////////////////////////////////
float Grid::vspline(float xx) {

  std::cerr << "A deprecated function was called" << std::endl;
  exit(1);
// #ifndef FGNOCHECK
//     if ( fgNdims != 1 ) {
// 	fgErrorStrm << "vspline(float): Expects a 1D object." << endl;
// 	fgErrorStrm << "               object \""
// 	            << fgData[VAR].name << "\" is " << fgNdims << "D." << endl;
// 	fg_error();
//     }
// #endif
// 
//     // GET START POSITION:
//     nxStart = fg_get_nstart(fgData[FRTIME].val, fgData[FRTIME].size, fgNspline[FRTIME], xx);
//     
//     // EXECUTE THIS ONLY ONCE TO ALLOCATE fgD2x:
//     if ( splineFlag == 0 ) {		
//     
// 	fgD2x = new float[fgData[VAR].size];
// 	if ( !fgD2x ) {
// 	    fgErrorStrm << "fg_vspline(float): new failed" << endl;
// 	    fg_error();
// 	}
// 	
// 	nxLastStart = nxStart;
// 	
// 	pX = fgData[FRTIME].val + nxStart;
// 	pV = fgData[VAR].val + nxStart;
// 	pD2x = fgD2x + nxStart;
// 	
// 	splineFlag = 1;
//     }
//     
//     // EXECUTE THIS ONCE TO STORE SECOND DERIVATTIVES 
//     // UNLESS VALUES CHANGE
//     if ( fgResetSpline == 1 ) { 
// 	fgResetSpline = 0;
// 	spline(fgData[FRTIME].val, fgData[VAR].val, fgData[FRTIME].size, 
// 	       1.e30, 1.e30, fgD2x);
//     }
//     
//     // CHECK IF NEED TO UPDATE POINTER POSITIONS:
//     if (nxLastStart != nxStart) {
// 	nxLastStart = nxStart;
// 	pX = fgData[FRTIME].val + nxStart;
// 	pV = fgData[VAR].val + nxStart;
// 	pD2x = fgD2x + nxStart;
//     }
//     
//     // RETURN SPLINE VALUE:
//     splint(pX, pV, pD2x, fgNspline[FRTIME], xx, vsplinep);
//    return vsplinep;
};    
    

////////////////////////////////////////////////////////////////////////
// 2D INTERPOLATION:
// Does NOT warn on extrapolation
////////////////////////////////////////////////////////////////////////
float Grid::vspline(float xx, float yy) {

  std::cerr << "A deprecated function was called" << std::endl;
  exit(1);
// #ifndef FGNOCHECK
//     if ( fgNdims != 2 ) {
// 	fgErrorStrm << "vspline(float,float): Expects a 2D object." << endl;
// 	fgErrorStrm << "               object \""
// 	            << fgData[VAR].name << "\" is " << fgNdims << "D." << endl;
// 	fg_error();
//     }
// #endif
// 
//     static int i, j, count;
//     
//     // GET START POSITION:
//     nxStart = fg_get_nstart(fgData[FRTIME].val, fgData[FRTIME].size, fgNspline[FRTIME], xx);
//     nyStart = fg_get_nstart(fgData[LEVEL].val, fgData[LEVEL].size, fgNspline[LEVEL], yy);
// 
//     // EXECUTE THIS ONLY ONCE TO ALLOCATE:
//     if ( splineFlag == 0 ) {
//     
// 	fgD2x = new float[fgData[VAR].size];
// 	if ( !fgD2x ) {
// 	    fgErrorStrm << "fg_vspline(float,float): new failed" << endl;
// 	    fg_error();
// 	}
// 	
// 	fgVal = new float[fgData[VAR].size];
// 	if ( !fgVal ) {
// 	    fgErrorStrm << "fg_vspline(float,float): new failed" << endl;
// 	    fg_error();
// 	}
// 	
// 	// TEMPS, MAX SIZE NECESSARY:
// 	fgTmpLineD2x = new float[fgData[FRTIME].size];
// 	if ( !fgTmpLineD2x ) {
// 	    fgErrorStrm << "fg_vspline(float,float): new failed" << endl;
// 	    fg_error();
// 	}
// 	
// 	fgTmpLineVal = new float[fgData[FRTIME].size];
// 	if ( !fgTmpLineVal ) {
// 	    fgErrorStrm << "fg_vspline(float,float): new failed" << endl;
// 	    fg_error();
// 	}
// 	
// 	nxLastStart = nxStart;
// 	nyLastStart = nyStart;
// 	
// 	pY = fgData[LEVEL].val + nyStart;
// 	pX = fgData[FRTIME].val + nxStart;
// 
// 	splineFlag = 1;
//     }
//     
//     // CHECK IF NEED TO UPDATE POINTER POSITIONS:
//     if (nxLastStart != nxStart) {
// 	nxLastStart = nxStart;
// 	pX = fgData[FRTIME].val + nxStart;
// 	fgResetSpline = 1;
//     }
//     
//     if (nyLastStart != nyStart) {
// 	nyLastStart = nyStart;
// 	pY = fgData[LEVEL].val + nyStart;
// 	fgResetSpline = 1;
//     }
//     
//     // EXECUTE THIS ONCE TO STORE SECOND DERIVATTIVES 
//     // UNLESS VALUES CHANGE
//     if ( fgResetSpline == 1 ) {
// 	fgResetSpline = 0;
// 	
// 	count = 0;
// 	for (i=nxStart; i<nxStart+fgNspline[FRTIME]; i++) {
// 	for (j=nyStart; j<nyStart+fgNspline[LEVEL]; j++) {
// 	    fgVal[count] = fgData[VAR].val[offset(i, j)];
// 	    count++;
// 	}}
// 	
// 	// Second derivative:
// 	for (i=0; i<fgNspline[FRTIME]; i+=fgNspline[LEVEL]) {
// 	    spline(pY, &fgVal[i], fgNspline[LEVEL], 1.e30, 1.e30, &fgD2x[i]);
// 	}
//     }
// 
//     // FILL TEMP LINE VALUE:
//     for (i=0, j=0; j<fgNspline[FRTIME]; i+=fgNspline[LEVEL], j++) {
// 	splint(pY, &fgVal[i], &fgD2x[i], fgNspline[LEVEL], yy, fgTmpLineVal[j]);
//     }
//     
//     // FILL TEMP_LINE_D2X:
//     spline(pX, fgTmpLineVal, fgNspline[FRTIME], 1.e30, 1.e30, fgTmpLineD2x);
// 
//     // SPLINE VALUE:
//     splint(pX, fgTmpLineVal, fgTmpLineD2x, fgNspline[FRTIME], xx, vsplinep);

//    return vsplinep;
}

////////////////////////////////////////////////////////////////////////
// 3D INTERPOLATION:
// Does NOT warn on extrapolation
////////////////////////////////////////////////////////////////////////
float Grid::vspline(float xx, float yy, float zz) {

  std::cerr << "A deprecated function was called" << std::endl;
  exit(1);
// #ifndef FGNOCHECK
//     if ( fgNdims != 3 ) {
// 	fgErrorStrm << "vspline(float,float,float): Expects a 3D object." << endl;
// 	fgErrorStrm << "               object \""
// 	            << fgData[VAR].name << "\" is " << fgNdims << "D." << endl;
// 	fg_error();
//     }
// #endif
//     
//     static int i, j, k, count;
//     
//     // GET START POSITION:
//     nxStart = fg_get_nstart(fgData[FRTIME].val, fgData[FRTIME].size, fgNspline[FRTIME], xx);
//     nyStart = fg_get_nstart(fgData[LEVEL].val, fgData[LEVEL].size, fgNspline[LEVEL], yy);
//     nzStart = fg_get_nstart(fgData[LAT].val, fgData[LAT].size, fgNspline[LAT], zz);
// 
//     // EXECUTE THIS ONLY ONCE TO ALLOCATE:
//     if ( splineFlag == 0 ) {
//     
// 	fgD2x = new float[fgData[VAR].size];
// 	if ( !fgD2x ) {
// 	    fgErrorStrm << "fg_vspline(float,float,float): new failed" << endl;
// 	    fg_error();
// 	}
// 	
// 	fgVal = new float[fgData[VAR].size];
// 	if ( !fgVal ) {
// 	    fgErrorStrm << "fg_vspline(float,float,float): new failed" << endl;
// 	    fg_error();
// 	}
// 	
// 	// TEMPS, MAX SIZE NECESSARY:
// 	fgTmpLineVal = new float[fgData[FRTIME].size];
// 	if ( !fgTmpLineVal ) {
// 	    fgErrorStrm << "fg_vspline(float,float,float): new failed" << endl;
// 	    fg_error();
// 	}
// 	
// 	fgTmpLineD2x = new float[fgData[FRTIME].size];
// 	if ( !fgTmpLineD2x ) {
// 	    fgErrorStrm << "fg_vspline(float,float,float): new failed" << endl;
// 	    fg_error();
// 	}
// 	
// 	fgTmpSrfVal = new float[fgData[FRTIME].size*fgData[LEVEL].size];
// 	if ( !fgTmpSrfVal ) {
// 	    fgErrorStrm << "fg_vspline(float,float,float): new failed" << endl;
// 	    fg_error();
// 	}
// 	
// 	fgTmpSrfD2x = new float[fgData[FRTIME].size*fgData[LEVEL].size];
// 	if ( !fgTmpSrfD2x ) {
// 	    fgErrorStrm << "fg_vspline(float,float,float): new failed" << endl;
// 	    fg_error();
// 	}
// 	nxLastStart = nxStart;
// 	nyLastStart = nyStart;
// 	nzLastStart = nzStart;
// 	
// 	pZ = fgData[LAT].val + nzStart;
// 	pY = fgData[LEVEL].val + nyStart;
// 	pX = fgData[FRTIME].val + nxStart;
// 
// 	
// 	splineFlag = 1;
//     }
//     
//     // CHECK IF NEED TO UPDATE POINTER POSITIONS:
//     if (nxLastStart != nxStart) {
// 	nxLastStart = nxStart;
// 	pX = fgData[FRTIME].val + nxStart;
// 	fgResetSpline = 1;
//     }
//     
//     if (nyLastStart != nyStart) {
// 	nyLastStart = nyStart;
// 	pY = fgData[LEVEL].val + nyStart;
// 	fgResetSpline = 1;
//     }
//     
//     if (nzLastStart != nzStart) {
// 	nzLastStart = nzStart;
// 	pZ = fgData[LAT].val + nzStart;
// 	fgResetSpline = 1;
//     }
//     
//     // EXECUTE THIS ONCE TO STORE SECOND DERIVATTIVES 
//     // UNLESS VALUES CHANGE
//     if ( fgResetSpline == 1 ) {
// 	fgResetSpline = 0;
// 	
// 	count = 0;
// 	for (i=nxStart; i<nxStart+fgNspline[FRTIME]; i++) {
// 	for (j=nyStart; j<nyStart+fgNspline[LEVEL]; j++) {
// 	for (k=nzStart; k<nzStart+fgNspline[LAT]; k++) {
// 	    fgVal[count] = fgData[VAR].val[offset(i, j, k)];
// 	    count++;
// 	}}}
// 	
// 	
// 	// Second derivative:
// 	for (i=0; i<fgNspline[FRTIME]*fgNspline[LEVEL]; i+=fgNspline[LAT]) {
// 	    spline(pZ, &fgVal[i], fgNspline[LAT], 1.e30, 1.e30, &fgD2x[i]);
// 	}
//     }
//     
//     
//     // FILL TEMP SURFACE VALUE:
//     for (i=0, j=0; j<fgNspline[FRTIME]*fgNspline[LEVEL]; i+=fgNspline[LAT], j++) {
// 	splint(pZ, &fgVal[i], &fgD2x[i], fgNspline[LAT], zz, fgTmpSrfVal[j]);
//     }
// 	
//     
//     // FILL TEMP_SRF_D2X:
//     for (i=0; i<fgNspline[FRTIME]; i+=fgNspline[LEVEL]) {
// 	spline(pY, &fgTmpSrfVal[i], fgNspline[LEVEL], 1.e30, 1.e30, &fgTmpSrfD2x[i]);
//     }
//     
//     
//     // FILL TEMP LINE VALUE:
//     for (i=0, j=0; j<fgNspline[FRTIME]; i+=fgNspline[LEVEL], j++) {
// 	splint(pY, &fgTmpSrfVal[i], &fgTmpSrfD2x[i], fgNspline[LEVEL], yy, 
// 	       fgTmpLineVal[j]);
//     }
//     
//     // FILL TEMP_LINE_D2X:
//     spline(pX, fgTmpLineVal, fgNspline[FRTIME], 1.e30, 1.e30, fgTmpLineD2x);
//     
//     // SPLINE VALUE:
//     splint(pX, fgTmpLineVal, fgTmpLineD2x, fgNspline[FRTIME], xx, vsplinep);

//    return vsplinep;
}

////////////////////////////////////////////////////////////////////////
// 4D INTERPOLATION:
// Does NOT warn on extrapolation
////////////////////////////////////////////////////////////////////////
float Grid::vspline(float xx, float yy, float zz, float tt) {

    static int	    splineFlag = 0;
    int		    fgResetSpline = 1;
    int		    fgNspline[5];
    float	    *fgD2x, *fgTmpLineD2x, *fgTmpSrfD2x, *fgTmpVolD2x;
    float	    *fgVal, *fgTmpLineVal, *fgTmpSrfVal, *fgTmpVolVal;
    int             nxStart, nyStart, nzStart, ntStart;
    int             nxLastStart, nyLastStart, nzLastStart, ntLastStart;
    float           *pX, *pY, *pZ, *pT;
    float           vsplinep;

    // INIT NSPLINE:
    fgNspline[VAR] = 0;
    for (int i=1; i<=4; i++) {
	if ( fgData[i].size > 6 ) {
	    fgNspline[i] = 6;
	}
	else {
	    fgNspline[i] = fgData[i].size;
	}
    }

#ifndef FGNOCHECK
    if ( fgNdims != 4 ) {
	fgErrorStrm << "vspline(float,float,float,float): Expects a 4D object." << std::endl;
	fgErrorStrm << "               object \""
	            << fgData[VAR].name << "\" is " << fgNdims << "D.\n";
	fg_error();
    }
#endif
    
    static int i, j, k, l, count;
    
    // GET START POSITION:
    nxStart = fg_get_nstart(fgData[FRTIME].val, fgData[FRTIME].size, fgNspline[FRTIME], xx);
    nyStart = fg_get_nstart(fgData[LEVEL].val, fgData[LEVEL].size, fgNspline[LEVEL], yy);
    nzStart = fg_get_nstart(fgData[LAT].val, fgData[LAT].size, fgNspline[LAT], zz);
    ntStart = fg_get_nstart(fgData[LON].val, fgData[LON].size, fgNspline[LON], tt);

    // EXECUTE THIS ONLY ONCE TO ALLOCATE:
    if ( splineFlag == 0 ) 
    {
      fgD2x = new float[fgData[VAR].size];
      fgVal = new float[fgData[VAR].size];
      fgTmpLineVal = new float[fgData[FRTIME].size];
      fgTmpLineD2x = new float[fgData[FRTIME].size];
      fgTmpSrfVal = new float[fgData[FRTIME].size*fgData[LEVEL].size];
      fgTmpSrfD2x = new float[fgData[FRTIME].size*fgData[LEVEL].size];
      fgTmpVolVal = new float[fgData[FRTIME].size*fgData[LEVEL].size*fgData[LAT].size];
      fgTmpVolD2x = new float[fgData[FRTIME].size*fgData[LEVEL].size*fgData[LAT].size];

      if ( !fgD2x         || !fgVal       || !fgTmpLineVal ||
	   ! fgTmpLineD2x || !fgTmpSrfVal ||  !fgTmpSrfD2x ||
	   ! fgTmpVolVal ||!fgTmpVolD2x ) 
      {
        fgErrorStrm << "fg_vspline(float,float,float,float): new failed\n";
	fg_error();
      }
	
	nxLastStart = nxStart;
	nyLastStart = nyStart;
	nzLastStart = nzStart;
	ntLastStart = ntStart;
	
	pT = fgData[LON].val + ntStart;
	pZ = fgData[LAT].val + nzStart;
	pY = fgData[LEVEL].val + nyStart;
	pX = fgData[FRTIME].val + nxStart;

	splineFlag = 1;
    }
    
    // CHECK IF NEED TO UPDATE POINTER POSITIONS:
    if (nxLastStart != nxStart) {
	nxLastStart = nxStart;
	pX = fgData[FRTIME].val + nxStart;
	fgResetSpline = 1;
    }
    
    if (nyLastStart != nyStart) {
	nyLastStart = nyStart;
	pY = fgData[LEVEL].val + nyStart;
	fgResetSpline = 1;
    }
    
    if (nzLastStart != nzStart) {
	nzLastStart = nzStart;
	pZ = fgData[LAT].val + nzStart;
	fgResetSpline = 1;
    }
    
    if (ntLastStart != ntStart) {
	ntLastStart = ntStart;
	pT = fgData[LON].val + ntStart;
	fgResetSpline = 1;
    }
    
    // EXECUTE THIS ONCE TO STORE SECOND DERIVATTIVES 
    // UNLESS VALUES CHANGE
    if ( fgResetSpline == 1 ) 
    {
      fgResetSpline = 0;
	
      count = 0;
      for (i=nxStart; i<nxStart+fgNspline[FRTIME]; i++) {
        for (j=nyStart; j<nyStart+fgNspline[LEVEL]; j++) {
          for (k=nzStart; k<nzStart+fgNspline[LAT]; k++) {
            for (l=ntStart; l<ntStart+fgNspline[LON]; l++) {
              fgVal[count] = fgData[VAR].val[offset(i, j, k, l)];
              count++;
	    }}}}
	
	// Second derivative:
	for (i=0; i<fgNspline[FRTIME]*fgNspline[LEVEL]*fgNspline[LAT]; i+=fgNspline[LON]) {
	    spline(pT, &fgVal[i], fgNspline[LON], 1.e30, 1.e30, &fgD2x[i]);
	}
    }

    // FILL TEMP VOL VALUE:
    for (i=0, j=0; j<fgNspline[FRTIME]*fgNspline[LEVEL]*fgNspline[LAT]; i+=fgNspline[LON], j++) {
	splint(pT, &fgVal[i], &fgD2x[i], fgNspline[LON], tt, fgTmpVolVal[j]);
    }

    // FILL TEMP_VOL_D2X:
    for (i=0; i<fgNspline[FRTIME]*fgNspline[LEVEL]; i+=fgNspline[LAT]) {
	spline(pZ, &fgTmpVolVal[i], fgNspline[LAT], 1.e30, 1.e30, &fgTmpVolD2x[i]);
    }

    // FILL TEMP SURFACE VALUE:
    for (i=0, j=0; j<fgNspline[FRTIME]*fgNspline[LEVEL]; i+=fgNspline[LAT], j++) {
	splint(pZ, &fgTmpVolVal[i], &fgTmpVolD2x[i], fgNspline[LAT], zz, 
	       fgTmpSrfVal[j]);
    }
	
    // FILL TEMP_SRF_D2X:
    for (i=0; i<fgNspline[FRTIME]; i+=fgNspline[LEVEL]) {
	spline(pY, &fgTmpSrfVal[i], fgNspline[LEVEL], 1.e30, 1.e30, &fgTmpSrfD2x[i]);
    }
    
    // FILL TEMP LINE VALUE:
    for (i=0, j=0; j<fgNspline[FRTIME]; i+=fgNspline[LEVEL], j++) {
	splint(pY, &fgTmpSrfVal[i], &fgTmpSrfD2x[i], fgNspline[LEVEL], yy, 
	       fgTmpLineVal[j]);
    }
    
    // FILL TEMP_LINE_D2X:
    spline(pX, fgTmpLineVal, fgNspline[FRTIME], 1.e30, 1.e30, fgTmpLineD2x);
    
    // SPLINE VALUE:
    splint(pX, fgTmpLineVal, fgTmpLineD2x, fgNspline[FRTIME], xx, vsplinep);
    
    return vsplinep;
}

////////////////////////////////////////////////////////////////////////
//
// NEAREST NEIGHBOR INTERPOLATION ROUTINES
//
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//
// LOCATE:
// return the index j from an ordered table xx[n] such that x lies between
// xx[j] and xx[j+1]. This is from Numerical Recipes and modified for
// zero-index arrays xx[0] to xx[n-1]. A return value of j=-1 lies below 
// xx[0] and a return value of j=n lies above xx[n-1].
//
////////////////////////////////////////////////////////////////////////
void fg_locate(float *xx, int n, float x, int &j) {

    int ju, jm, jl;
    int ascnd;

    jl = -1;
    ju = n+1;

    ascnd = ( xx[n-1] > xx[0] );

    while ( ju-jl > 1 ) {
	jm = (ju+jl) >> 1;
	if ( x > xx[jm] == ascnd ) {
	    jl = jm;
	}
	else {
	    ju = jm;
	}
    }
    j = jl;

    if ( !ascnd && j == n && x == xx[n-1] ) {
	j = n-2;
    }

    if ( j < 0 && x == xx[0] ) {
	j = 0;
    }

    return;

}
////////////////////////////////////////////////////////////////////////
// 1D NEAREST NEIGHBOR INTERPOLATION:
// RETURNS FILL ON EXTRAPOLATION
////////////////////////////////////////////////////////////////////////
float Grid::nnint(float xx) {

  std::cerr << "A deprecated function was called" << std::endl;
  exit(1);
// #ifndef FGNOCHECK
//     if ( fgNdims != 1 ) {
// 	fgErrorStrm << "nnint(float): Expects a 1D object." << endl;
// 	fgErrorStrm << "              object \""
// 	            << fgData[VAR].name << "\" is " << fgNdims << "D." << endl;
// 	fg_error();
//     }
// #endif
// 
//     static float xwhi, xwlo, pt;
// 
//     static int ilo, ihi;
// 
//     fg_locate(fgData[FRTIME].val, fgData[FRTIME].size, xx, ilo);
//     /****
//     if ( ilo < 0 || ilo >= fgData[FRTIME].size-1 ) {
// 	return fgFillValue;
//     }
//     ***/
//     if ( ilo < 0 ) {
// 	ilo = 0;
// 	xx = fgData[FRTIME].val[ilo];
//     }
//     if ( ilo >= int(fgData[FRTIME].size-1) ) {
// 	ilo = fgData[FRTIME].size-2;
// 	xx = fgData[FRTIME].val[ihi];
//     }
// 
// 
//     ihi = ilo+1;
// 
//     xwhi = (xx-fgData[FRTIME].val[ilo])/(fgData[FRTIME].val[ihi]-fgData[FRTIME].val[ilo]);
//     if ( xwhi < 0 ) xwhi = -xwhi;
//     xwlo = 1.0 - xwhi;
// 
//     pt = xwlo*fgData[VAR].val[ilo] + xwhi*fgData[VAR].val[ihi];
// 
//     return pt;
  return 0;
}



////////////////////////////////////////////////////////////////////////
// 2D NEAREST NEIGHBOR INTERPOLATION:
// RETURNS FILL ON EXTRAPOLATION
////////////////////////////////////////////////////////////////////////
float Grid::nnint(float xx, float yy) {

// #ifndef FGNOCHECK
//     if ( fgNdims != 2 ) {
// 	fgErrorStrm << "nnint(float,float): Expects a 2D object." << endl;
// 	fgErrorStrm << "                    object \""
// 	            << fgData[VAR].name << "\" is " << fgNdims << "D." << endl;
// 	fg_error();
//     }
// #endif
// 
    static float xwhi, xwlo, ywhi, ywlo, pt_yhi, pt_ylo, pt;
    static int ilo, ihi, jlo, jhi;

    fg_locate(fgData[FRTIME].val, fgData[FRTIME].size, xx, ilo);
    /****
    if ( ilo < 0 || ilo >= fgData[FRTIME].size-1 ) {
	return fgFillValue;
    }
    ***/
    if ( ilo < 0 ) {
	ilo = 0;
	xx = fgData[FRTIME].val[ilo];
    }
    if ( ilo >= int(fgData[FRTIME].size-1) ) {
	ilo = fgData[FRTIME].size-2;
	xx = fgData[FRTIME].val[ihi];
    }

    fg_locate(fgData[LEVEL].val, fgData[LEVEL].size, yy, jlo);
    /****
    if ( jlo < 0 || jlo >= fgData[LEVEL].size-1 ) {
	return fgFillValue;
    }
    ***/
    if ( jlo < 0 ) {
	jlo = 0;
	yy = fgData[LEVEL].val[jlo];
    }
    if ( jlo >= int(fgData[LEVEL].size-1) ) {
	jlo = fgData[LEVEL].size-2;
	yy = fgData[LEVEL].val[jhi];
    }

    ihi = ilo+1;
    jhi = jlo+1;

    xwhi = (xx-fgData[FRTIME].val[ilo])/(fgData[FRTIME].val[ihi]-fgData[FRTIME].val[ilo]);
    if ( xwhi < 0 ) xwhi = -xwhi;
    xwlo = 1.0 - xwhi;

    ywhi = (yy-fgData[LEVEL].val[jlo])/(fgData[LEVEL].val[jhi]-fgData[LEVEL].val[jlo]);
    if ( ywhi < 0 ) ywhi = -ywhi;
    ywlo = 1.0 - ywhi;

    pt_ylo = xwlo*fgData[VAR].val[offset(ilo, jlo)]
	   + xwhi*fgData[VAR].val[offset(ihi, jlo)];

    pt_yhi = xwlo*fgData[VAR].val[offset(ilo, jhi)]
	   + xwhi*fgData[VAR].val[offset(ihi, jhi)];

    pt = ywlo*pt_ylo + ywhi*pt_yhi;

	
    return pt;
  return 0;
}

////////////////////////////////////////////////////////////////////////
// 3D NEAREST NEIGHBOR INTERPOLATION:
// RETURNS FILL ON EXTRAPOLATION
////////////////////////////////////////////////////////////////////////
float Grid::nnint(float xx, float yy, float zz) {

// #ifndef FGNOCHECK
//     if ( fgNdims != 3 ) {
// 	fgErrorStrm << "nnint(float,float,float): Expects a 3D object." << endl;
// 	fgErrorStrm << "                          object \""
// 	            << fgData[VAR].name << "\" is " << fgNdims << "D." << endl;
// 	fg_error();
//     }
// #endif

    static float xwhi, xwlo, ywhi, ywlo, zwhi, zwlo, 
                 pt_yhi_zhi, 
                 pt_ylo_zhi, 
                 pt_yhi_zlo, 
                 pt_ylo_zlo, 
                 pt_zhi, 
                 pt_zlo, 
                 pt;
    static int ilo, ihi, jlo, jhi, klo, khi;

    fg_locate(fgData[FRTIME].val, fgData[FRTIME].size, xx, ilo);
    /****
    if ( ilo < 0 || ilo >= fgData[FRTIME].size-1 ) {
	return fgFillValue;
    }
    ***/
    if ( ilo < 0 ) {
	ilo = 0;
	xx = fgData[FRTIME].val[ilo];
    }
    if ( ilo >= int(fgData[FRTIME].size-1) ) {
	ilo = fgData[FRTIME].size-2;
	xx = fgData[FRTIME].val[ihi];
    }

    fg_locate(fgData[LEVEL].val, fgData[LEVEL].size, yy, jlo);
    /****
    if ( jlo < 0 || jlo >= fgData[LEVEL].size-1 ) {
	return fgFillValue;
    }
    ***/
    if ( jlo < 0 ) {
	jlo = 0;
	yy = fgData[LEVEL].val[jlo];
    }
    if ( jlo >= int(fgData[LEVEL].size-1) ) {
	jlo = fgData[LEVEL].size-2;
	yy = fgData[LEVEL].val[jhi];
    }

    fg_locate(fgData[LAT].val, fgData[LAT].size, zz, klo);
    /****
    if ( klo < 0 || klo >= fgData[LAT].size-1 ) {
	return fgFillValue;
    }
    ***/
    if ( klo < 0 ) {
	klo = 0;
	zz = fgData[LAT].val[klo];
    }
    if ( klo >= int(fgData[LAT].size-1) ) {
	klo = fgData[LAT].size-2;
	zz = fgData[LAT].val[khi];
    }



    ihi = ilo+1;
    jhi = jlo+1;
    khi = klo+1;

    xwhi = (xx-fgData[FRTIME].val[ilo])/(fgData[FRTIME].val[ihi]-fgData[FRTIME].val[ilo]);
    if ( xwhi < 0 ) xwhi = -xwhi;
    xwlo = 1.0 - xwhi;

    ywhi = (yy-fgData[LEVEL].val[jlo])/(fgData[LEVEL].val[jhi]-fgData[LEVEL].val[jlo]);
    if ( ywhi < 0 ) ywhi = -ywhi;
    ywlo = 1.0 - ywhi;

    zwhi = (zz-fgData[LAT].val[klo])/(fgData[LAT].val[khi]-fgData[LAT].val[klo]);
    if ( zwhi < 0 ) zwhi = -zwhi;
    zwlo = 1.0 - zwhi;

    pt_ylo_zhi = xwlo*fgData[VAR].val[offset(ilo, jlo, khi)]
	       + xwhi*fgData[VAR].val[offset(ihi, jlo, khi)];

    pt_yhi_zhi = xwlo*fgData[VAR].val[offset(ilo, jhi, khi)]
	       + xwhi*fgData[VAR].val[offset(ihi, jhi, khi)];

    pt_ylo_zlo = xwlo*fgData[VAR].val[offset(ilo, jlo, klo)]
	       + xwhi*fgData[VAR].val[offset(ihi, jlo, klo)];

    pt_yhi_zlo = xwlo*fgData[VAR].val[offset(ilo, jhi, klo)]
	       + xwhi*fgData[VAR].val[offset(ihi, jhi, klo)];

    pt_zhi = ywlo*pt_ylo_zhi + ywhi*pt_yhi_zhi;

    pt_zlo = ywlo*pt_ylo_zlo + ywhi*pt_yhi_zlo;
    
    pt = zwlo*pt_zlo + zwhi*pt_zhi;

   return pt;

}

////////////////////////////////////////////////////////////////////////
// 4D NEAREST NEIGHBOR INTERPOLATION:
// DOES NOT WARN ON EXTRAPOLATION!
////////////////////////////////////////////////////////////////////////
float Grid::nnint(float xx, float yy, float zz, float tt) {
  // don't let this variable names fool you, they could be anything
//  static int counter = 0;

    if ( fgNdims != 4 ) {
	fgErrorStrm << "nnint(float,float,float,float): Expects a 4D object." << std::endl;
	fgErrorStrm << "                                object \""
	            << fgData[VAR].name << "\" is " << fgNdims << "D." << std::endl;
	fg_error();
    }

    static float xwhi, xwlo, ywhi, ywlo, zwhi, zwlo, twhi, twlo,
                 pt_yhi_zhi_thi, 
                 pt_ylo_zhi_thi, 
                 pt_yhi_zlo_thi, 
                 pt_ylo_zlo_thi,
                 pt_yhi_zhi_tlo, 
                 pt_ylo_zhi_tlo, 
                 pt_yhi_zlo_tlo, 
                 pt_ylo_zlo_tlo,
                 pt_zlo_thi,
                 pt_zhi_thi,
                 pt_zlo_tlo,
                 pt_zhi_tlo,
                 pt_thi,
                 pt_tlo,
                 pt;

    static int ilo, ihi, jlo, jhi, klo, khi, llo, lhi;

    fg_locate(fgData[FRTIME].val, fgData[FRTIME].size, xx, ilo);
    /****
    if ( ilo < 0 || ilo >= fgData[FRTIME].size-1 ) {
	return fgFillValue;
    }
    ***/
    if ( ilo < 0 ) {
	ilo = 0;
	xx = fgData[FRTIME].val[ilo];
    } else if ( ilo >= int(fgData[FRTIME].size-1) ) {
	ilo = fgData[FRTIME].size-2;
	xx = fgData[FRTIME].val[ilo+1];
    }


    fg_locate(fgData[LEVEL].val, fgData[LEVEL].size, yy, jlo);
    /****
    if ( jlo < 0 || jlo >= fgData[LEVEL].size-1 ) {
	return fgFillValue;
    }
    ***/
    if ( jlo < 0 ) {
	jlo = 0;
	yy = fgData[LEVEL].val[jlo];
    }
    if ( jlo >= int(fgData[LEVEL].size-1) ) {
	jlo = fgData[LEVEL].size-2;
	yy = fgData[LEVEL].val[jhi];
    }

    fg_locate(fgData[LAT].val, fgData[LAT].size, zz, klo);
    /****
    if ( klo < 0 || klo >= fgData[LAT].size-1 ) {
	return fgFillValue;
    }
    ***/
    if ( klo < 0 ) {
	klo = 0;
	zz = fgData[LAT].val[klo];
    }
    if ( klo >= int(fgData[LAT].size-1) ) {
	klo = fgData[LAT].size-2;
	zz = fgData[LAT].val[khi];
    }


    fg_locate(fgData[LON].val, fgData[LON].size, tt, llo);
    /****
    if ( llo < 0 || llo >= fgData[LON].size-1 ) {
	return fgFillValue;
    }
    ***/
    if ( llo < 0 ) {
	llo = 0;
	tt = fgData[LON].val[llo];
    }
    if ( llo >= int(fgData[LON].size-1) ) {
	llo = fgData[LON].size-2;
	tt = fgData[LON].val[lhi];
    }

    if (fgData[FRTIME].size > (ilo + 1) ) {
      ihi = ilo+1;
    } else { return nnint(xx, yy, zz); }
    jhi = jlo+1;
    khi = klo+1;
    lhi = llo+1;

    xwhi = (xx-fgData[FRTIME].val[ilo])/(fgData[FRTIME].val[ihi]-fgData[FRTIME].val[ilo]);
    if ( xwhi < 0 ) xwhi = -xwhi;
    xwlo = 1.0 - xwhi;

    ywhi = (yy-fgData[LEVEL].val[jlo])/(fgData[LEVEL].val[jhi]-fgData[LEVEL].val[jlo]);
    if ( ywhi < 0 ) ywhi = -ywhi;
    ywlo = 1.0 - ywhi;

    zwhi = (zz-fgData[LAT].val[klo])/(fgData[LAT].val[khi]-fgData[LAT].val[klo]);
    if ( zwhi < 0 ) zwhi = -zwhi;
    zwlo = 1.0 - zwhi;

    twhi = (tt-fgData[LON].val[llo])/(fgData[LON].val[lhi]-fgData[LON].val[llo]);
    if ( twhi < 0 ) twhi = -twhi;
    twlo = 1.0 - twhi;

    pt_ylo_zhi_thi = xwlo*fgData[VAR].val[offset(ilo, jlo, khi, lhi)]
	           + xwhi*fgData[VAR].val[offset(ihi, jlo, khi, lhi)];

    pt_yhi_zhi_thi = xwlo*fgData[VAR].val[offset(ilo, jhi, khi, lhi)]
	           + xwhi*fgData[VAR].val[offset(ihi, jhi, khi, lhi)];

    pt_ylo_zlo_thi = xwlo*fgData[VAR].val[offset(ilo, jlo, klo, lhi)]
	           + xwhi*fgData[VAR].val[offset(ihi, jlo, klo, lhi)];

    pt_yhi_zlo_thi = xwlo*fgData[VAR].val[offset(ilo, jhi, klo, lhi)]
	           + xwhi*fgData[VAR].val[offset(ihi, jhi, klo, lhi)];

    pt_ylo_zhi_tlo = xwlo*fgData[VAR].val[offset(ilo, jlo, khi, llo)]
	           + xwhi*fgData[VAR].val[offset(ihi, jlo, khi, llo)];

    pt_yhi_zhi_tlo = xwlo*fgData[VAR].val[offset(ilo, jhi, khi, llo)]
	           + xwhi*fgData[VAR].val[offset(ihi, jhi, khi, llo)];

    pt_ylo_zlo_tlo = xwlo*fgData[VAR].val[offset(ilo, jlo, klo, llo)]
	           + xwhi*fgData[VAR].val[offset(ihi, jlo, klo, llo)];

    pt_yhi_zlo_tlo = xwlo*fgData[VAR].val[offset(ilo, jhi, klo, llo)]
	           + xwhi*fgData[VAR].val[offset(ihi, jhi, klo, llo)];

    pt_zhi_thi = ywlo*pt_ylo_zhi_thi + ywhi*pt_yhi_zhi_thi;

    pt_zlo_thi = ywlo*pt_ylo_zlo_thi + ywhi*pt_yhi_zlo_thi;
    
    pt_zhi_tlo = ywlo*pt_ylo_zhi_tlo + ywhi*pt_yhi_zhi_tlo;

    pt_zlo_tlo = ywlo*pt_ylo_zlo_tlo + ywhi*pt_yhi_zlo_tlo;

    pt_thi = zwlo*pt_zlo_thi + zwhi*pt_zhi_thi;
    
    pt_tlo = zwlo*pt_zlo_tlo + zwhi*pt_zhi_tlo;
    
    pt = twlo*pt_tlo + twhi*pt_thi;

//    counter++;
    return pt;
}

