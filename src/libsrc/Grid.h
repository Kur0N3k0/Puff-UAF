#ifndef PUFF_GRID_H_
#define PUFF_GRID_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctime>
#include <iostream>
#include <fstream>

#ifdef HAVE_SSTREAM
#include <sstream>
#elif HAVE_STRSTREAM
#include <strstream>
#define ostringstream ostrstream
#else 
#error Do not have <sstream> or <strstream>
#endif

#include <cstdlib>
#include <string>

#ifdef HAVE_NETCDFCPP_H
#include <netcdfcpp.h>
#else
#include "netcdfcpp.h"
#endif

#ifdef HAVE_CMAPF_H
#include "cmapf.h"
#else
#include "dmapf-c/cmapf.h"
#endif

extern const int FG_ERROR;
extern const int FG_OK;

#define FGMAXCHAR 65

#ifdef BUG_GMTIME
#define gmtime localtime
#endif

enum ID { VAR=0, FRTIME=1, LEVEL=2, LAT=3, LON=4 };

enum DISPLAY_ENUM { SIMPLE, FULL, COLUMN, SIMPLE_NOSTATS, FULL_NOSTATS, 
		    COLUMN_NOSTATS, QUICK, QUICK_STATS, INFO };
enum IO_ENUM { USEEXTENSION, FLTGRID, BINARY, FLTARRAY, ASCIITEXT, CDF };

enum INTERP_ENUM { FG_VSPLINE, FG_NNINT };

struct fgDataStruct {
    float              *val;
    unsigned int       size;
    char               name[FGMAXCHAR];
    char               units[FGMAXCHAR];
    float              range[2];
};

struct GridRotation {
	float lat;
  float lon;
	float angle;
	};

class Grid { 
protected:
    // Basic Variables:
    fgDataStruct    fgData[5];
    unsigned int    fgNdims;
    float	    fgFillValue;
    char	    fgTitle[FGMAXCHAR];
    char	    fgReftime[FGMAXCHAR];
    double          add_offset, scale_factor;

    int uniShiftWest;  // imported from uniGrid.h
    
    // Display Variables:
    int		    fgWidth;
    int		    fgPrecision;
    int		    fgLeftAdjust;
    char	    fgSpace[7];
    enum {UNKNOWN, REGIONAL, GLOBAL} coverage;

    
public:
    ~Grid();
    
    Grid();
    Grid(unsigned int nx1, unsigned int nx2, unsigned int nx3, unsigned int nx4);
    
    void create(unsigned int nx1, unsigned int nx2, unsigned int nx3, 
		unsigned int nx4);
    
    // READ/WRITE:
    int read(char *file, const char* eDate = "", const double runHours = -1);
    int read(std::string* s);
    int write(char *file);
    int write(std::string *file);
    int append(std::string *file);

    // DISPLAY:
    void display(DISPLAY_ENUM disStyle=SIMPLE);
    void display_info();

    void width(int iw=-1) { fgWidth = iw; }
    void precision(int ip=-1) { fgPrecision = ip; }
    void leftadjust() { fgLeftAdjust = 1; }
    void rightadjust() { fgLeftAdjust = 0; }
    void spacing(char *s);
    
    // BASIC STATS:
    float pct_bad();
    float pct_fill();
    float min(ID idx = VAR);
    float max(ID idx = VAR);
    float mean();
    float variance();
    
    fgDataStruct & operator[](ID dimid) { return fgData[dimid]; }
    bool empty() { return (fgData[VAR].size == 0); }
    int ndims() { return fgNdims; }
    int n(ID dimid=VAR) { return fgData[dimid].size; }
    char *name(ID index=VAR) {return fgData[index].name;}
    char *title() {return fgTitle;}
    char *reftime() {return fgReftime;}
    char *units(ID index=VAR) {return fgData[index].units;}
    float fill_value() {return fgFillValue;}
    float valid_range(unsigned i);
    float valid_range(ID dimid, unsigned i);
    bool isGlobal();
    bool isProjectionGrid();
    
    // OPERATOR():
    float & operator()(unsigned int i) { 
        ck_ndims(1); ck_index(FRTIME, i);
	return fgData[VAR].val[i];
    }
    float & operator()(unsigned int i, unsigned int j) {
	    ck_ndims(2); ck_index(FRTIME, i); ck_index(LEVEL, j);
	return fgData[VAR].val[offset(i, j)];
    }
    float & operator()(unsigned int i, unsigned int j, unsigned int k) {
	    ck_ndims(3); ck_index(FRTIME, i); ck_index(LEVEL, j); ck_index(LAT, k);
	return fgData[VAR].val[offset(i, j, k)];
    }
     float & operator()(unsigned int i, unsigned int j, unsigned int k, 
 		       unsigned int l) {
	    ck_ndims(4); ck_index(FRTIME, i); ck_index(LEVEL, j); 
	    ck_index(LAT, k); ck_index(LON, l);
 	return fgData[VAR].val[offset(i, j, k, l)];
     }
    float & operator()(ID dimid, unsigned int i) {
	    ck_index(dimid, i);
	return fgData[dimid].val[i];
    }
    
    // OFFSET:
    inline unsigned int offset(unsigned int i) { return i; }
    inline unsigned int offset(unsigned int i, unsigned int j) { 
	return i*fgData[LEVEL].size + j;
    }
    inline unsigned int offset(unsigned int i, unsigned int j, unsigned int k) { 
	return i*fgData[LEVEL].size*fgData[LAT].size + j*fgData[LAT].size + k;
    }
    inline unsigned int offset(unsigned int i, unsigned int j, unsigned int k, 
                               unsigned int l) { 
	return i*fgData[LEVEL].size*fgData[LAT].size*fgData[LON].size +
	       j*fgData[LAT].size*fgData[LON].size + k*fgData[LON].size + l;
    }
    
    
    // DIMENSION SETS:
    int dim_norm(ID dimid, float fmax=1.0);
    int dim_step(ID dimid, float start, float step);
//    int dim_expand(ID, float start, float end);
	
    // SET VALUES:
    void set_coverage();
    void set_name(const char *s);
    void set_name(ID index, const char *s);
    void set_units(const char *s);
    void set_units(ID index, const char *s);
    void set_title(const char *s);
    void set_reftime(const char *s);
    void set_valid_range(float rlo, float rhi);
    void set_range(ID index, float rlo, float rhi);
    void set_fill_value(float fill) { fgFillValue = fill; }
    
    // SPLINE ROUTINES:
    void patch(ID line1=FRTIME, ID line2=LEVEL, ID line3=LAT, ID line4=LON);
    void patch_line(ID lineid);
    
    float vspline(float xx);
    float vspline(float xx, float yy);
    float vspline(float xx, float yy, float zz);
    float vspline(float xx, float yy, float zz, float tt);

    float nnint(float xx);
    float nnint(float xx, float yy);
    float nnint(float xx, float yy, float zz);
    float nnint(float xx, float yy, float zz, float tt);

    // SNAP TO NEAREST GRID:
    void snap(float x, int &i);
    void snap(float x, float y, int &i, int &j);
    void snap(float x, float y, float z, int &i, int &j, int &k);
    void snap(float x, float y, float z, float t, 
	      int &i, int &j, int &k, int &l);

    int read_cdf(const std::string *file, const char* eruptDate, const double runHours);

  // imported from uniGrid.h
    void set_shift_west(int i=1) { uniShiftWest = i; }
    int PtoH(Grid &H, int dz, int &iwarn);
    int PtoH(float P0=1000.0, float Hconst=7400.0, float round=100.0);
    int uni_shift_west();
    void pressureGridFromZ(Grid &Z);
    void TstandardAtm();

//    int uni_sort_frtime();
  // done imported from uniGrid.h
  
protected:
    void allocate(unsigned int nx1=0, unsigned int nx2=0, 
		  unsigned int nx3=0, unsigned int nx4=0);
    void initialize();
    
        inline void ck_ndims(unsigned int n) { 
	    if (fgNdims != n ) fg_error_ndims();
	}
        inline void ck_index(ID dimid, unsigned int i) {
	    if (i >= fgData[dimid].size) fg_error_index(dimid); 
	}
    
    // SPLINE ROUTINES:
    void spline(float *x, float *y, int n, float yp1, float ypn, float *y2);
    void splint(float *xa, float *ya, float *y2a, int n, float x, float & y);


    
private:
    std::ostringstream fgErrorStrm;
    void fg_error();
    void fg_error_ndims();
    void fg_error_index(ID dimid);

    std::ostringstream display_ostr(DISPLAY_ENUM);
    
    // READ/WRITE:
    int write_cdf(char *file);
    int read_pp(std::string *file);

		void adjust_dimensions(long int *offsets, int dim_idx);
		void make_monotonic();
    void locate(float *xx, int n, float x, int &j);
    void snap_line(float *xx, int n, float x, int &j);
    void get_attributes(NcVar *vp, int idx);
    void get_global_attributes(NcFile *vp);
    char *reftimeFromNcVar(NcVar *vp);
    void reftimeFromBasedate(int year, int month, int day);
    char *pp_reftime(std::ifstream *file);

  // imported from uniGrid.h
    std::ostringstream uniErrorStrm;
    void uni_error();
    void shellsort(float *arrPtr, int arrSize);
    int get_reftime(int cdfid);
    
}; // END FLTGRID CLASS


// UTILITY FUNCTIONS:
char *time2unistr(time_t time);
int init_grid(char* filename, maparam *proj_grid);
int init_grid(std::string filename, maparam *proj_grid);

  // imported from uniGrid.h
time_t unistr2time(const char *unistr);
//char *time2unistr(time_t time);
int unistr2jul(char *unistr);
char *unistr2jstr(char *unistr);
time_t unifilestr2time(char *unistr);
char *time2unifilestr(time_t time);
std::string correspondingFile(std::string *s, char *tok, char *repl);

extern "C" { void tzset(); }

#ifndef HAVE_FLOORF
#define floorf(x) floor((float)x)
#endif

#ifndef HAVE_CEILF
#define ceilf(x) ceil((float)x)
#endif

#endif
