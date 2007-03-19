AM_CPPFLAGS= 	-I$(srcdir)/../../libk3b/core \
		-I$(srcdir)/../../libk3bdevice \
		-I$(srcdir)/../../libk3b/projects \
		-I$(srcdir)/../../libk3b/projects/audiocd \
		-I$(srcdir)/../../libk3b/projects/datacd \
		-I$(srcdir)/../../libk3b/projects/mixedcd \
		-I$(srcdir)/../../libk3b/projects/movixcd \
		-I$(srcdir)/../../libk3b/projects/datadvd \
		-I$(srcdir)/../../libk3b/projects/videocd \
		-I$(srcdir)/../../libk3b/projects/videodvd \
		-I$(srcdir)/../../libk3b/projects/movixdvd \
		-I$(srcdir)/../../libk3b/cddb \
		-I$(srcdir)/../../libk3b/tools \
		-I$(srcdir)/../../libk3b/plugin \
		-I$(srcdir)/../rip \
		-I$(srcdir)/.. \
		$(all_includes)

METASOURCES = AUTO

SUBDIRS = kostore

noinst_LTLIBRARIES = libprojects.la

libprojects_la_LIBADD = ../rip/librip.la ./kostore/libkostore.la

libprojects_la_SOURCES = k3baudioburndialog.cpp \
			 k3baudiocdtextwidget.cpp \
			 k3baudiodatasourceviewitem.cpp \
			 k3baudioeditorwidget.cpp \
			 k3baudiotrackdialog.cpp \
			 k3baudiotrackwidget.cpp \
			 k3baudiotrackplayer.cpp \
			 k3baudiotracksplitdialog.cpp \
			 k3baudiotrackview.cpp \
			 k3baudiotrackviewitem.cpp \
			 k3baudioview.cpp \
			 base_k3baudiocdtextwidget.ui \
			 base_k3baudiocdtextallfieldswidget.ui \
	 		 base_k3baudiotrackwidget.ui \
			 k3bmixedburndialog.cpp \
			 k3bmixeddirtreeview.cpp \
			 k3bmixedview.cpp \
			 base_k3badvanceddataimagesettings.ui \
			 base_k3bbootimageview.ui \
			 base_k3bdataimagesettings.ui \
			 base_k3bdatavolumedescwidget.ui \
			 k3bdataadvancedimagesettingswidget.cpp \
			 k3bdatadirtreeview.cpp \
			 k3bdatafileview.cpp \
			 k3bdataimagesettingswidget.cpp \
			 k3bdatapropertiesdialog.cpp \
			 k3bdataview.cpp \
			 k3bdataviewitem.cpp \
			 k3bdatavolumedescwidget.cpp \
			 k3bdataburndialog.cpp \
			 k3bbootimagedialog.cpp \
			 k3bbootimageview.cpp \
			 k3bdvdburndialog.cpp \
			 k3bdvdview.cpp \
			 base_k3bmovixoptionswidget.ui \
			 k3bmovixburndialog.cpp \
			 k3bmovixlistview.cpp \
			 k3bmovixoptionswidget.cpp \
			 k3bmovixview.cpp \
			 k3bmovixdvdburndialog.cpp \
			 k3bmovixdvdview.cpp \
			 k3bvideodvdburndialog.cpp \
			 k3bvideodvdview.cpp \
			 k3bvcdburndialog.cpp \
			 k3bvcdlistview.cpp \
			 k3bvcdlistviewitem.cpp \
			 k3bvcdtrackdialog.cpp \
			 k3bvcdview.cpp \
			 k3bfillstatusdisplay.cpp \
			 k3bview.cpp \
			 k3bprojectburndialog.cpp \
			 k3bprojectplugindialog.cpp \
			 k3baudiotracktrmlookupdialog.cpp \
			 k3bdatamultisessioncombobox.cpp \
			 k3bmusicbrainzjob.cpp \
			 k3bdataurladdingdialog.cpp \
			 k3bdatasessionimportdialog.cpp \
			 k3baudiodatasourceeditwidget.cpp \
			 k3baudiotrackaddingdialog.cpp \
			 k3bencodingconverter.cpp