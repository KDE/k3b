
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install( FILES  DESTINATION ${DATA_INSTALL_DIR}/k3b/icons )



#original Makefile.am contents follow:

#k3bicondir = $(kde_datadir)/k3b/icons
#k3bicon_ICON = AUTO
