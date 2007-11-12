
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

add_subdirectory( decoder )
add_subdirectory( encoder )
add_subdirectory( audiooutput )
add_subdirectory( project )


########### install files ###############




#original Makefile.am contents follow:

#SUBDIRS = decoder encoder audiooutput project
