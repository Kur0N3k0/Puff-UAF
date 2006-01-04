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
 
#ifdef HAVE_CONFIG_H
#include <config.h> // autoconf defines
#endif
#include "Grid.h"

#ifdef HAVE_NETCDFCPP_H
#include <netcdfcpp.h>
#else
#include "netcdfcpp.h"
#endif // HAVE_NETCDFCPP_H

#ifdef HAVE_UDUNITS_H
extern "C" {
#include <udunits.h>
}
#endif

#include <cstdio> //sscanf
#include <vector>
#include <string>

// local functions
const double timeOffsetUsingUDUnits(NcVar *vp);

////////////////////////////////////////////////////////////////////////////
// CLASS CDF READER:
////////////////////////////////////////////////////////////////////////////
// new reader using libnetcdf_c++
// there may be multiple files to read, and they are colon-delimited.  The
// list is tokenized and the first file is used to set dimension names, sizes,
// and values.  If multiple files are specified, the forecast data is not read,
// and instead only the zero-hour forecast (analysis) data is read from each
// file.  In this case, the values for the time dimension are passed here in
// the colon-delimited file list.
////////////////////////////////////////////////////////////////////////////

int Grid::read_cdf(const std::string* cdf_file, 
                   const char* eruptDate, 
		   const double runHours) 
{
  
  // open the file read-only
  NcFile ncfile(cdf_file->c_str(), NcFile::ReadOnly);
  if (! ncfile.is_valid() ) {
    std::cerr << "failed to open netCDF file " << *cdf_file <<" as a c++ object\n";
    return FG_ERROR;
    }
  
  NcVar* vp; // Pointer to a NcVar object for the variable
  // NcToken is a typecast for char currently, but could change in the future
  vp = ncfile.get_var((NcToken)fgData[VAR].name);
  // Puff needs 4-dimensional data, so fail otherwise
  if ( vp->num_dims() != 4 ) 
  {
    std::cerr << "ERROR: file " << cdf_file << " does not have 4-dimensional data for variable " << fgData[VAR].name << std::endl;
    return FG_ERROR;
  } else {
    fgNdims = 4;
  }

  // get the variable size 
  long int *edges = vp->edges();
  
  // retrieve global attributes
  get_global_attributes(&ncfile);
  // retrieve variable attributes
  get_attributes(vp, VAR);
 
  // set the error status non-fatal so we can test for the presence/absence
  // of different possible dimension variables, particularly 'time'
  NcError ncerr(NcError::silent_nonfatal);
  
  // retrieve dimensions
  NcVar *dp; // pointer to a dimension object
	long int *dim_offset = new long int[5]; // dimension offsets to read on only parts of data
  int o_d_size[5];  // keep record of the original dimension sizes
  // loop over all the dimensions, which are indexed 1-4 in the Grid object
  // 0 is the variable data VAR
  for (int dim_idx = 1; dim_idx < (int)fgNdims+1 ; dim_idx++) 
  {
    // get the dimension variable's name
    NcToken dname =  vp->get_dim(dim_idx-1)->name() ;
    // set NcVar 'dp' to this dimension
    // for record dimensions, we want 'valtime' as the variable
    if (strcmp(dname,"record") == 0) 
    {
      dp = ncfile.get_var("valtime");
    } else {
      dp = ncfile.get_var(dname);
    }
    // copy the name into the Grid object
    strcpy(fgData[dim_idx].name,dp->name());
    // get the size of the dimension, edges is declared 'long' above
    edges = dp->edges();
    // retrieve dimension attributes
    get_attributes(dp, dim_idx);
    //put the values into type NcValues and typecast later
    NcValues *values = dp->values();
    // sequentially copy since data types may not be the same 
      fgData[dim_idx].size = 0;
      //fgData[dim_idx].val = new float[values->num()];
      fgData[dim_idx].val = (float*)calloc(values->num(),sizeof(float));
    for (int i=0; i<values->num(); i++)
    {
      fgData[dim_idx].val[i] = values->as_float(i);
      fgData[dim_idx].size++;
    }
		o_d_size[dim_idx]=fgData[dim_idx].size;
		adjust_dimensions(dim_offset, dim_idx);
  }  

  // at this point, all the dimensions have been loaded in their entirety.
  // However, I want to cull the time variable since we may not need all its
  // values.  The time variable can be slippery because values may be in units
  // like "hours since 1992-1-1", so we use libUDUnits to convert this.  
  // Puff references the values in the time variable to the Grid::reftime.
  
  // the reftime could have been set by a global attribute already
  if (strlen(fgReftime) == 0)
  {
    // some conventions have the time variable in "hours since ..." and the
    // reftime can be obtained from units attribute.
    if (strstr(fgData[FRTIME].units,"hours since") != NULL)
    {
      dp = ncfile.get_var((NcToken)fgData[FRTIME].name);
      strcpy(fgReftime,reftimeFromNcVar(dp));
    } else if (strstr(fgData[FRTIME].units,"hours") != NULL) {
    // old uni2puff-generated files have valtime_offset in units of simply
    // 'hours' and the retime is a variable of characters.
      dp = ncfile.get_var((NcToken)"reftime");
      if (dp) 
      {
        NcValues *val = dp->values();
	for (int i=0;i<val->num();i++)
	{
          fgReftime[i]=val->as_char(i);
	}
      }
    } else {
    // unable to get the reference time
      std::cerr << "ERROR: unable to get the reference time for the file " <<
              *cdf_file << std::endl;
      return FG_ERROR;    
    }
    
  
  } // if strlen(fgReftime) == 0
  
  // sort the time values since sometimes data comes in non-sequentially
  // we make sure the VAR data is read in the correct order below
  shellsort(fgData[FRTIME].val, fgData[FRTIME].size);
  
  // we want to read data that complete covers from eruptDate until
  // (eruptDate+runHours) and discard the rest.
  // We'll convert everything to a (time_t) form,
  // then remove all members of fgData[FRTIME] that we will not need.  Then,
  // read the records into fgData[VAR]
  
  unsigned int index1 = 0;
      
  for (index1 = 0; (index1<fgData[FRTIME].size); index1++ )
  {
    time_t startTime = unistr2time(eruptDate);
    time_t thisTime  =
       unistr2time(fgReftime)+time_t(fgData[FRTIME].val[index1]*3600);
    
    if (startTime == thisTime) 
    {
      break;
    } else if (startTime < thisTime) { // went too far, need the previous value
          if (index1 > 0) index1--;  // dont decrement if already at zero
      break;
    }
  }
    
  // now set the last index
  unsigned int index2 = index1;
  while(index2 < fgData[FRTIME].size)
  {
    time_t endTime = unistr2time(eruptDate) + time_t(runHours*3600);
    time_t thisTime = 
      unistr2time(fgReftime)+time_t(fgData[FRTIME].val[index2]*3600);
    
    if (endTime <= thisTime) // want this value and no more
    {
      break;
    } else { 
      index2++;  
    }
  }
  
  // eruption duration may span beyond the data, in which case index2 is one
  // too large, so decrement it
  if (index2 > (fgData[FRTIME].size-1) ) index2--;
  
  // now reduce time vector to only the values we want, one value is no good
  if (index2 > index1)
  {
    fgData[FRTIME].size = index2-index1+1;
  } else {
    fgData[FRTIME].size = 0;
  }
  
  for (unsigned int i = 0;i <fgData[FRTIME].size; i++)
  {
    fgData[FRTIME].val[i]=fgData[FRTIME].val[index1];
    index1++;  
  }
  
  if (fgData[FRTIME].size == 0) 
  {
    std::cerr << "\nWARNING: file " << *cdf_file << " does not contain data covering ";
    std::cerr << "the eruption beginning at " << eruptDate << std::endl;
  }
  
  // now read in the data
  // set the NcVar pointer to the time variable
  dp = ncfile.get_var((NcToken)fgData[FRTIME].name);
  // set another NcVar pointer to the variable data
  vp = ncfile.get_var((NcToken)fgData[VAR].name);
  // allocate space
  fgData[VAR].size=fgData[FRTIME].size *
                   fgData[LON].size *
                   fgData[LAT].size *
                   fgData[LEVEL].size;
  if (fgData[VAR].size > 0)
		fgData[VAR].val = (float*)calloc(fgData[VAR].size,sizeof(float));
    //fgData[VAR].val = new float[fgData[VAR].size];

 // NcTypedComponent *vp2 = ncfile.get_var(fgData[VAR].name);
  //NcValues *values = vp2->values(); 
  
	int v2_idx = 0;
  for (unsigned int recNum=0; recNum<fgData[FRTIME].size; recNum++)
  {
    // get the record index, the time variable may be one of several types
    int recIdx;
    if (dp->type() == ncDouble) 
    {
      double t = (double)fgData[FRTIME].val[recNum];
      recIdx = dp->get_index(&t);
    } else if (dp->type() == ncFloat) {
      float t = (float)fgData[FRTIME].val[recNum];
      recIdx = dp->get_index(&t);
    } else if (dp->type() == ncLong) {
      long int t = (long int)fgData[FRTIME].val[recNum];
      recIdx = dp->get_index(&t);
    } else {
      std::cerr << "ERROR: time variable type " << dp->type() <<" is not valid\n";
      return FG_ERROR;
    }

		// debug
		vp->set_cur(recIdx,dim_offset[LEVEL],dim_offset[LAT],dim_offset[LON]);
		long *counts = new long[4];
		counts[0]=1;
		counts[1]=fgData[LEVEL].size;
		counts[2]=fgData[LAT].size;
		counts[3]=fgData[LON].size;

		NcType type = vp->type();
		if (type == ncShort)
		{
			short *v = new short[fgData[VAR].size];
			if (vp->get(v, counts) == false) { std::cout << "ERROR: wrong variable type\n";}
			for (int i=0; i<counts[1]*counts[2]*counts[3]; i++){
				fgData[VAR].val[v2_idx]=scale_factor*(float)v[i] + add_offset;
				v2_idx++;
			}
		}
		if (type == ncFloat)
		{
			float *v = new float[fgData[VAR].size];
			if (vp->get(v, counts) == false) { std::cout << "ERROR: wrong variable type\n";}
			for (int i=0; i<counts[1]*counts[2]*counts[3]; i++)
			{
				if (v[i] <= 0 or v[i] > 0)
					fgData[VAR].val[v2_idx]=scale_factor*(float)v[i] + add_offset;
				else
					fgData[VAR].val[v2_idx] = fgFillValue;
				v2_idx++;
			}
		}

//		vp2->set_cur(dim_offset[LON],dim_offset[LAT],dim_offset[LEVEL],recIdx);
		// load values.  Read one value at a time from memory.  Indexing
		// is a little weird because we might be hyperslabbing, which means
		// we need the dimension offsets determined earlier to line up the
		// corners properly
//	  float val;
//	  for (int i = 0; i < fgData[LEVEL].size; i++) {
//			for (int j = 0; j < fgData[LAT].size; j++) {
//				for (int k = 0; k < fgData[LON].size ; k++) {
//					int d_idx = o_d_size[LON]*o_d_size[LAT]*o_d_size[LEVEL]*recIdx+
//						o_d_size[LON]*o_d_size[LAT]*(i + dim_offset[LEVEL]) +
//						o_d_size[LON]*(j+dim_offset[LAT]) + (k+dim_offset[LON]);
//					int v_idx = offset(recNum,i,j,k);
//					val = values->as_float(d_idx);
//					if ((fgData[VAR].val[v_idx] - (scale_factor*val+add_offset))>0.1){
//						std::cout << "not equal\n";
//					}
//					if (val <= 0 || val >= 0)
//						fgData[VAR].val[v_idx] =  scale_factor * val + add_offset ;
//					else fgData[VAR].val[v_idx] = fgFillValue;
//		  } } } 

    }
  // delete values; 
//	 delete vp2;
     
  
  
  delete[] edges;
  return FG_OK;
  }

////////////////////////////////////////////////////////////////////////////
void Grid::make_monotonic()
{
	bool ascend = (fgData[LON].val[0] < fgData[LON].val[1]);
	if (!ascend) return;
	for (unsigned int i = 0; i < fgData[LON].size-1; i++)
	{
		if (fgData[LON].val[i] > fgData[LON].val[i+1]) 
			fgData[LON].val[i+1] += 360.0;
	}
	return;
}
	
////////////////////////////////////////////////////////////////////////////
void Grid::adjust_dimensions(long int *offsets, int dim_idx)
{

	offsets[dim_idx] = 0;
	// number of bytes to memmove(), assume entire array at first
	size_t move_size = fgData[dim_idx].size*sizeof(fgData[dim_idx].val[0]); 
	int offset = 0;  // local offset value, set *offsets before returning
	// only adjust the latitude and longitude data
	if (strcmp(fgData[dim_idx].name,"lat") && strcmp(fgData[dim_idx].name,"lon"))
		return;

	// longitude may have non-monotonic values, so adjust those
	if (dim_idx == LON) make_monotonic();
  float low = fgData[dim_idx].range[0];
	float high = fgData[dim_idx].range[1];  // lower and higher values 
	bool up = (fgData[dim_idx].val[0] < fgData[dim_idx].val[1]);

  for (int i = 0; i < (int)fgData[dim_idx].size-1; i++)
	{
		if ((up and (low > fgData[dim_idx].val[i+1]) ) or 
		    (!up and (high < fgData[dim_idx].val[i+1]) ) ) 
		{
			offset++;
			// decrement the amount of mem to move by one value
			move_size -= sizeof(fgData[dim_idx].val[0]);
	  }
	}

  if (offset != 0) memmove(fgData[dim_idx].val,&fgData[dim_idx].val[offset], move_size);

	// adjust the size
	fgData[dim_idx].size -= offset;

	// set the size to the high value
	for (int i = 0; i < (int)fgData[dim_idx].size; i++)
	{
		if (up and (high < fgData[dim_idx].val[i]))
			fgData[dim_idx].size = i+1;
		if (!up and (low > fgData[dim_idx].val[i]))
			fgData[dim_idx].size = i+1;
	}

	offsets[dim_idx] = offset;
  return;
}
////////////////////////////////////////////////////////////////////////////
// retrieve dimension and variable attributes
////////////////////////////////////////////////////////////////////////////
void Grid::get_attributes(NcVar *vp, int idx) {
  // retrieve attributes
  int natt = vp->num_atts();  // number of attributes for this variable
  NcAtt *att; // create an attribute object
  NcToken attname; // attribute name, assumed to be a const char*
  // loop through the attributes, filling the appropriate places
  for (int i = 0; i< natt; i++) {
    att = vp->get_att(i);
    attname = att->name();
    
    // a block of if/elseif to retrieve attributes
    if (strcmp(attname,"valid_range") == 0 ) 
    {
      if (att->num_vals() != 2 ) 
      {
        std::cerr << "WARNING: valid_range does not contain 2 values as expected\n";
      } else {
        fgData[idx].range[0]=att->as_float(0);
        fgData[idx].range[1]=att->as_float(1);
      }
	
    }  else if (strcmp(attname,"_FillValue") == 0) {  // valid_range 
      if (att->num_vals() != 1 ) 
      {
        std::cerr << "WARNING: _FillValue does not contain 1 value as expected\n"; 
      } else {
        fgFillValue = att->as_float(0);
      }
            
    } else if (strcmp(attname,"units") == 0) {  // _FillValue
      strcpy(fgData[idx].units, att->as_string(0));
      
      
    } else if (strcmp(attname,"add_offset") == 0) { // add_offset
      if (att->num_vals() != 1 )
      {
	std::cerr << "WARNING: add_offset attribute for variable " << vp->name()
	     << " does not contain 1 value as expected\n";
      } else {
        add_offset = att->as_double(0);
      }
      
    } else if (strcmp(attname,"scale_factor") == 0) { // scale_factor
      if (att->num_vals() != 1 )
      {
	std::cerr << "WARNING: scale_factor attribute for variable " << vp->name()
	     << " does not contain 1 value as expected\n";
      } else {
        scale_factor = att->as_double(0);
      }
      
    } else {
      //cout << "attribute " << attname << " not used\n";
    }     
  }
 
  return;
}
////////////////////////////////////////////////////////////////////////////
// retrieve global attributes
////////////////////////////////////////////////////////////////////////////
void Grid::get_global_attributes(NcFile *vp) {
  // retrieve attributes
  int natt = vp->num_atts();  // number of attributes for this variable
  NcAtt *att; // create an attribute object
  NcToken attname; // attribute name, assumed to be a const char*
  // loop through the attributes, filling the appropriate places
  for (int i = 0; i< natt; i++) {
    att = vp->get_att(i);
    attname = att->name();
          
    if (strcmp(attname,"title") == 0) { // title (global attribute)
      if (att->type() == ncChar ) 
      {
        strncpy(fgTitle, att->as_string(0), att->num_vals() );
      } else {
        std::cerr << "WARNING: title attribute is not a character string\n";
      }
      
    } else if (strcmp(attname,"base_date") == 0) { //base_date (global att)
      if (att->num_vals() != 3 ) 
      {
        std::cerr << "WARNING: base_date does not contain 3 values as expected\n";
      } else {
//        reftimeFromBasedate(att->as_int(0),att->as_int(1),att->as_int(2));
      }      
    
    } else if (strcmp(attname,"reftime") == 0) {  // reftime (global att)
      if (att->num_vals() != 1)
      {
        std::cerr << "WARNING: reftime attributed does not have 1 value\n";
      } else {
         strcpy(fgReftime, att->as_string(0) );
      }
      
      
    } else {
      //cout << "attribute " << attname << " not used\n";
    }     
  }
 
  return;
}
////////////////////////////////////////////////////////////////////////////
// CLASS CDF WRITER:
////////////////////////////////////////////////////////////////////////////
//  net write function using netCDF C++
int Grid::write_cdf(char *cdf_file) {
  // create/clobber netCDF file
  NcFile ncfile(cdf_file, NcFile::Replace);
  // create dimensions, we need then all later for making the main variable
  NcDim *d_time = ncfile.add_dim((NcToken)fgData[FRTIME].name, fgData[FRTIME].size);
  NcDim *d_lev = ncfile.add_dim((NcToken)fgData[LEVEL].name, fgData[LEVEL].size );
  NcDim *d_lat = ncfile.add_dim((NcToken)fgData[LAT].name, fgData[LAT].size );
  NcDim *d_lon = ncfile.add_dim((NcToken)fgData[LON].name, fgData[LON].size);
  // reftime dimension, add one for the closing \0
  NcDim *d_rtim = ncfile.add_dim((NcToken)"timelen", strlen(fgReftime)+1);
  // add dimension variables
  NcVar *vp;
  vp = ncfile.add_var((NcToken)fgData[FRTIME].name, ncFloat, d_time);
  vp->put(&fgData[FRTIME].val[0],vp->edges());
  vp->add_att((NcToken)"units",fgData[FRTIME].units);
  vp = ncfile.add_var((NcToken)fgData[LEVEL].name, ncFloat, d_lev);
  vp->put(&fgData[LEVEL].val[0],vp->edges());
  vp->add_att((NcToken)"units",fgData[LEVEL].units);
  vp = ncfile.add_var((NcToken)fgData[LAT].name, ncFloat, d_lat);
  vp->put(&fgData[LAT].val[0],vp->edges());
  vp->add_att((NcToken)"units",fgData[LAT].units);
  vp = ncfile.add_var((NcToken)fgData[LON].name, ncFloat, d_lon);
  vp->put(&fgData[LON].val[0],vp->edges());
  vp->add_att((NcToken)"units",fgData[LON].units);
  vp = ncfile.add_var((NcToken)"reftime", ncChar, d_rtim);
  vp->put(fgReftime, vp->edges());
  
  // add wind variable
  vp = ncfile.add_var((NcToken)fgData[0].name, ncFloat, d_time, d_lev, d_lat, d_lon);
  vp->put(&fgData[VAR].val[0], vp->edges());
  vp->add_att((NcToken)"units",fgData[0].units);
  
  // add global attributes
  ncfile.add_att((NcToken)"coverage",coverage);
  ncfile.add_att((NcToken)"title","Puff-generated windfield");
  
  return FG_OK;
  }
////////////////////////////////////////////////////////////////////////////
// append data to this file
////////////////////////////////////////////////////////////////////////////
int Grid::append(std::string *file)
{
  NcFile ncfile(file->c_str(), NcFile::Write);
  
  // retrieve dimensions
  NcDim *d_time = ncfile.get_dim((NcToken)fgData[FRTIME].name);
  NcDim *d_lev = ncfile.get_dim((NcToken)fgData[LEVEL].name);
  NcDim *d_lat = ncfile.get_dim((NcToken)fgData[LAT].name);
  NcDim *d_lon = ncfile.get_dim((NcToken)fgData[LON].name);
  
  // create a variable pointer
  NcVar *vp;
  // create a new variable
  vp = ncfile.add_var((NcToken)fgData[VAR].name, ncFloat, d_time, d_lev, d_lat, d_lon);
  // add the data
  vp->put(&fgData[VAR].val[0], vp->edges());
  vp->add_att((NcToken)"units", fgData[VAR].units);
  
  return FG_OK;
  }
////////////////////////////////////////////////////////////////////////////
// return the number of seconds from 1-1-1970 (which is zero for the time_t
// variable) and the units attribute of the variable 'vp'. Negative values
// indicate that the units attribute preceeds 1-1-1970.  Return a double value
// because the offset can be large for 'time since 1-1-1'.
////////////////////////////////////////////////////////////////////////////
const double timeOffsetUsingUDUnits(NcVar *vp) {

#ifdef HAVE_LIBUDUNITS
  if ( utInit("") != 0) {
    std::cerr << "ERROR: failed to initialize UDUnits library\n";
    return (const double)NULL;
    }
  NcAtt *att = vp->get_att((NcToken)"units");
  // check validity
  if (! att->is_valid()) {
    std::cerr << "ERROR: \"units\" attribute does not exist for variable " 
         << vp->name() << std::endl;
    return (const double)NULL;
    }
  // get the unit attribute, should be "time since ...." 
  utUnit reftime_unit_sct;  // unit structure for reftime
  utUnit time_t_unit_sct;  // time_t unit structure
  utScan(att->as_string(0), &reftime_unit_sct);
  utScan("hours since 1970-1-1", &time_t_unit_sct);
  // recheck attribute
  if (!utIsTime(&reftime_unit_sct) || !utHasOrigin(&reftime_unit_sct) ) {
    std::cerr << "ERROR: units attribute is \"" << att->as_string(0);
    std::cerr << "\" which is not a valid UTUnit time unit\n";
    return (const double)NULL;
    }
  const double offset = (reftime_unit_sct.origin) - (time_t_unit_sct.origin);

  utTerm();	   
  return offset;
    
#else
  return (const long int)NULL;
#endif
  }
////////////////////////////////////////////////////////////////////////////
void Grid::reftimeFromBasedate(int y, int m, int d)
{
  char *str = new char[16];
  int ret = 0;
  if (m < 10 && d < 10)
  {
    ret = sprintf(str, "%4i 0%1i 0%1i 00:00", y, m, d);
  } else if (m >= 10 && d < 10) {
    ret = sprintf(str, "%4i %2i 0%1i 00:00", y, m, d);
  } else if (m < 10 && d >= 10) {
    ret = sprintf(str, "%4i 0%1i %2i 00:00", y, m, d);
  } else {
    ret = sprintf(str, "%4i %2i %2i 00:00", y, m, d);
  }
  if (ret != 16)
  { 
    std::cout << "\nWARNING: failed to create reference date from base_date attributes:\n year = " << y << " mon = " << m << " day = " << d << std::endl;
  } else {
    strcpy(fgReftime, str);
  }
  if (str) delete str;
  return;
}
////////////////////////////////////////////////////////////////////////////
char *Grid::reftimeFromNcVar(NcVar *vp) {
  static bool warnedAboutVaryingValues = false;

  // get the time offset from the units if possible
  double offset = timeOffsetUsingUDUnits(vp);
  // NULL returned if it failed
  if (!offset) {
    std::cout << std::endl << "WARNING: assuming \"time since 1992-1-1 00:00\"\n";
    offset = 694224000;  // seconds from 1970-1-1 to  1992-1-1
    }
  
  return time2unistr(time_t(offset));
  
  const int numDims = vp->num_dims();
  if (numDims != 1) {
    std::cerr << "ERROR: expected variable " << vp->name() << " to be one-dimensional\n";
    return (char*)NULL;
    }
  int size = 1;  // size of this variable
  long int *edges = vp->edges();  // vector describing shape
  for (int i=0; i<numDims; i++) { size *= edges[i]; }
  double valtimeOffset;
  double *val = new double[size];
  vp->get(val, edges);
  // only check these values and warn once
  for (int i=1; i<size; i++) { 
    if ( (val[i] != val[i-1]) && !warnedAboutVaryingValues)  {
      std::cerr << std::endl;
      std::cerr << "WARNING: expected all values of variable \"" << vp->name();
      std::cerr << "\" to be the same\n" << val[i] << " != " << val[i-1] << std::endl;
      warnedAboutVaryingValues = true;
      }
    }
  valtimeOffset = val[0];
  delete[] val;
  return time2unistr(time_t(3600*valtimeOffset)+(time_t)offset);

  }
////////////////////////////////////////////////////////////////////////////
// initialize the 'maparam' struct that holds information for regional grids.
// Use netCDF file '*filename' to read necessary information. 
////////////////////////////////////////////////////////////////////////////
int init_grid(char* filename, maparam *proj_grid) {
  static bool configured = false;  // only initialize one grid
  if (configured) { return FG_OK;}
  
  double lat1 = -9999,  // these values need to be read from the file
         lon1 = -9999,  // lower left corner of the grid
				 latin1 = -9999,
				 latin2 = -9999,
         lonv = -9999;  // vertical longitude value
	 
  double x1 = 0, y1 = 0;// lower left grid values, default to (0,0)
  
  const static double delx = 1.0;  // create a 1x1-meter grid
  int num_vars = 0;
	char *grid_type;
  
  // open netCDF file read-only
  NcFile ncfile(filename, NcFile::ReadOnly);
  if (! ncfile.is_valid() ) {
    std::cerr << "ERROR: Failed to open netCDF file \"" << filename << "\" to get grid initialization information\n";
    return FG_ERROR;
    }
  
  num_vars = ncfile.num_vars();
  NcVar *vp;
  // try to get any/all grid information
  for (int idx = 0; idx < num_vars; idx++) {
    vp = ncfile.get_var(idx);
    if (strcmp((char*)vp->name(),"La1") == 0) lat1 = vp->as_double(0);
    if (strcmp((char*)vp->name(),"Lo1") == 0) lon1 = vp->as_double(0);
    if (strcmp((char*)vp->name(),"Lov") == 0) lonv = vp->as_double(0);
    if (strcmp((char*)vp->name(),"Latin1") == 0) latin1 = vp->as_double(0);
    if (strcmp((char*)vp->name(),"Latin2") == 0) latin2 = vp->as_double(0);
    if (strcmp((char*)vp->name(),"vertical_longitude") == 0) lonv = vp->as_double(0);
		if (strcmp((char*)vp->name(),"grid_type") == 0) grid_type = vp->as_string(0);
		// backwards compatibility with afwa2puff-generated Puff files
    if (strcmp((char*)vp->name(),"latitude1") == 0) lat1 = vp->as_double(0);
    if (strcmp((char*)vp->name(),"longitude1") == 0) lon1 = vp->as_double(0);
		if (strcmp((char*)vp->name(),"map_projection") == 0)
		{
			char c = vp->as_char(0);
			switch (c)
			{
				 case 'L': grid_type = strdup("Lambert"); break;
				 case 'P': grid_type = strdup("polar"); break;
			}
		}

    // make the lower left corner of the grid correspond with the x,y
    // values read from the data file
    if (strcmp((char*)vp->name(),"x") == 0) x1 = vp->as_double(0);
    if (strcmp((char*)vp->name(),"col") == 0) x1 = vp->as_double(0);
    if (strcmp((char*)vp->name(),"y") == 0) y1 = vp->as_double(0);
    if (strcmp((char*)vp->name(),"row") == 0) y1 = vp->as_double(0);
//    if (strcmp((char*)vp->name(),"Dx")  == 0) delx = vp->as_double(0);
    }
  // tangent latitude is probably lat1
;
  // continue if values got redefined to something sane
//  if (lat1 >= -90 && lon1 >=-360 && lonv >= -360) {
    //stlmbr(proj_grid, tan_lat, lon1);
  if (strstr(grid_type, "olar"))  // polar stereographic
	{
		stlmbr(proj_grid, 90.0, 0);
		stcm1p(proj_grid, x1, y1, lat1, lon1, 60, lonv, delx, 0);
	} else if (strstr(grid_type, "ambert")) // lambert conformal
	{
    stlmbr(proj_grid, eqvlat(latin1, latin2), lonv);
    stcm1p(proj_grid, x1, y1, lat1, lon1, latin1, lonv, delx, 0);
  } else {
		std::cout << "Unknown grid_type" << grid_type << std::endl;
		exit(0);
	}

//  else {
//    std::cerr << "ERROR: failed to initialize a projection grid.\n";
//    return FG_ERROR;
//    }
  
  configured = true;
  return FG_OK;
  }
////////////////////////////////////////////////////////////////////////////

// redirection for string -> char*
////////////////////////////////////////////////////////////////////////////
int init_grid(std::string s, maparam *proj_grid) {
  // get only the first item in the colon-delimited list, which is the first
  // file name
  unsigned int loc = s.find_first_of(":");
  if (loc != (int)std::string::npos) s.erase(loc);
  
  int ret;  // return code
  int len = s.length();
  char *p = new char[len+1];
  s.copy(p, len, 0);
  p[len]=0;
  ret = init_grid(p, proj_grid);
  delete[] p;
  return ret;
  }

  
