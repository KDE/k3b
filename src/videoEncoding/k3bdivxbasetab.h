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
class K3bDivxDirectories;
class K3bDivxAVSet;
class K3bDivxAVExtend;
class K3bDivxCodecData;
class K3bDivxInfo;

/**
  *@author Sebastian Trueg
  */

class K3bDivxBaseTab : public QWidget  {
   Q_OBJECT
public:
    K3bDivxBaseTab( K3bDivxCodecData *data, QWidget *parent=0, const char *name=0);
    ~K3bDivxBaseTab();
public slots:
    void slotUpdateView(  );
    void slotInitView(  );
signals:
    void projectLoaded();
private:
    K3bDivxCodecData *m_data;
    K3bDivxDirectories *m_directories;
    K3bDivxAVSet *m_avsettings;
    K3bDivxAVExtend *m_avextended;
    K3bDivxInfo *m_info;
    void setupGui();
};

#endif
