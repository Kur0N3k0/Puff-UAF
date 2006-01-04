if ASHXP_BUILD_LIBJPEG
noinst_LIBRARIES = libjpeg.a
endif

libjpeg_a_SOURCES = jcapimin.c jcapistd.c jctrans.c jcparam.c jdatadst.c \
jcinit.c jcmaster.c jcmarker.c jcmainct.c jcprepct.c jccoefct.c jccolor.c \
jcsample.c jchuff.c jcphuff.c jcdctmgr.c jfdctfst.c jfdctflt.c jfdctint.c \
jdapimin.c jdapistd.c jdtrans.c jdatasrc.c jdmaster.c jdinput.c jdmarker.c \
jdhuff.c jdphuff.c jdmainct.c jdcoefct.c jdpostct.c jddctmgr.c jidctfst.c \
jidctflt.c jidctint.c jidctred.c jdsample.c jdcolor.c jquant1.c jquant2.c \
jdmerge.c jcomapi.c jutils.c jerror.c jmemmgr.c jmemnobs.c \
$(HEADER_SRC) $(CFG_FILES)

HEADER_SRC = cderror.h jchuff.h jdct.h jerror.h jmemsys.h jpegint.h \
jversion.h cdjpeg.h jconfig.h jdhuff.h jinclude.h jmorecfg.h jpeglib.h \
 transupp.h 

CFG_FILES = jconfig.cfg makefile.cfg
