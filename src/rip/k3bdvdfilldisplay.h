/***************************************************************************
                          k3bdvdfilldisplay.h  -  description
                             -------------------
    begin                : Sun Mar 24 2002
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

#ifndef K3BDVDFILLDISPLAY_H
#define K3BDVDFILLDISPLAY_H

#include <qframe.h>
class QPainter;
/**
  *@author Sebastian Trueg
  */

class K3bDvdFillDisplay : public QFrame  {
public: 
    K3bDvdFillDisplay(QWidget *parent=0, const char *name=0);
    ~K3bDvdFillDisplay();
    void setKbSize( unsigned long s ){ m_size=s; }
    void setKbAvailable( unsigned long a ){ m_available=a; }
    void setKbUsed( unsigned long u ){ m_used=u; }
    void setKbDvd( unsigned long d ){ m_dvd=d; }
    QString freeWithDvdAvi();
    QString freeWithDvd();

protected:
    void drawContents(QPainter*);
private:
    unsigned long m_size;
    unsigned long m_available;
    unsigned long m_used;
    unsigned long m_dvd;
    float m_aviFull;
    float m_dvdFull;
    void setupGui();

};

#endif
