
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(ARTSDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(ALSADIR)")

########### install files ###############




#original Makefile.am contents follow:

#if include_arts
#ARTSDIR=arts
#endif
#
#if include_ALSA
#ALSADIR=alsa
#endif
#
#SUBDIRS = $(ARTSDIR) $(ALSADIR)
