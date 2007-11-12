
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

add_subdirectory( 73lab )
add_subdirectory( crystal )
add_subdirectory( quant )
add_subdirectory( RobsTheme )


########### install files ###############




#original Makefile.am contents follow:

#SUBDIRS = 73lab crystal quant RobsTheme
