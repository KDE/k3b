/***************************************************************************
                          k3bdvdavextend.h  -  description
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

#ifndef K3BDVDAVEXTEND_H
#define K3BDVDAVEXTEND_H

#include "k3bdivxdatagui.h"

class QWidget;
class QCheckBox;
class QSlider;
class KLineEdit;
class KComboBox;

/**
  *@author Sebastian Trueg
  */

class K3bDvdAVExtend : public K3bDivXDataGui  {
   Q_OBJECT
public: 
    K3bDvdAVExtend(QWidget *parent=0, const char *name=0);
    ~K3bDvdAVExtend();
void updateData( K3bDvdCodecData *);
private:
    QCheckBox *m_checkResample;
    KLineEdit *m_editKeyframes;
    QCheckBox *m_checkYuv;
    KComboBox *m_comboDeinterlace;
    KComboBox *m_comboLanguage;
    QSlider *m_sliderCrispness;
    KLineEdit *m_editAudioGain;

    void setupGui();
};

#endif
