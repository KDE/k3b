/***************************************************************************
                          k3bdvdinfo.h  -  description
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

#ifndef K3BDVDINFO_H
#define K3BDVDINFO_H

#include <qwidget.h>
#include <qgroupbox.h>
class QLabel;
class QGridLayout;

/**
  *@author Sebastian Trueg
  */

class K3bDvdInfo : public QGroupBox  {

public:
    K3bDvdInfo(QWidget *parent=0, const char *name=0);
    ~K3bDvdInfo();
protected:
    QGridLayout *m_mainLayout;
    void setupGui();
private:
    QLabel *m_length;
    QLabel *m_frames;
    QLabel *m_size;
    QLabel *m_aspect;


};

#endif
