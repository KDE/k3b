

add_subdirectory( sox )
add_subdirectory( external )

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(ENCLAMEDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(ENCOGGDIR)")

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
