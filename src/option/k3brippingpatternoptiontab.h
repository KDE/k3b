/***************************************************************************
                          k3brippingpatternoptiontab.h  -  description
                             -------------------
    begin                : Fri Nov 23 2001
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

#ifndef K3BRIPPINGPATTERNOPTIONTAB_H
#define K3BRIPPINGPATTERNOPTIONTAB_H

#include <qwidget.h>

class QCheckBox;
class K3bPatternWidget;

/**
  *@author Sebastian Trueg
  */

class K3bRippingPatternOptionTab : public QWidget  {
   Q_OBJECT
public: 
    K3bRippingPatternOptionTab(QWidget *parent=0, const char *name=0);
    ~K3bRippingPatternOptionTab();
    void init( QString& album );
    void apply();
    void readSettings();
    //QString getFilePattern();
    //QString getDirPattern();
private:
    QCheckBox *m_usePattern;
    K3bPatternWidget *m_frame;

    void setup();
};

#endif
