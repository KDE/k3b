/***************************************************************************
                          k3bdvdavset.h  -  description
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

#ifndef K3BDVDAVSET_H
#define K3BDVDAVSET_H

#include "k3bdivxdatagui.h"

class KComboBox;
class QRadioButton;
class K3bDvdCodecData;
class K3bDivXDataGui;
/**
  *@author Sebastian Trueg
  */

class K3bDvdAVSet : public K3bDivXDataGui  {
   Q_OBJECT
public: 
    K3bDvdAVSet( QWidget *parent=0, const char *name=0 );
    ~K3bDvdAVSet();
    void updateData( K3bDvdCodecData *data);
signals:
    void dataChanged( K3bDivXDataGui *);
private:
    KComboBox *m_comboCd;
    KComboBox *m_comboMp3;
    KComboBox *m_comboCodec;

    QRadioButton *m_buttonOnePass;
    QRadioButton *m_buttonTwoPass;

    int m_lengthSecs;
    void setupGui();
};

#endif
