/***************************************************************************
                          k3boptiondialog.h  -  description
                             -------------------
    begin                : Tue Apr 17 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BOPTIONDIALOG_H
#define K3BOPTIONDIALOG_H

#include <kdialogbase.h>
#include <klistview.h>

class KListView;
class QLabel;
class QListViewItem;
class QPushButton;
class QGroupBox;
class QXEmbed;
class QWidgetStack;
class QCheckBox;

class K3bDevice;
class K3bOptionCddb;
class K3bDeviceOptionTab;


/**
  *@author Sebastian Trueg
  */
class K3bOptionDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bOptionDialog(QWidget *parent=0, const char *name=0, bool modal = true);
  ~K3bOptionDialog();
	
  enum m_configPageIndex { Devices = 0, Programs = 1, Cddb = 2 };
		
 protected slots:
  void slotOk();
  void slotApply();
  void slotDefault();
	
 private:
  // programs Tab
  KListView* m_viewPrograms;
  QPushButton* m_buttonSearch;
  QLabel* m_labelInfo;

  // burning tab
  QCheckBox* m_checkUseID3Tag;
		
  void setupProgramsPage();
  void readPrograms();
  bool savePrograms();

  // device tab
  K3bDeviceOptionTab* m_deviceOptionTab;	
  void setupDevicePage();

  void setupBurningPage();
  void readBurningSettings();
  void saveBurningSettings();
	
  // cddb tab
  K3bOptionCddb *m_cddbPage;
  void setupCddbPage();		
};

#endif
