#ifndef PUFF_DEM_H
#define PUFF_DEM_H

#define DEM_ERROR 1
#define DEM_OK 0

#ifdef HAVE_CMAPF_H
#include "cmapf.h"
#else
#include "libsrc/dmapf-c/cmapf.h"
#endif

class Dem {
  private:
    int no_data_value,
        ntiles,
	resolution_factor;
    // these two are used to help speed up DTED0's DEM
    unsigned int *tileArray;
    int numTilesRead;
    
    char *path;
    bool initialized;
    struct Tile {
      char *name;
      double maxLat, minLon;
      double dx, dy;
      int nrows, ncols;
      bool loaded, exists;
      double *data;
      } *tile;
    enum {GTOPO30, DTED0} type;
        
    int setGtopo30();
    int setDted0();
    int readGtopo30Header(int idx);
    int readDted0Header(int idx);
    int readTile(int idx);
    int tileNumber (double lat, double lon);
      
  public:
    Dem();
    ~Dem();
    
    int initialize(const char *type);
    int setPath(const char* inPath);
    int setResolution(int res);
    double elevation(double lat, double lon, maparam* proj_grid);
    };
    
#endif
