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
class K3bDvdCodecData;
/**
  *@author Sebastian Trueg
  */

class K3bDvdResize : public QGroupBox  {
   Q_OBJECT
public: 
    K3bDvdResize(K3bDvdCodecData *data, QWidget *parent=0, const char *name=0);
    ~K3bDvdResize();
    void initView();
    void updateView();
signals:
    void sizeChanged();
private slots:
    void slotResizeChanged( int );
    void slotHeightChanged( int );
private:
    QSlider *m_sliderResize;
    KComboBox *m_comboHeight;
    QLabel *m_labelAspectRatio;
    QLabel *m_labelAspectError;
    QLabel *m_labelWidth;

    float m_currentAspect;
    float m_orginalAspect;
    K3bDvdCodecData *m_data;
    void setupGui();
};

#endif
