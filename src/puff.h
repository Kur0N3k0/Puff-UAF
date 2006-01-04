#ifndef PUFF_H_
#define PUFF_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <iostream> /* cout */
#include <cmath>
#include <cstdlib> /* ecvt */

#include "Grid.h"
#include "ran_utils.h"
#include "ash.h"

// Subroutines in puff_utils.C:
void   get_lon_lat(char *volc, double &lon, double &lat);
int    check_lon_lat(char *vname, double lon, double lat);
void   show_volcs();
float  nrst_grid(float x, float dx);
int    get_eruptDate(char *erupt_date, char *arg_eruptDate);
//char   *outTime(time_t time);
void   meter2sphere(double &dx, double &dy, double &y);
int    run_puff();

// CONSTANT GLOBALS in puff_utils.C:
extern const double	Re; 
extern const double	pi;
extern const double	Deg2Rad;
extern const double	Rad2Deg;
extern const double	GravConst;
const double	puff_undef_dbl = -9999.;

extern const int PUFF_ERROR;
extern const int PUFF_OK;

// approximate earth radius in meters
#define PUFF_EARTH_RADIUS 6371220.0

#ifndef HAVE_LOGF
#define logF(x) log(reinterpret_cast<float>(x))
#endif

#endif //PUFF_H_
