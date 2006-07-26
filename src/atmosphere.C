#include <string> // std::string
#include <iostream> // std::cout, cerr, etc.
#include "atmosphere.h"
#include "puff_options.h" // Argument structure
#include "rcfile.h" // Resources class

#ifndef PUFF_OK
#define PUFF_OK 0
#endif
#ifndef PUFF_ERROR
#define PUFF_ERROR 1
#endif

extern Argument argument;
extern PuffRC resources;
//////////////////////////////////////////////////////////////////////////
Atmosphere::Atmosphere() {
  return;
}

//////////////////////////////////////////////////////////////////////////
Atmosphere::~Atmosphere() {
  return;
}

//////////////////////////////////////////////////////////////////////////
int Atmosphere::init(double *lon, double *lat) 
{
	rotGrid.lat = rotGrid.lon = rotGrid.angle = 0;
	center_lon = lon;
	center_lat = lat;
  if ( make_winds() == PUFF_ERROR) return PUFF_ERROR;
  return PUFF_OK;
}
//////////////////////////////////////////////////////////////////////////
float Atmosphere::xSpeed (float time, Particle *p) {

  return U.nnint(time, (*p).z, (*p).y, (*p).x);
  }
//////////////////////////////////////////////////////////////////////////
float Atmosphere::ySpeed (float time, Particle *p) {

  return V.nnint(time, (*p).z, (*p).y, (*p).x);
  }
//////////////////////////////////////////////////////////////////////////
float Atmosphere::zSpeed (float time, Particle *p) {

  return W.nnint(time, (*p).z, (*p).y, (*p).x);
  }
//////////////////////////////////////////////////////////////////////////
float Atmosphere::temperature (float time, Particle *p) {
  if (T.empty()) return 273.15f;
	// if the standard atmosphere approximation is used, T is
	// 2-dimensional
  if (T.ndims() == 2) return T.nnint(time, (*p).z);
  return T.nnint(time, (*p).z, (*p).y, (*p).x);
}
//////////////////////////////////////////////////////////////////////////
float Atmosphere::diffuseKh (float time, Particle *p) {

	return Kh.nnint(time, (*p).z, (*p).y, (*p).x);
	}
//////////////////////////////////////////////////////////////////////////
float Atmosphere::pressure (float time, Particle *p) {
  if (P.empty())
  {
    // return standard atmosphere h = RT/g ln(P/P0)
    // or P = P0 * exp(h * g)/(R * T)
    // gravitational constant in m/s^2
    const double grav = 9.807;
    // gas constant for air J/kg.K
    const double R_air = 287.0;
    const double height = (*p).z;
    const float pres = 1013.0 * exp(-height * grav / R_air / temperature(time, p) );
    return pres;
  }
  
  return P.nnint(time, (*p).z, (*p).y, (*p).x);
}
//////////////////////////////////////////////////////////////////////////
// determine and return fall velocity, positive is up, so falling particles 
// have negative fall velocities.
//////////////////////////////////////////////////////////////////////////
float Atmosphere::fallVelocity(float time, Particle *p)
{
  enum {FALL_LAMINAR, FALL_TRANS, FALL_TURBULENT} fall_regime;

  if (argument.sedimentation == FALL_CONSTANT)
  {
    static const double GravConst = (2./9.) * 1.08e9;
    return -(*p).size * (*p).size * GravConst;
  }
  
  // particle diameter is twice (*p).size
  const double dia = 2 * (*p).size;
  // particle height in kilometers
  const double height = (*p).z / 1000.0;
  
  // gravitational constant in m/s^2
  const double grav = 9.807;
  // gas constant for air J/kg.K
  const double R_air = 287.0;
  // air viscosity is fairly linear between 150-300 K
  // units here are N.s/m^2
  const double mu_air = (0.5675*temperature(time, p) + 14.35)*1e-7; 
  // for now, particle density is constant in kg/m^3
  const double rho_ash = 1500;
  // air density is determined from ideal gas law
  // rho = m/V = P/(RT)
  const double rho_air = pressure(time, p)/R_air/temperature(time, p);
  
  if (argument.sedimentation == FALL_STOKES)
  {
    fall_regime = FALL_LAMINAR;
  } else if (argument.sedimentation == FALL_REYNOLDS) {
    // there are three regimes here, turbulent, transition, and laminar
    // to find the region, we'll determine the two boundary values for a 
    // particular column height: laminar->transition and transition->turbulent
    // then the region is determined by the particle size relative to these
    // boundary values.
    
    // laminar->transitional boundary value line
    // log(d) = 0.01554 * h + 1.7
    // where d = particle diameter in microns
    //       h = particle's height in column in kilometers
    // particle 'p' height is 'z' in meters
    // multiple (*p).z by two to get diameter from radius
#ifdef HAVE_EXP10
    const double lam2tran_diam = 1e-6*exp10(0.01554*height + 1.7); 
#else
    const double exp_arg = 0.1554*height+1.7;
    const double lam2tran_diam = 1e-6*pow(10,exp_arg);
#endif // HAVE_EXP10
    // transitional->turbulent boundary value line
    // log(d) = 0.01607 * h + 3.0414
    // where d = particle diameter in microns
    //       h = particle's height in column in kilometers
#ifdef HAVE_EXP10
    const double tran2turb_diam = 1e-6*exp10(0.01607*height + 3.0414); 
#else
    const double exp_arg2 = 0.01607*height + 3.0414;
    const double tran2turb_diam = 1e-6*pow(10, exp_arg2); 
#endif 
    // find region
    if (2*(*p).size > tran2turb_diam)
    {
       fall_regime = FALL_TURBULENT;
    } else if (2*(*p).size < lam2tran_diam) {
       fall_regime = FALL_LAMINAR;
    } else {
      fall_regime = FALL_TRANS;
    }
  }
  
  // now switch on the fall regime
  double v;
  switch(fall_regime) 
  {
  case FALL_LAMINAR:
    // v = (g * rho * d^2)/ (18 * mu_air)
    v = grav * rho_ash * dia * dia / 18 / mu_air;
    break;
  case FALL_TRANS:
    // v = d*(4 * rho^2 * g^2 / (225 * mu * rho_air) ) ^(1/3)
    v = dia * cbrt(4 * rho_ash*rho_ash * grav*grav / 225 / mu_air / rho_air);
    break;
  case FALL_TURBULENT:
    // v = (3.1 * g * rho * d / rho_air)^(1/2)
    v = sqrt(3.1 * grav * rho_ash * 2 * dia / rho_air);
     break;
  default:
    std::cerr << "ERROR: fall velocity law is not defined\n";
    exit(0);
    break;
  }
  
//   double v_old = (2./9.)*1.08e9 * (*p).size * (*p).size;
//   double percent = (v-v_old);
//   std::cout << percent << "\t" << height << "\t" << dia*1e6 << std::endl;
 
  // positive is up, so falling particles have negative velocity
  if (v <= 0 || v > 0)
  {
    return -v; 
  } else {
    std::cerr << "ERROR: bad fall velocity computed: " << v << std::endl;
    exit(0);
    return 0.0;
  }
}   

//////////////////////////////////////////////////////////////////////////
int Atmosphere::make_winds ()
{

  std::string pUfile, pVfile;

  // Read U and V:
  // give the U wind a variable name so the netcdf Grid reader
  // can find the right variable id
  U.set_name((resources.getString("varU")).c_str() );
  // override this value with the command-line argument if given
  if ( argument.varU )
  {
    U.set_name (argument.varU);
  }

  if ( !argument.fileU ) {
    pUfile = resources.mostRecentFile(argument.eruptDate, "u", argument.runHours);
  } else {
    pUfile = argument.path;
    pUfile.append(argument.fileU);
  }
  
	(void) checkRotatedGrid(pUfile.c_str() );

  if (read_uni (U, &pUfile) == PUFF_ERROR) {
    return PUFF_ERROR;
  }
	
  // check that some data was read, otherwise bail
  if (U.n(VAR) == 0)
  {
    std::cerr << "ERROR: no " << U.name(VAR) << " data in " << pUfile << " for " << argument.eruptDate << std::endl;
    exit(0);
  }
  
  // Set reftime:
  reftime_t = unistr2time (U.reftime ());

// this check is probably unnecessary since mostRecentData is used now.
  // Check the reftime:
//   if (reftime_t > eruptDate_t) {
//     std::cout << "ERROR: Eruption date precedes wind reference time." << std::endl;
//     return PUFF_ERROR;
//   }

  // Continue reading V, make W:
  V.set_name((resources.getString("varV")).c_str() );
  // override this value with the command-line argument if given
  if (argument.varV)
  {
    V.set_name (argument.varV);
  }

  // if -fileV was not specified, use the resources file
  if ( !argument.fileV ) {
    pVfile = resources.mostRecentFile(argument.eruptDate, "v", argument.runHours);
  } else {
    pVfile = argument.path;
    pVfile.append(argument.fileV);
  }
  if (read_uni (V, &pVfile) == PUFF_ERROR) {
    return PUFF_ERROR;
  }

  // check that the reference times from both variables are the same.  If the
  // input files were the same, they probably are, but if different files were
  // used, the data might not correspond.
  if (unistr2time(V.reftime()) != reftime_t)
  {
    std::cerr << "ERROR: reference times do not correspond\n \"u\" data: "
    << U.reftime() << "\n\"v\" data: " << V.reftime();
    return PUFF_ERROR;
  }

  // now read in temperature data if it is available and necessary
	if (argument.needTemperatureData)
	{
    std::string Tfile;
    if ( !argument.fileT ) 
    {
      Tfile = resources.mostRecentFile(argument.eruptDate, "T", argument.runHours);
    } else {
      Tfile = argument.fileT;
    }
    T.set_name("T");
    if (Tfile.length() > 0)
    {
      read_uni(T, &Tfile);
    }
    // if no data was read, use standard atmosphere
    if ( T.empty() )
      {
      std::cout << "using standard atmosphere for temperature\n";
      T.TstandardAtm();
    }
	}
    
  // if 'level; is in pressure units, try to find a geopotential file
  // Use the 'U' variable and assume the V variable is the same.  This would
  // cause complications if the two files had different units for the level
  // variable, which is pretty unlikely (and impossible if the same input file
  // was used.
  std::string units = U.units(LEVEL); 
  // if the units are not meters, proceed
  if (units.find("meter") == std::string::npos) {
    Grid uniZ;

		// variable name, default 'Z'
    uniZ.set_name("Z");
		// resource file may specify it
  	uniZ.set_name((resources.getString("varZ")).c_str() );
  	// override this value with the command-line argument if given
  	if ( argument.varZ ) { uniZ.set_name(argument.varZ); }

    std::string Zfile;
    if ( !argument.fileZ ) {
      Zfile = resources.mostRecentFile(argument.eruptDate, "z", argument.runHours);
    } else {
      Zfile = argument.fileZ;
    }
    int warn;  // warning flag from PtoH function
    
    bool failedZfileRead = true;
    if ( Zfile.length() > 0 ) 
    {
      read_uni(uniZ, &Zfile);
      // uniZ may contain no data because appropriate data was not available
      // and some other day's data got read in.
      if (uniZ.n(VAR) <= 0 ) {
        std::cout << "\""<< Zfile << "\" contains no usable data, discarding\n";
      } else {
        // convert with it
        std::cout << "Converting levels to geopotential meters ... " << std::flush;
	P.pressureGridFromZ(uniZ);
	P.PtoH(uniZ, 0, warn);
  U.PtoH(uniZ, 0, warn);
	V.PtoH(uniZ, 0, warn);
	T.PtoH(uniZ, 0, warn);	
	
        std::cout << "done.\n" << std::flush;
        if ( warn ) std::cout << "WARNING: P-to-H interpolation outside valid range.\n";
	failedZfileRead = false;
        }
    }
    if (failedZfileRead) {  
      std::cout <<"Converting levels using standard-atmosphere approximation ... ";
      U.PtoH(1000, 7400, 100);
      V.PtoH(1000, 7400, 100);
      T.PtoH(1000, 7400, 100);
      std::cout << "done.\n";
    }

	// Re-do min/max values for LEVEL
	U.set_minimum(LEVEL); U.set_maximum(LEVEL);
	V.set_minimum(LEVEL); V.set_maximum(LEVEL);
	T.set_minimum(LEVEL); T.set_maximum(LEVEL);
  } // end of 'units were not in meters'

// make sure both U and V have some data.  The read() function doesn't die
// earlier since some variables, like T and Z can be empty and things still
// function, but all is lost if there is no wind data
		if (U.empty() or V.empty())
		{
			std::cout << "ERROR: no wind data\n";
			return PUFF_ERROR;
		}

// set the U and V coverage before calculating W since global data
// is handled differently in wind_create_W
  U.set_coverage ();
  V.set_coverage ();

  if (wind_create_W (U, V, W, Kh) == PUFF_ERROR) {
    return PUFF_ERROR;
  }

// now set W's coverage, but maybe it should just be U and V's, right?
  W.set_coverage ();

  if (argument.verbose) {
    W.display (INFO);
  }

  // set the filenames
  filenameU = pUfile;
  filenameV = pVfile;

  if (argument.saveWfile) 
  {
		// make the variable names distinct, because they might all have
		// been named something like 'data'
		U.set_name("u");
		V.set_name("v");
		W.set_name("w");
    U.write(&argument.saveWfilename);
    V.append(&argument.saveWfilename);
    W.append(&argument.saveWfilename);
		Kh.append(&argument.saveWfilename);
    // these values should be written as well, but they might be a different
    // size, so something has to be done to Grid::append() to deal with it
//    T.append(&argument.saveWfilename);
//    P.append(&argument.saveWfilename);

  }
		// check for a rotated grid
	//	(void) checkRotatedGrid(pUfile.c_str());

  return PUFF_OK;
}

////////////////////////////////////////////////////////////////////////
// attempt to read data from 'filename' into the Grid object &uni
// It also attempts to patch bad data
////////////////////////////////////////////////////////////////////////
int Atmosphere::read_uni (Grid & uni, std::string *filename)
{
  if (filename->length() == 0) 
	{
    std::cerr << "ERROR: no data found\n";
    return PUFF_ERROR;
  }

	// set the range of lat/lon data to read
	if ( argument.regionalWinds )
	{
	  const float region_size = argument.regionalWinds;
		uni.set_range(LON, *center_lon-region_size, *center_lon+region_size);
		uni.set_range(LAT, *center_lat-region_size, *center_lat+region_size);
	}
  
  std::cout << "Reading " << uni.name() << " from " << *filename << " ... " << std::flush;
  if (uni.read_cdf (filename, argument.eruptDate, argument.runHours) == PUFF_ERROR)
  {
    std::cerr << std::endl;
    std::cerr << "ERROR: Read failed for " << *filename << std::endl;
    return PUFF_ERROR;
  }
  std::cout << "done." << std::endl;
  
  // return if there are no values.  This happens when the file read in was
  // not appropriate, like when no geopotential height data is available for
  // the eruption date specified, so the mostRecentFile was some previous 
  // days.  By returning with no values, the standard atmosphere approximation
  // will be used.  However, if this is a wind file, it should be fatal, but 
  // we don't know right now, so things will fail later on.
  if (uni.n(VAR) <= 0 ) 
  {
    return PUFF_OK;
  }

	// only shift west (make all positive lon values) when we have
	// global data in degrees.  Regional data crossing the dateline
	// will be screwed up otherwise.
  if ((uni.min(LON) < 0 ) and
			(strstr(uni.units(LON),"deg") ) and
			(uni.isGlobal() ) ) uni.uni_shift_west();
  
  if (!argument.noPatch) 
  {
    float pct_bad = uni.pct_bad();
    if (pct_bad != 0)
    {
      printf("Patching %s data (%4.2f %%bad) ... ",uni.name(),pct_bad);
      std::cout <<  std::flush;
      uni.patch ();
      std::cout << "done." << std::endl;
    }

    // Warn if still bad:
    if (uni.pct_bad () != 0) 
    {
      std::cout << "\nWARNING: Wind data has bad values: " << uni.name () << std::endl;
    }
  }

  if (argument.verbose) uni.display (INFO);

  return PUFF_OK;
}
////////////////////////////////////////////////////////////////////////
//
// This routine takes the Grid objects, U and V
// and creates the vertical wind, W using divergence. It copies 
// other relevent info (e. g. reftime, units, etc) into the W object
//
// This routine originally assumed a Lat/Lon spherical surface! IF data is
// in nmc format, this is not so!
//
////////////////////////////////////////////////////////////////////////
int Atmosphere::wind_create_W (Grid & U, Grid & V, Grid & W, Grid & Kh)
{

  std::cout << "Making vertical wind ... " << std::flush;
 
  // check the units of the horizontal velocities
  bool good = false;
  if (strcmp (U.units (), "m/sec") == 0)
  {
    good = true;
  } else if (strcmp (U.units (), "meters/second") == 0) {
    good = true;
  } else if (strcmp (U.units (), "m/s") == 0) {
    good = true;
  }
  if (!good) {
    std::cout << "ERROR: U is not in \"m/sec\"\nFAILED\n";
    return PUFF_ERROR;
  }

  good = false;
  if (strcmp (V.units (), "m/sec") == 0)
  {
    good = true;
  } else if (strcmp (V.units (), "meters/second") == 0) {
    good = true;
  } else if (strcmp (V.units (), "m/s") == 0) {
    good = 1;
  }
  if (!good) {
    std::cout << "ERROR: V is not in \"m/sec\"\nFAILED\n";
    return PUFF_ERROR;
  }

  // some local variables
  int nx = U.n (LON);
  int nxm1 = nx - 1;
  int ny = U.n (LAT);
  int nym1 = ny - 1;
  int nz = U.n (LEVEL);
  //int nzm1 = nz-1;
  int nt = U.n (FRTIME);
  //int ntm1 = nt-1;
  int i, j, k, l;

  // create a Grid object the same size as the horizontal velocities
  W.create (nt, nz, ny, nx);

  // copy some information
  W.set_title ("Vertical wind");
  W.set_units (U.units ());
  W.set_name ("w_wind");

	// create the horizontal diffusivity object as well
	Kh.create (nt, nz, ny, nx);

  // copy some information
  Kh.set_title ("Horizontal diffusivity");
  Kh.set_units ("m^2/s");
  Kh.set_name ("kh");

  // copy dimension data into Kh,W, i.e. lat,lon,level
  for (i = 1; i <= 4; i++) {
    W.set_units (ID (i), U.units (ID (i)));
    W.set_name (ID (i), U.name (ID (i)));
    Kh.set_units (ID (i), U.units (ID (i)));
    Kh.set_name (ID (i), U.name (ID (i)));
    for (j = 0; unsigned (j) < U[ID (i)].size; j++) {
      W[ID (i)].val[j] = U[ID (i)].val[j];
      Kh[ID (i)].val[j] = U[ID (i)].val[j];
    }
  }

  W.set_reftime (U.reftime ());
  W.set_fill_value (U.fill_value ());
  W.set_valid_range (U.valid_range (0), U.valid_range (1));

	// set pointers for notational convenience
  float *lon = U[LON].val;
  float *lat = U[LAT].val;
  float *lev = U[LEVEL].val;
  float *ppU = U[VAR].val;
  float *ppV = V[VAR].val;
  float *ppW = W[VAR].val;
	float *ppKh = Kh[VAR].val;

	// make sure surface velocity is zero
	// level = X2
  int off;
  for (l = 0; l < nt; l++) {
    for (j = 0; j < ny; j++) {
      for (i = 0; i < nx; i++) {
	off = W.offset (l, 0, j, i);
	ppW[off] = 0.0;
	ppKh[off] = 0.0;
      }
    }
  }

	// calculate divergence
  //float C1 = Re*Deg2Rad;
  float C1 = 6371220.0 * 3.14159 / 180.0;	// about 111,198.67
  float Dx, Dy, Dz, Du, Dv, temp_float = 0;
  int i2, ip, im, jp, jm;
  int offp, offm;

  for (l = 0; l < nt; l++) {	// l -> time variable index
    for (k = 0; k < nz; k++) {	// k -> level variable index - skip ground level
			// surface velocity is zero, so the bottom (k=0) loop is only for
			// calculating Kh.  Set Dz to something (zero here) to avoid the
			// indexing error. It doesn't matter, we don't use Dz later on.
			if (k == 0) Dz = 0;
			else Dz = lev[k] - lev[k-1]; // meters
      //Dz = lev[k] - lev[k - 1];	// meters
      for (i = 0; i < nx; i++) {	// loop over all longitude values
	i2 = i;			// copy if 'i' because we may change it if on the pole
	ip = i + 1;		// "forward" index
	im = i - 1;		// "backward" index
	if (U.isGlobal ()) {	// we can loop the index value
	  if (i == 0) {
	    im = nxm1;
	  }
	  if (i == nxm1) {
	    ip = 0;
	  }
	}
	else {			// it is not global data
	  if (i == 0) {
	    im = i;
	  }			// backward index is zero if on the edge
	  if (i == nxm1) {
	    ip = i;
	  }			// forward index is last if o.t. edge
	}
	for (j = 0; j < ny; j++) {	// loop over all latitude values
	  jp = j + 1;		// "up" index
	  jm = j - 1;		// "down" index
	  // if it is global data, we'll forget this all together later
	  if (j == 0) {
	    jm = j;
	  }			// edge fudging
	  if (j == nym1) {
	    jp = j;
	  }			// edge fudging
	  offp = U.offset (l, k, j, ip);	// get the netCDF index
	  offm = U.offset (l, k, j, im);	// ditto
	  Du = ppU[offp] - ppU[offm];	// delta U in meters/sec

	  if (U.isProjectionGrid() && V.isProjectionGrid() ) {
	    // lon[ip], etc is actually in kilometers!
	    Dx = 1000.0 * (lon[ip] - lon[im]);
	  }
	  else {
	    Dx = C1 * cos (lat[j] * 3.14159 / 180.) * (lon[ip] - lon[im]);
	  }

	  offp = V.offset (l, k, jp, i2);
	  offm = V.offset (l, k, jm, i2);
	  Dv = ppV[offp] - ppV[offm];

	  if (U.isProjectionGrid() && V.isProjectionGrid() ) {
	    // lat[jp], etc is actually in kilometers!
	    Dy = 1000.0 * (lat[jp] - lat[jm]);
	  }
	  else {
	    Dy = C1 * (lat[jp] - lat[jm]);
	    if (Dy == 0)
	      Dy = C1 * 2 * (lat[jp] - lat[j]);
	  }

		// this gives a negative number on the first level, but the 1st level is
		// only computing Kh, and 'off' will not be used
	  off = W.offset (l, k - 1, j, i2);
	  // ppW[off] has already been initialized for all but the 1st level since 
	  // looping upward in 'k', so the 'k-1' value has been done.
		// However, W is not need on 1st level, so don't calculate 'temp_float'
	  if (k != 0) temp_float = ppW[off] - Dz * (Du / Dx + Dv / Dy);
	  // if we're at the pole in global data, this method will
	  // not work to calculate W since Dx makes no sense and
	  // Dv is ambiguous; negative and postive values are the same.
	  if ((j == 0 || j == ny - 1) && U.isGlobal ()) {
	    temp_float = 0;
	  }
	  if (!(temp_float < 0) && !(temp_float > 0) && !(temp_float == 0)) {
	    std::cerr << "Invalid value of W calculated.\n";
	    return PUFF_ERROR;
	  }
	  off = W.offset (l, k, j, i2);
		// don't assign the bottom level (k=0) a value since W is zero there.
		// It was already zero'ed out, so simply skip.  We are only doing the
		// k=0 level to calculate Kh anyway.
	  if (k != 0) ppW[off] = temp_float;
		ppKh[off] = 1/sqrt(2)*0.14*0.14*Dx*Dx*sqrt(powf((Dv/Dx+Du/Dy),2)+powf((Du/Dx-Dv/Dy),2));
	}
      }
    }
  }

  std::cout << "done.\n";
  
  return PUFF_OK;
}

////////////////////////////////////////////////////////////////////////
time_t Atmosphere::reference_time() {
  return reftime_t;
}
////////////////////////////////////////////////////////////////////////
bool Atmosphere::isProjectionGrid()
{
  return (U.isProjectionGrid() && V.isProjectionGrid() );
}
////////////////////////////////////////////////////////////////////////
bool Atmosphere::isGlobal()
{
  return (U.isGlobal() && V.isGlobal());
}
////////////////////////////////////////////////////////////////////////
bool Atmosphere::containsXYPoint(float x, float y)
{
  // Boundary check variables:
  float xmin = U.min(LON);
  float xmax = U.max(LON);
  float ymin = U.min(LAT);
  float ymax = U.max(LAT);

//  some lat data is +90 -> -90, so swap the values.  This is ugly, but
//  I think a decent optimizer will fix it.
  if (ymin > ymax) 
  {
    float yswap = ymin;
    ymin = ymax;
    ymax = yswap;
  }

  if (x < xmin || x > xmax || y < ymin || y > ymax)
  {
    return false;
  }
  return true;
}
////////////////////////////////////////////////////////////////////////
// check is this location is within the vertical atmospheric bounds.  Return
// 0 if so, otherwise +1 if greater than zmax, and -1 is less than zmin.
int Atmosphere::containsZPoint(float z)
{
  // Boundary check variables:
  float zmin = U.min(LEVEL);
  float zmax = U.max(LEVEL);

	if (z < zmin) return -1;
	if (z > zmax) return +1;
	return 0;
}
////////////////////////////////////////////////////////////////////////
bool Atmosphere::containsXYZPoint(float x, float y, float z)
{
  // Boundary check variables:
  float xmin = U.min(LON);
  float xmax = U.max(LON);
  float ymin = U.min(LAT);
  float ymax = U.max(LAT);
  float zmin = U.min(LEVEL);
  float zmax = U.max(LEVEL);

//  some lat data is +90 -> -90, so swap the values.  This is ugly, but
//  I think a decent optimizer will fix it.
  if (ymin > ymax) 
  {
    float yswap = ymin;
    ymin = ymax;
    ymax = yswap;
  }

  if (x < xmin || x > xmax || 
      y < ymin || y > ymax ||
      z < zmin || z > zmax)
  {
    return false;
  }
  return true;
}
////////////////////////////////////////////////////////////////////////

void Atmosphere::checkRotatedGrid(const char *file)
{
  // open netCDF file read-only
  NcFile ncfile(file, NcFile::ReadOnly);
  if (! ncfile.is_valid() ) 
	{
    std::cerr << "ERROR: Failed to open netCDF file \"" << file << "\" to get grid rotation information\n";
		return;
  }
  
	// non-fatal in case this is not a rotated grid
  NcError ncerr(NcError::silent_nonfatal);

  NcVar *vp;
  vp = ncfile.get_var((NcToken)"RotLat");
	if (! vp ) return; 

  rotGrid.lat = vp->as_float(0);
	vp = ncfile.get_var((NcToken)"RotLon");
	if (! vp ) 
	{
		checkRotatedGridError();
		return;
	}
	rotGrid.lon = vp->as_float(0);

	vp = ncfile.get_var((NcToken)"RotAngle");
	if (! vp-> is_valid() )
	{
		checkRotatedGridError();
		return;
	}
	rotGrid.angle = vp->as_float(0);

	// check is values seem sane
	if (( fabs(rotGrid.lat) > (float)90) ||
			( fabs(rotGrid.lon) > (float)180) ||
			( fabs(rotGrid.angle) > (float)90) )
	{
		std::cerr << "WARNING: rotated grid values appear incorrect.\n  'lon' = " << rotGrid.lon << "\n 'lat' = " << rotGrid.lat << "\n 'angle' = " << rotGrid.angle << std::endl;
	}

	// can't handle an angle rotation
	if (rotGrid.angle != 0) std::cerr << "WARNING: non-zero grid rotation angle, and I can't handle that. FIXME!\n";

	// center values are for reading regional winds
	*center_lat += double(-90-rotGrid.lat);
	*center_lon -= (double)rotGrid.lon;

	std::cout << "WARNING: operating on a rotated grid.  Volcano lat/lon values may be reported incorrectly\n";

	return;
}
////////////////////////////////////////////////////////////////////////////
void Atmosphere::checkRotatedGridError()
{
	std::cerr << "ERROR: rotated grid specification is incomplete.  RotLat, RotLon, RotAngle are all required is any are specified.  Defaulting to 0,0,0\n";
	rotGrid.lon = rotGrid.lat = rotGrid.angle = 0;
	return;
}
