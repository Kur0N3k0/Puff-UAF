# list ashxp after libsrc for conformal mapping library (dmapf) is built first
SUBDIRS = libsrc perl ashxp 

# netCDF c++ library is necessary for these executables
NETCDF_CXX_LIB=-lnetcdf_c++

if PUFF_NEED_GETOPT_LONG
  PUFF_GETOPT=my_getopt.c
else
  PUFF_GETOPT=
endif

AM_CPPFLAGS = -I./libsrc

#bin_PROGRAMS = puff uni2puff ashdump pp2nc
bin_PROGRAMS = puff ashdump pp2nc

puff_SOURCES = atmosphere.C dem.C particle.C puff.C cloud.C ran_utils.C ash.C \
puff_utils.C rcfile.C volc_utils.C planes.C puff_options.C ${PUFF_GETOPT}

if PUFF_NEED_LIBDMAPF
LIBDMAPF = libsrc/dmapf-c/libdmapf.a
else
LIBDMAPF =
endif

puff_LDADD = $(NETCDF_CXX_LIB) libsrc/libpuff.la $(LIBDMAPF)

# uni2puff_SOURCES = uni2puff.C uni2puff_options.C $(PUFF_GETOPT)
# uni2puff_LDADD = libsrc/libpuff.la $(LIBDMAPF)

ashdump_SOURCES = ashdump.C ashdump_options.C ash.C particle.C ran_utils.C cloud.C planes.C ${PUFF_GETOPT}

#ashdump_LDADD = libsrc/libpuff.la $(LIBDMAPF)
ashdump_LDADD = $(NETCDF_CXX_LIB)  $(LIBDMAPF) libsrc/utils.o

pp2nc_SOURCES = pp2nc.C 

pp2nc_LDADD = $(NETCDF_CXX_LIB)

HEADER_SRC = ash.h ashdump_options.h atmosphere.h dem.h cloud.h particle.h \
planes.h puff.h puff_options.h ran_utils.h rcfile.h volcsites.h volc_utils.h uni2puff_options.h

EXTRA_DIST = $(HEADER_SRC) volcsites.args volcanos.txt my_getopt.c my_getopt.h
