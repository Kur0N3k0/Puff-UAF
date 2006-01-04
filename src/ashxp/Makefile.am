SUBDIRS = jpeg-6b

if PUFF_NEED_GETOPT_LONG
  PUFF_GETOPT=my_getopt.c
else
	PUFF_GETOPT=
endif

if PUFF_NEED_LIBDMAPF
LIBDMAPF = ../libsrc/dmapf-c/libdmapf.a
else
LIBDMAPF =
endif

if ASHXP_BUILD_LIBJPEG
LIBJPEG = jpeg-6b/libjpeg.a
else
LIBJPEG =
endif

ashxp_LDADD = $(LIBDMAPF) $(LIBJPEG)

bin_PROGRAMS = ashxp
ashxp_SOURCES = ashxp.c color.c fileio.c make_jpeg.c readash.c arg_util.c \
${PUFF_GETOPT} ashxp.h  color.h make_jpeg.h numbers.dat projection.c \
projection.h

EXTRA_DIST = my_getopt.c my_getopt.h \
numbers.dat \
$(IMAGE1) $(IMAGE2) $(IMAGE3) $(FONTFILE)

IMAGE1=images/GLOBALeb10colshade.jpg
IMAGE2=images/GLOBALeb3colshade.jpg
IMAGE3=images/GLOBALeb6colshadesmall.jpg
FONTFILE=fonts/FreeMonoBold.ttf

install-data-local:
	$(mkinstalldirs) $(prefix)/etc/images
	$(mkinstalldirs) $(prefix)/etc/fonts
	$(INSTALL_DATA) $(IMAGE1) $(prefix)/etc/$(IMAGE1)
	$(INSTALL_DATA) $(IMAGE2) $(prefix)/etc/$(IMAGE2)
	$(INSTALL_DATA) $(IMAGE3) $(prefix)/etc/$(IMAGE3)
	$(INSTALL_DATA) $(FONTFILE) $(prefix)/etc/$(FONTFILE)

