/***************************************************************************
                          k3bdvdbasetab.h  -  description
                             -------------------
    begin                : Mon Apr 1 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#ifndef K3BDVDBASETAB_H
#define K3BDVDBASETAB_H

#include <qwidget.h>
class K3bDvdDirectories;
class K3bDvdAVSet;
class K3bDvdAVExtend;
class K3bDvdCodecData;
class K3bDvdInfo;
class K3bDivXDataGui;

/**
  *@author Sebastian Trueg
  */

class K3bDvdBaseTab : public QWidget  {
   Q_OBJECT
public: 
    K3bDvdBaseTab( K3bDvdCodecData *data, QWidget *parent=0, const char *name=0);
    ~K3bDvdBaseTab();
private:
    K3bDvdCodecData *m_datas;
    K3bDvdDirectories *m_directories;
    K3bDvdAVSet *m_avsettings;
    K3bDvdAVExtend *m_avextended;
    K3bDvdInfo *m_info;
    void setupGui();
    void updateView();
private slots:
    void slotUpdateData( K3bDivXDataGui *dataGui );
};

#endif
