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

class K3bCddbOptionTab;
class K3bCddbLocalDBTab;
class K3bDeviceOptionTab;
class K3bBurningOptionTab;
class K3bPatternOptionTab;
class K3bExternalBinOptionTab;
class K3bMiscOptionTab;


/**
  *@author Sebastian Trueg
  */
class K3bOptionDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bOptionDialog(QWidget *parent=0, const char *name=0, bool modal = true);
  ~K3bOptionDialog();
	
  enum m_configPageIndex { Burning = 0, Devices = 1, Programs = 2, Cddb = 3 };
		
 protected slots:
  void slotOk();
  void slotApply();
  void slotDefault();
	
 private:
  bool saveSettings();

  // programs tab
  K3bExternalBinOptionTab* m_externalBinOptionTab;
  void setupProgramsPage();

  // device tab
  K3bDeviceOptionTab* m_deviceOptionTab;	
  void setupDevicePage();

  // burning tab
  void setupBurningPage();
  K3bBurningOptionTab* m_burningOptionTab;

  // cddb tabs
  K3bCddbOptionTab *m_cddbOptionTab;
  K3bCddbLocalDBTab *m_cddbLocalTab;
  void setupCddbPage();		

  // pattern tab
  K3bPatternOptionTab *m_patternOptionTab;
  void setupPatternPage();

  // misc options
  K3bMiscOptionTab* m_miscOptionTab;
  void setupMiscPage();
};

#endif
