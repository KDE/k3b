
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install( FILES  k3bphotovcd.mpg k3bphotosvcd.mpg DESTINATION ${DATA_INSTALL_DIR}/k3b/extra )



#original Makefile.am contents follow:

#
#extradir = $(kde_datadir)/k3b/extra
#extra_DATA = k3bphotovcd.mpg k3bphotosvcd.mpg
#
#EXTRA_DIST = $(extra_DATA)
