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
#include "Grid.h"

int patch_offset(int nx2, int i, int j);
int patch_offset(int nx2, int nx3, int i, int j, int k);
int patch_offset(int nx2, int nx3, int nx4, int i, int j, int k, int l);

// 2D OFFSET:
int patch_offset(int nx2, int i, int j) {
    return i*nx2 + j;
}
// 3D OFFSET:
int patch_offset(int nx2, int nx3, int i, int j, int k) {
    return i*nx2*nx3 + j*nx3 + k;
}
// 4D OFFSET:
int patch_offset(int nx2, int nx3, int nx4, int i, int j, int k, int l) {
    return i*nx2*nx3*nx4 + j*nx3*nx4 + k*nx4 + l;
}

///////////////////////////////////////////////////////////////////
// BAD DATA PATCH ROUTINE
///////////////////////////////////////////////////////////////////
void Grid::patch(ID line1, ID line2, ID line3, ID line4) {

    // return if nothing is bad
    if (this->pct_bad() == 0.0) return;
	
    patch_line(line1);
    if (this->pct_bad() == 0.0) return;
	
    if (fgNdims > 1) {
	patch_line(line2);
	if (this->pct_bad() == 0.0) return;
	    if (fgNdims > 2) {
		patch_line(line3);
		    if (this->pct_bad() == 0.0) return;
			if (fgNdims > 3) {
			    patch_line(line4);
				if (this->pct_bad() == 0.0) return;
			}
	    }
    }
			
	
    if (this->pct_bad() != 0.0) {
	std::cerr << "Grid WARNING: patch() Failed" << '\n';
    }
    return;
};

void Grid::patch_line(ID line_index) {
    float *dat, *dat_line, *pd2x, *xx;
    float temp_val;
    float temp_spline;
    int npts, nmax;
    int xscan, x1, x2, x3;	// Counters
    int pscan, px1, px2, px3;	// Indices
    int nscan, nx1, nx2, nx3;	// Sizes
	
    switch(line_index) {
	case (FRTIME) : {
	    pscan = int(FRTIME);
	    px1 = int(LEVEL);
	    px2 = int(LAT);
	    px3 = int(LON);
	    nscan = fgData[FRTIME].size;
	    nx1 = fgData[LEVEL].size;
	    nx2 = fgData[LAT].size;
	    nx3 = fgData[LON].size;
	    break;
	}
	case (LEVEL) : {
	    pscan = int(LEVEL);
	    px1 = int(FRTIME);
	    px2 = int(LAT);
	    px3 = int(LON);
	    nscan = fgData[LEVEL].size;
	    nx1 = fgData[FRTIME].size;
	    nx2 = fgData[LAT].size;
	    nx3 = fgData[LON].size;
	    break;
	}
	case (LAT) : {
	    pscan = int(LAT);
	    px1 = int(FRTIME);
	    px2 = int(LEVEL);
	    px3 = int(LON);
	    nscan = fgData[LAT].size;
	    nx1 = fgData[FRTIME].size;
	    nx2 = fgData[LEVEL].size;
	    nx3 = fgData[LON].size;
	    break;
	}
	case (LON) : {
	    pscan = int(LON);
	    px1 = int(FRTIME);
	    px2 = int(LEVEL);
	    px3 = int(LAT);
	    nscan = fgData[LON].size;
	    nx1 = fgData[FRTIME].size;
	    nx2 = fgData[LEVEL].size;
	    nx3 = fgData[LAT].size;
	    break;
	}
	default : {
	    fgErrorStrm << "patch_line(): Bad line index." << std::endl;
	    fg_error();
	} 
    }
	
    // set pointers:
    nmax = fgData[pscan].size;
    xx = fgData[pscan].val;
	
    // temporaries:
    dat = new float[nmax];
    if (!dat) {
	fgErrorStrm << "patch_line(): new failed." << std::endl;
	fg_error();
    }

    dat_line = new float[nmax];
    if (!dat_line) {
    	fgErrorStrm << "patch_line(): new failed." << std::endl;
	fg_error();
    }

    pd2x = new float[nmax];
    if (!pd2x) {
	fgErrorStrm << "patch_line(): new failed." << std::endl;
	fg_error();
    }
	
    int scan_start, scan_end, scan_p;
    if (xx[1] > xx[0]) {		// Ascending data set
	scan_start = 0;
	scan_end = nmax;
	scan_p = 1;
    }
    else {				// Descending data set
	scan_start = nmax-1;
	scan_end = -1;
	scan_p = -1;
    }
	
    // Begin scan along the indices:
    int offset;
    x1=0;
    do { x2=0; do { x3=0; do {
	
	npts = 0;
	for (xscan=scan_start; xscan!=scan_end; xscan=xscan+scan_p) {
	    switch(line_index) { 
		case (VAR) : { 
		  std::cerr << "ERROR: Can't patch along the VAR dimension!\n";
		  exit(0);
		  break; 
		  }
		case (FRTIME) : {
		    switch (fgNdims) {
		        case (1) : {
			    offset = xscan;
			    break;
			}
			case (2) : {
			    offset = patch_offset(nx1, xscan, x1);
			    break;
			}
			case (3) : {
			    offset = patch_offset(nx1, nx2, xscan, x1, x2);
			    break;
			}
		        case (4) : {
			    offset = patch_offset(nx1, nx2, nx3, xscan, 
						  x1, x2, x3);
			    break;
			}
			default : { break; }
		    }
		    break;
		}
		case (LEVEL) : {
		    switch (fgNdims) {
			case (2) : {
			    offset = patch_offset(nscan, x1, xscan);
			    break;
			}
			case (3) : {
			    offset = patch_offset(nscan, nx2, x1, xscan, x2);
			    break;
			}
		        case (4) : {
			    offset = patch_offset(nscan, nx2, nx3, x1, 
						  xscan, x2, x3);
			    break;
			}
			default : { break; }
		    }
		    break;
		}
		case (LAT) : {
		    switch (fgNdims) {
			case (3) : {
			    offset = patch_offset(nx2, nscan, x1, x2, xscan);
			    break;
			}
		        case (4) : {
			    offset = patch_offset(nx2, nscan, nx3, x1, x2, 
						  xscan, x3);
			    break;
			}
			default : { break; }
		    }
		    break;
		}
		case (LON) : {
		    switch (fgNdims) {
		        case (4) : {
			    offset = patch_offset(nx2, nx3, nscan, x1, x2, 
						  x3, xscan);
			    break;
			}
			default : { break; }
		    }
		    break;
		}
	    }
			
	    temp_val = fgData[VAR].val[offset];
	    
	    // GOOD POINTS:
	    if (temp_val >= fgData[VAR].range[0] &&
		temp_val <= fgData[VAR].range[1] &&
		temp_val != fgFillValue) {
		dat[npts] = temp_val;
		dat_line[npts] = xx[xscan];
		npts++;
	    }
	}
			
	if (npts > 1 && npts != nmax) {	
	    spline(dat_line, dat, npts, 1.e30, 1.e30, pd2x);
				
	    for (xscan=scan_start; xscan!=scan_end; xscan=xscan+scan_p) {
		switch(line_index) { 
		    case (VAR) : { break; }
		    case (FRTIME) : {
			switch (fgNdims) {
		            case (1) : {
				offset = xscan;
				break;
			    }
			    case (2) : {
				offset = patch_offset(nx1, xscan, x1);
				break;
			    }
			    case (3) : {
				offset = patch_offset(nx1, nx2, xscan, x1, x2);
				break;
			    }
		            case (4) : {
				offset = patch_offset(nx1, nx2, nx3, xscan, 
						      x1, x2, x3);
				break;
			    }
			    default : { break; }
			}
			break;
		    }
		    case (LEVEL) : {
		        switch (fgNdims) {
			    case (2) : {
				offset = patch_offset(nscan, x1, xscan);
				break;
			    }
			    case (3) : {
				offset = patch_offset(nscan, nx2, x1, xscan, x2);
				break;
			    }
		            case (4) : {
				offset = patch_offset(nscan, nx2, nx3, x1, 
						      xscan, x2, x3);
				break;
			    }
			    default : { break; }
		        }
			break;
		    }
		    case (LAT) : {
		        switch (fgNdims) {
			    case (3) : {
				offset = patch_offset(nx2, nscan, x1, x2, xscan);
				break;
			    }
		            case (4) : {
				offset = patch_offset(nx2, nscan, nx3, x1,
						      x2, xscan, x3);
				break;
			    }
			    default : { break; }
		        }
			break;
		    }
		    case (LON) : {
		        switch (fgNdims) {
		            case (4) : {
				offset = patch_offset(nx2, nx3, nscan, x1,
						      x2, x3, xscan);
				break;
			    }
			    default : { break; }
		        }
			break;
		    }
		}
		
		temp_val = fgData[VAR].val[offset];
		
		if (temp_val < fgData[VAR].range[0] ||
		    temp_val > fgData[VAR].range[1] ||
		    temp_val == fgFillValue) {
		    
		    splint(dat_line, dat, pd2x, npts, xx[xscan], temp_spline);
						
		    if (temp_spline < fgData[VAR].range[0] ||
			temp_spline > fgData[VAR].range[1]) {
			temp_spline = fgFillValue;
		    }
						
		    fgData[VAR].val[offset] = temp_spline;
		    
		} // END OF PATCHING
	    } // END OF SEARCH LOOP
	} // END OF BAD DATA FOUND STATEMENT
		
    x3++; } while (x3 < nx3);
    x2++; } while (x2 < nx2);
    x1++; } while (x1 < nx1);
	
    delete[] dat;
    delete[] dat_line;
    delete[] pd2x;
	
    return;
};

