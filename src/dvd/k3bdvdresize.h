/***************************************************************************
                          k3bdvdresize.h  -  description
                             -------------------
    begin                : Sat Apr 6 2002
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

#ifndef K3BDVDRESIZE_H
#define K3BDVDRESIZE_H

#include <qwidget.h>
#include <qgroupbox.h>

class QSlider;
class QLabel;
class KComboBox;

/**
  *@author Sebastian Trueg
  */

class K3bDvdResize : public QGroupBox  {
   Q_OBJECT
public: 
    K3bDvdResize(QWidget *parent=0, const char *name=0);
    ~K3bDvdResize();
private slots:

private:
    QSlider *m_sliderResize;
    KComboBox *m_comboHeight;
    QLabel *m_labelAspectRatio;
    QLabel *m_labelAspectError;
    QLabel *m_labelWidth;

    void setupGui();
};

#endif
