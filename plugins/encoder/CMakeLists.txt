

add_subdirectory( sox )
add_subdirectory( external )

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(ENCLAMEDIR)")

if(OGGVORBIS_FOUND)
   add_subdirectory(ogg)
endif(OGGVORBIS_FOUND)

########### install files ###############




#original Makefile.am contents follow:

#if include_LAME
#ENCLAMEDIR = lame
#endif
#
#if include_OGG
#ENCOGGDIR = ogg
#endif
#
#SUBDIRS = sox external $(ENCLAMEDIR) $(ENCOGGDIR)
