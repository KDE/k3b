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

class KListView;
class KActionMenu;
class KAction;
class QLabel;
class QListViewItem;
class QPushButton;
class QGroupBox;


/**
  *@author Sebastian Trueg
  */

class K3bOptionDialog : public KDialogBase
{
   Q_OBJECT

public:
	K3bOptionDialog(QWidget *parent=0, const char *name=0, bool modal = true);
	~K3bOptionDialog();
	
protected slots:
	void slotOk();
	void slotApply();
	void slotDefault();
	
private:
	// programs Tab
    KListView* m_viewPrograms;
    QPushButton* m_buttonSearch;
    QLabel* m_labelInfo;

    // devices Tab
    QLabel* m_labelDevicesInfo;
    KListView* m_viewDevicesReader;
	KListView* m_viewDevicesWriter;
    QGroupBox* m_groupReader;
    QGroupBox* m_groupWriter;
    QPushButton* m_buttonRefreshDevices;
	KActionMenu* m_menuDevices;
	KAction* m_actionNewDevice;
	KAction* m_actionRemoveDevice;

    void setupProgramsPage();
    void readPrograms();
    bool savePrograms();
	
	void setupDevicePage();
	void readDevices();
	void saveDevices();

	bool devicesChanged;
			
private slots:
	void slotRefreshDevices();
	void slotNewDevice();
	void slotRemoveDevice();
	void slotDevicesChanged();
	void slotDevicesPopup( QListViewItem*, const QPoint& );
};

#endif
