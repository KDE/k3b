/***************************************************************************
                          k3bdvdextraripstatus.h  -  description
                             -------------------
    begin                : Sun Mar 31 2002
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

#ifndef K3BDVDEXTRARIPSTATUS_H
#define K3BDVDEXTRARIPSTATUS_H

#include <qwidget.h>

class QLabel;

/**
  *@author Sebastian Trueg
  */

class K3bDvdExtraRipStatus : public QWidget  {
   Q_OBJECT
public: 
    K3bDvdExtraRipStatus(QWidget *parent=0, const char *name=0);
    ~K3bDvdExtraRipStatus();

public slots:
    void slotDataRate( float );
    void slotEstimatedTime( unsigned int );
private:
    QLabel *m_rate;
    QLabel *m_estimatetime;
    void setupGui();
};

#endif
