
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############


########### install files ###############




#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir) -I$(srcdir)/.. $(all_includes)
#AM_CCASFLAGS = -I$(srcdir) $(GV_ASM_DEFS)
#
##CXXFLAGS = -fexceptions
#
#noinst_LTLIBRARIES = libfastscale.la
#
#libfastscale_la_SOURCES = \
#	scale.cpp \
#	asm_scale.S
#
#libfastscale_la_LIBADD = $(LIB_KDECORE) $(LIBQT) $(LIBJPEG)
#
#METASOURCES = AUTO
