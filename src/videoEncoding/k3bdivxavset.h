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

#ifndef K3BDIVXAVSET_H
#define K3BDIVXAVSET_H

#include <qgroupbox.h>

class KComboBox;
class QRadioButton;
class QLabel;
class QString;
class K3bDivxCodecData;
/**
  *@author Sebastian Trueg
  */

class K3bDivxAVSet : public QGroupBox  {
   Q_OBJECT
public:
    K3bDivxAVSet( K3bDivxCodecData *data, QWidget *parent=0, const char *name=0 );
    ~K3bDivxAVSet();
    void updateView();
    void init();
signals:
    void dataChanged();
private:
    KComboBox *m_comboCd;
    KComboBox *m_comboMp3;
    KComboBox *m_comboCodec;
    QLabel *m_vBitrate;
    QString m_vBitrateDesc;
    QRadioButton *m_buttonOnePass;
    QRadioButton *m_buttonTwoPass;

    K3bDivxCodecData *m_data;
    int m_lengthSecs;
    void setupGui();
private slots:
    void slotCalcBitrate();
    void slotCodecChanged( int );
    void slotModeChanged( int );
};

#endif
