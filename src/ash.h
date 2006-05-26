#ifndef ASH_H_
#define ASH_H_

#include <string>
#include <vector> // std::vector<>
#include "particle.h"
#include "ran_utils.h"
#include "Grid.h"
#include "planes.h"

extern int iseed;
extern const double GravConst;
extern const int ASH_ERROR;
extern const int ASH_OK;

class Ash {
    long     ashN;
    long     ashNpart;
    long int numGrounded, numOutOfBounds;
    Particle *particle;
    std::vector<Particle> recParticle; // record of particles
    std::vector<long> recTime; // record of times
    long     recAshN;  // number of particles in the complete record
    long     clockTime;
    long     origTime;
    char     origName[120];
    double   minlat, minlon, minhgt, maxlat, maxlon, maxhgt;
    enum {ASH_SORT_YES, ASH_SORT_NO, ASH_SORT_NEVER} sorting_protocol;
		enum {ASH_SORT_T, ASH_SORT_X, ASH_SORT_Y, ASH_SORT_Z} sorting_variable;

//    float *abs_air_conc_avg, *rel_air_conc_avg, *abs_fo_conc_avg, *rel_fo_conc_avg;
   CCloud cc;
    
public:
#ifdef PUFF_STATISTICS
    double    *dif_x, *dif_y, *dif_z;
    double    *adv_x, *adv_y, *adv_z;
#endif
	  struct GridRotation rotGrid;
    Ash();
    Ash(long n);
    ~Ash();

    int create(long n);

    void write(const char *file);
    int read(char *file);
    
    void clearStash();
    void findLimits();
    void stashData(time_t now);
    int ground(long int idx);
    void horiz_spread(float width, float height, float bottom);
    void init_site(float lon, float lat, char *name);
    void init_site_custom(int multE=0, maparam* proj_grid=NULL);  
    void init_age(long erupt_time, long lengthSecs);
    int  init_size(double logMean, double logSdev, char* phiDist);
    void init_linear_column(float height, float bottom=0.0);
    void init_expon_column(float height, float width, float bottom);
    void init_poisson_column(float height, float bottom, float width);
    void initialize();
    int isAshFile(char *name);
    int outOfBounds(int outIdx);
    void quicksort();
    void setSortingProtocol(char *arg);
    void writeGriddedData(std::string eDate, bool last);

#ifdef PUFF_STATISTICS
    void clearStats();
#endif
    
    // RETURNS:
    int n() { return ashN; }
    
    // shortcut notation for particle location
    Particle *r;
    
    double    origLon;
    double    origLat;

    char *origname() { return origName; }
    float origlon() { return origLon; }
    float origlat() { return origLat; }
    long origtime() { return origTime; }
    
    long &clock() { return clockTime; }

    // finctions that return details about particle's attributes    
    double age(long i) const { return (clockTime - particle[i].startTime); }
    double start(long i) const { return particle[i].startTime; }
    double getSize(long i) const { return particle[i].size; }
    bool isGrounded(long i) const { return particle[i].grounded;}
    double fallVelocity(int idx) ;
    bool particleExists(long i) const  { return particle[i].exists; }
		void copyRotatedGrid(struct GridRotation *r);
		bool isRotatedGrid();

    // PUFF PARAMETERS FOR SAVING PURPOSES:
    float erupt_hours;
    float plume_height;
    float plume_min;
    float plume_width_z;
    float plume_width_h;
    char plume_shape[120];
    float diffuse_h;
    float diffuse_v;
    float log_mean;
    float log_sdev;
    char date_time[13]; // YYYYMMDDHHMM + ending newline
    
private:
    int allocate();
    void averageGriddedData(float *abs_air_conc,
                            float *rel_air_conc, 
			    float *abs_fo_conc,
                            float *rel_fo_conc);

	void rotateGrid(double *loc, float val, ID l);
	void rotateGridPoint(double *loc, float val, ID l);

};

bool cmpX(Particle a, Particle b);
bool cmpY(Particle a, Particle b);
bool cmpZ(Particle a, Particle b);

double dlat2meter(double dlat);
double dlon2meter(double dlon, double lon);

#endif


