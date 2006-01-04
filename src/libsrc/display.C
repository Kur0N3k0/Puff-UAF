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
#include <config.h>
#endif

#include <iostream>  // std::cout
#include <iomanip>

#include "Grid.h"

// if we do not have the <sstream> header, substitute the (less powerful)
// 'ostrstream' type for 'ostringstream'
#ifndef HAVE_SSTREAM
#define ostringstream ostrstream
#endif

//
// Display to std::cout
//
void Grid::display(DISPLAY_ENUM displayStyle) {

    unsigned int i, j, k, l;

    if ( displayStyle == INFO ) {
	display_info();
	return;
    }

    // DISPLAY THIS FOR ALL STYLES:
      
    std::cout << std::endl;
    std::cout << '\"' << fgTitle << "\"" << std::endl;
   
    unsigned int title_len = unsigned(strlen(fgTitle));
    for (i=0; i<title_len+2; i++) {
	std::cout << '-';
    }
    std::cout << std::endl;
    
    // MAJOR VARAIBLE:
    std::cout << fgData[VAR].name << '(';
    for (i=1; i<=fgNdims; i++) {
	std::cout << fgData[i].name;
	if (i != fgNdims) std::cout << ',';
    }
    std::cout << ") :" << std::endl;
    
    // BASIC INFO:
    std::cout << "\tunits = \"" << fgData[VAR].units << "\"" << std::endl;
    std::cout << "\tfill_value = " << fgFillValue << std::endl;
    std::cout << "\tvalid_range = " << fgData[VAR].range[0] << " , " 
         << fgData[VAR].range[1] << std::endl;
    
    std::cout << std::endl;
    std::cout << "\tReference Time = " << '\"' << fgReftime << "\"" << std::endl;
    
    // DIMENSION INFO:
    std::cout << std::endl;
    for (i=1; i<=fgNdims; i++) {
        std::cout << '\t' << fgData[i].name
             << '\t' << fgData[i].size
             << "\t\"" << fgData[i].units << "\"" << std::endl;
    }
    std::cout << std::endl;
    
    // RETURN HERE IF DATA HAS NOT BEEN ALLOCATED:
    if ( fgData[VAR].size == 0 )  {
	std::cout << "\n>> Data for this object has not been allocated << \n";
	std::cout << std::endl;
	return;
    }
    
    // RETURN IF QUICK:
    if (displayStyle == QUICK) {
	std::cout << std::endl;
	return;
    }
	
    // BASIC STATS:
    if (displayStyle != FULL_NOSTATS && displayStyle != SIMPLE_NOSTATS
        && displayStyle != COLUMN_NOSTATS) {
	std::cout << "\tminimum  = " << this->min() << std::endl;
	std::cout << "\tmaximum  = " << this->max() << std::endl;
	std::cout << "\tmean     = " << this->mean() << std::endl;
	std::cout << "\tvariance = " << this->variance() << std::endl;
	std::cout << "\tpct_bad  = " << this->pct_bad() << " %" << std::endl;
	std::cout << '\n';
    }
    
    // RETURN IF STATS ONLY:
    if ( displayStyle == QUICK_STATS ) {
	std::cout << std::endl;
	return;
    }
    
    // DATA:
    std::cout << "Data:" << std::endl;
    
    // SET std::cout FIELDS:
    if ( fgLeftAdjust ) {
	std::cout.setf(std::ios::left);
    }
    else {
	std::cout.setf(std::ios::right, std::ios::adjustfield);
    }
    
    if (fgPrecision > 0) {
	std::cout.setf(std::ios::fixed);
	std::cout.precision(fgPrecision);
    }
	
    // COLUMN ASCII STYLE:
    // VARIABLE DATA:
    if ( displayStyle == COLUMN || displayStyle == COLUMN_NOSTATS ) {

	// WRITE NAME:
	for (i=1; i<=fgNdims; i++) {
	    if (fgWidth > 0) std::cout.width(fgWidth-1);
	    std::cout << fgData[i].name << ": ";
	}

	if (fgWidth > 0) std::cout.width(fgWidth-1);
	std::cout << fgData[VAR].name << ":\n\n";
	
	// WRITE EACH DIMENSIONAL CASE:
	switch (fgNdims) {
	    case (1) : {
		for (i=0; i<fgData[FRTIME].size; i++) {
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[FRTIME].val[i] << fgSpace;
		    
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[VAR].val[i] << std::endl;
		}
		break;
	    }
	    case (2) : {
		for (i=0; i<fgData[FRTIME].size; i++) {
		for (j=0; j<fgData[LEVEL].size; j++) {
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[FRTIME].val[i] << fgSpace;
		    
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[LEVEL].val[j] << fgSpace;
		    
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[VAR].val[offset(i, j)] << std::endl;
		}}
		break;
	    }
	    case (3) : {
		for (i=0; i<fgData[FRTIME].size; i++) {
		for (j=0; j<fgData[LEVEL].size; j++) {
		for (k=0; k<fgData[LAT].size; k++) {
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[FRTIME].val[i] << fgSpace;
		    
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[LEVEL].val[j] << fgSpace;
		    
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[LAT].val[k] << fgSpace;
		    
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[VAR].val[offset(i, j, k)] << std::endl;
		}}}
		break;
	    }
	    case (4) : {
		for (i=0; i<fgData[FRTIME].size; i++) {
		for (j=0; j<fgData[LEVEL].size; j++) {
		for (k=0; k<fgData[LAT].size; k++) {
		for (l=0; l<fgData[LON].size; l++) {
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[FRTIME].val[i] << fgSpace;
		    
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[LEVEL].val[j] << fgSpace;
		    
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[LAT].val[k] << fgSpace;
		    
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[LON].val[l] << fgSpace;
		    
		    if (fgWidth > 0) std::cout.width(fgWidth);
		    std::cout << fgData[VAR].val[offset(i, j, k, l)] << std::endl;
		}}}}
		break;
	    }
	    default : { break; }
	}
	
	// FLUSH AND RETURN:
	std::cout << std::endl;
	return;
    } // END COLUMN STYLE
    
    
    std::ostringstream *tmpStr;
    std::ostringstream *tmpWrd;
    int cmax = 78;
    
    // WRITE DIMENSION DATA:
    for (i=1; i<=fgNdims; i++) {
	tmpWrd = new std::ostringstream;
	tmpStr = new std::ostringstream;
	std::cout << std::endl;
	std::cout << fgData[i].name << " = " << std::endl;
	for (j=0; j<fgData[i].size; j++) {
	    
	    // SET FIELDS:
	    if (fgWidth > 0) tmpWrd->width(fgWidth);
	    
	    if ( fgLeftAdjust ) {
		tmpWrd->setf(std::ios::left);
	    }
	    else {
		tmpWrd->setf(std::ios::right, std::ios::adjustfield);
	    }
    
	    if (fgPrecision > 0) {
		tmpWrd->setf(std::ios::fixed);
		tmpWrd->precision(fgPrecision);
	    }
	    
	    // INSERT:
	    *tmpWrd << fgData[i].val[j];
	    if (j != (fgData[i].size - 1) ) *tmpWrd << fgSpace;
	    
	    // COUNT:
	    if ( (tmpWrd->width() + tmpStr->width()) < cmax ) {
	        *tmpWrd << std::ends;
		*tmpStr << tmpWrd->str();
		delete tmpWrd;
		tmpWrd = new std::ostringstream;
	    }
	    else {
	        *tmpStr << std::ends;
		std::cout << tmpStr->str();
		if ( strchr(fgSpace, '\n') == NULL ) std::cout << std::endl;
		delete tmpStr;
		tmpStr = new std::ostringstream;
	    }
	}
	*tmpWrd << std::ends;
	*tmpStr << tmpWrd->str();
	*tmpStr << std::ends;
	std::cout << tmpStr->str() << std::endl;
	delete tmpStr;
	delete tmpWrd;
    }
    
    if (displayStyle == SIMPLE || displayStyle == SIMPLE_NOSTATS) {
	std::cout << std::endl;
	return;
    }
    
    
    tmpWrd = new std::ostringstream;
    tmpStr = new std::ostringstream;
    
    // FULL STYLE IS ALL THAT IS LEFT:
    // VARIABLE DATA:
    std::cout << std::endl;
    std::cout << fgData[VAR].name << " = " << std::endl;

	// WRITE EACH DIMENSIONAL CASE:
	switch (fgNdims) {
	    case (1) : {
		for (i=0; i<fgData[FRTIME].size; i++) {
		
		    // SET FIELDS:
		    if (fgWidth > 0) tmpWrd->width(fgWidth);
		    
		    if ( fgLeftAdjust ) {
			tmpWrd->setf(std::ios::left);
		    }
		    else {
			tmpWrd->setf(std::ios::right, std::ios::adjustfield);
		    }
    
		    if (fgPrecision > 0) {
			tmpWrd->setf(std::ios::fixed);
			tmpWrd->precision(fgPrecision);
		    }
		    
		    // INSERT:
		    *tmpWrd << fgData[VAR].val[i];
		    if (i != (fgData[FRTIME].size - 1) ) *tmpWrd << fgSpace;
		    
		    // COUNT:
		    if ((tmpWrd->width() + tmpStr->width()) < cmax) {
		        *tmpWrd << std::ends;
			*tmpStr << tmpWrd->str();
			delete tmpWrd;
			tmpWrd = new std::ostringstream;
		    }
		    else {
		        *tmpStr << std::ends;
			std::cout << tmpStr->str();
			if ( strchr(fgSpace, '\n') == NULL ) std::cout << std::endl;
			delete tmpStr;
			tmpStr = new std::ostringstream;
		    }
		}
		*tmpWrd << std::ends;
		*tmpStr << tmpWrd->str();
		*tmpStr << std::ends;
		std::cout << tmpStr->str() << std::endl;
		delete tmpStr;
		delete tmpWrd;
		break;
	    }
	    case (2) : {
		for (i=0; i<fgData[FRTIME].size; i++) {
		    std::cout << "i=" << i << "; j:" << std::endl;
		    for (j=0; j<fgData[LEVEL].size; j++) {
		    
			// SET FIELDS:
			if (fgWidth > 0) tmpWrd->width(fgWidth);
			
			if ( fgLeftAdjust ) {
			    tmpWrd->setf(std::ios::left);
			}
			else {
			    tmpWrd->setf(std::ios::right, std::ios::adjustfield);
			}
    
			if (fgPrecision > 0) {
			    tmpWrd->setf(std::ios::fixed);
			    tmpWrd->precision(fgPrecision);
			}
			
			// INSERT:
			*tmpWrd << fgData[VAR].val[offset(i, j)];
			if (j != (fgData[LEVEL].size - 1) ) *tmpWrd << fgSpace;
			
			// COUNT:
			if ((tmpWrd->width() + tmpStr->width()) < cmax) {
			    *tmpWrd << std::ends;
			    *tmpStr << tmpWrd->str();
			    delete tmpWrd;
			    tmpWrd = new std::ostringstream;
			}
			else {
			    *tmpStr << std::ends;
			    std::cout << tmpStr->str();
			    if ( strchr(fgSpace, '\n') == NULL ) std::cout << std::endl;
			    delete tmpStr;
			    tmpStr = new std::ostringstream;
			}
		    }
		    *tmpWrd << std::ends;
		    *tmpStr << tmpWrd->str();
		    delete tmpWrd;
		    tmpWrd = new std::ostringstream;
		    *tmpStr << std::ends;
		    std::cout << tmpStr->str() << std::endl;
		    delete tmpStr;
		    tmpStr = new std::ostringstream;
		}
		*tmpWrd << std::ends;
		*tmpStr << tmpWrd->str();
		delete tmpWrd;
		*tmpStr << std::ends;
		std::cout << tmpStr->str() << std::endl;
		delete tmpStr;
		break;
	    }
	    case (3) : {
		for (i=0; i<fgData[FRTIME].size; i++) {
		for (j=0; j<fgData[LEVEL].size; j++) {
		    std::cout << "i=" << i << "; j=" << j << "; k:" << std::endl;
		    for (k=0; k<fgData[LAT].size; k++) {
		    
			// SET FIELDS:
			if (fgWidth > 0) tmpWrd->width(fgWidth);
			
			if ( fgLeftAdjust ) {
			    tmpWrd->setf(std::ios::left);
			}
			else {
			    tmpWrd->setf(std::ios::right, std::ios::adjustfield);
			}
    
			if (fgPrecision > 0) {
			    tmpWrd->setf(std::ios::fixed);
			    tmpWrd->precision(fgPrecision);
			}
			
			// INSERT:
			*tmpWrd << fgData[VAR].val[offset(i, j, k)];
			if ( k != (fgData[LAT].size -1) ) *tmpWrd << fgSpace;
			
			// COUNT:
			if ((tmpWrd->width() + tmpStr->width()) < cmax) {
			    *tmpWrd << std::ends;
			    *tmpStr << tmpWrd->str();
			    delete tmpWrd;
			    tmpWrd = new std::ostringstream;
			}
			else {
			    *tmpStr << std::ends;
			    std::cout << tmpStr->str();
			    if ( strchr(fgSpace, '\n') == NULL ) std::cout << std::endl;
			    delete tmpStr;
			    tmpStr = new std::ostringstream;
			}
		    }
		    *tmpWrd << std::ends;
		    *tmpStr << tmpWrd->str();
		    delete tmpWrd;
		    tmpWrd = new std::ostringstream;
		    *tmpStr << std::ends;
		    std::cout << tmpStr->str() << std::endl;
		    delete tmpStr;
		    tmpStr = new std::ostringstream;
		}}
		*tmpWrd << std::ends;
		*tmpStr << tmpWrd->str();
		delete tmpWrd;
		*tmpStr << std::ends;
		std::cout << tmpStr->str() << std::endl;
		delete tmpStr;
		break;
	    }
	    case (4) : {
		for (i=0; i<fgData[FRTIME].size; i++) {
		for (j=0; j<fgData[LEVEL].size; j++) {
		for (k=0; k<fgData[LAT].size; k++) {
		    std::cout << "i=" << i << "; j=" << j << "; k=" 
			 << k << "; l:" << std::endl;
		    for (l=0; l<fgData[LON].size; l++) {

			// SET FIELDS:
			if (fgWidth > 0) tmpWrd->width(fgWidth);
			
			if ( fgLeftAdjust ) {
			    tmpWrd->setf(std::ios::left);
			}
			else {
			    tmpWrd->setf(std::ios::right, std::ios::adjustfield);
			}
    
			if (fgPrecision > 0) {
			    tmpWrd->setf(std::ios::fixed);
			    tmpWrd->precision(fgPrecision);
			}
			
			// INSERT:
			*tmpWrd << fgData[VAR].val[offset(i, j, k, l)];
			if ( l != (fgData[LON].size - 1) ) *tmpWrd << fgSpace;
			
			// COUNT:
			if ((tmpWrd->width() + tmpStr->width()) < cmax) {
			    *tmpWrd << std::ends;
			    *tmpStr << tmpWrd->str();
			    delete tmpWrd;
			    tmpWrd = new std::ostringstream;
			}
			else {
			    *tmpStr << std::ends;
			    std::cout << tmpStr->str();
			    if ( strchr(fgSpace, '\n') == NULL ) std::cout << std::endl;
			    delete tmpStr;
			    tmpStr = new std::ostringstream;
			}
		    }
		    *tmpWrd << std::ends;
		    *tmpStr << tmpWrd->str();
		    delete tmpWrd;
		    tmpWrd = new std::ostringstream;
		    *tmpStr << std::ends;
		    std::cout << tmpStr->str() << std::endl;
		    delete tmpStr;
		    tmpStr = new std::ostringstream;
		}}}
		*tmpWrd << std::ends;
		*tmpStr << tmpWrd->str();
		delete tmpWrd;
		*tmpStr << std::ends;
		std::cout << tmpStr->str() << std::endl;
		delete tmpStr;
		break;
	    }
	    default : {
		break;
	    }
	}

    // FLUSH AND RETURN:
    std::cout << std::endl;
    return;

}


/////////////////////////////////////////////////////////////////
//
// INFO STYLE DISPLAY TO std::cout:
//
/////////////////////////////////////////////////////////////////
void Grid::display_info() {

    unsigned i;

    std::cout << std::endl;

    int hdwidth = 13;

    // Title:
    std::cout << std::setw(hdwidth) << "Title: " << fgTitle << std::endl;
    std::cout << std::endl;

    // Variable:
    std::cout << std::setw(hdwidth) << "Variable: " << fgData[VAR].name << '(';
    for (i=1; i<=fgNdims; i++) {
	std::cout << fgData[i].name;
	if (i != fgNdims) std::cout << ',';
    }
    std::cout << ')' << std::endl;

    // Units:
    std::cout << std::setw(hdwidth) << "Units: " << fgData[VAR].units << std::endl;
    
    // Reftime:
    std::cout << std::endl;
    std::cout << std::setw(hdwidth) << "Reference: " << '\"' << fgReftime << '\"' << std::endl;
    std::cout << std::endl;

    // Data:
    std::ostringstream *dimstr;

    // Get max name size:
    int length;
    int maxlength = -1;
    for (i=1; i<=fgNdims; i++) {
	length = strlen(fgData[i].name);
	if ( length > maxlength ) {
	    maxlength = length;
	}
    }
    
    int dimwidth = maxlength + 2;

    // Get max size size:
    maxlength = -1;
    for (i=1; i<=fgNdims; i++) {
	dimstr = new std::ostringstream;
	*dimstr << fgData[i].size << std::ends;
	length = dimstr->width();
	if (  length > maxlength ) {
	    maxlength = length;
	}
	delete dimstr;
    }


    int szwidth = maxlength + 2;

    // Get max range size:
    maxlength = -1;
    for (i=1; i<=fgNdims; i++) {
	dimstr = new std::ostringstream;
	*dimstr << fgData[i].val[0] << std::ends;
	length = dimstr->width();
	if (  length > maxlength ) {
	    maxlength = length;
	}
	delete dimstr;
    }

    int rngwidth = maxlength + 4;

    // Write Data:
    for (i=1; i<=fgNdims; i++) {
	dimstr = new std::ostringstream;
	*dimstr << "Dim " << i << ": " << std::ends;

	std::cout << std::setw(hdwidth) << dimstr->str() 
	     << std::setw(dimwidth) << fgData[i].name << ' ' 
	     << std::setw(szwidth) << fgData[i].size << ' ' 
	     << std::setw(rngwidth) << fgData[i].val[0] 
		        << ':' << fgData[i].val[fgData[i].size-1]
	     << std::endl;

	delete dimstr;
    }
    

    // STATS:

    
    int statwidth = 12;

    dimstr = new std::ostringstream;
    *dimstr << fgData[VAR].range[0] << " , " << fgData[VAR].range[1]
	    << std::ends;

    length = dimstr->width();
    if ( length > statwidth) {
	statwidth = length + 2;
    }

    std::cout << std::endl;
    std::cout << std::setw(hdwidth) << "fill value: "
	 << std::setw(statwidth) << fgFillValue << std::endl;
    std::cout << std::setw(hdwidth) << "valid range: " 
	 << std::setw(statwidth) << dimstr->str() << std::endl;

    std::cout << std::setw(hdwidth) << "minimum: " 
         << std::setw(statwidth) << this->min() << std::endl;
    std::cout << std::setw(hdwidth) << "maximum: " 
	 << std::setw(statwidth) << this->max() << std::endl;
    std::cout << std::setw(hdwidth) << "mean: " 
	 << std::setw(statwidth) << this->mean() << std::endl;
    std::cout << std::setw(hdwidth) << "variance: " 
	 << std::setw(statwidth) << this->variance() << std::endl;
    std::cout << std::setw(hdwidth) << "pct bad: " 
	 << std::setw(statwidth) << this->pct_bad() << std::endl;


    std::cout << std::endl;
    return;
}
