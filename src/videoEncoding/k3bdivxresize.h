/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BDVDRESIZE_H
#define K3BDVDRESIZE_H

#include <qwidget.h>
#include <qgroupbox.h>

class QSlider;
class QLabel;
class KComboBox;
class K3bDivxCodecData;

class K3bDivxResize : public QGroupBox  {
   Q_OBJECT
public:
    K3bDivxResize(K3bDivxCodecData *data, QWidget *parent=0, const char *name=0);
    ~K3bDivxResize();
    void initView();
    void resetView();
public slots:
    void slotUpdateView();

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
    float m_realAspect; // estimated aspect after cropping
    K3bDivxCodecData *m_data;
    void setupGui();
};

#endif
