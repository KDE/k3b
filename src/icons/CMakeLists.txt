
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

add_subdirectory( actions )


########### install files ###############


kde4_install_icons( ${ICON_INSTALL_DIR} )



#original Makefile.am contents follow:

#KDE_ICON = AUTO
#SUBDIRS = actions
