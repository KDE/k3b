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
class QCheckBox;
class QRadioButton;
class QHButtonGroup;
class QLabel;
class QString;
class K3bDivxCodecData;
class K3bDivXTcprobeAc3;
class QSpinBox;
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

public slots:
    void slotViewAc3Bitrate();

private:
    KComboBox *m_comboCd;
    KComboBox *m_comboMp3;
    KComboBox *m_comboCodec;
    QLabel *m_vBitrate;
    QLabel *m_aAC3Bitrate;
    QRadioButton *m_buttonOnePass;
    QRadioButton *m_buttonTwoPass;
    QRadioButton *m_buttonCbr;
    QRadioButton *m_buttonVbr;
    QCheckBox *m_checkAc3Passthrough;
    QHButtonGroup *m_mp3modeGroup;
    K3bDivxCodecData *m_data;
    K3bDivXTcprobeAc3 *m_parser;
    QSpinBox *m_vBitrateCustom;
    int m_lengthSecs;
    bool m_fixedCDSize;
    void setupGui();

private slots:
    void slotCalcBitrate();
    void slotCodecChanged( int );
    void slotModeChanged( int );
    void slotMp3ModeChanged( int );
    void slotAc3Passthrough( int );
    void slotAc3Scaned();
    void slotCustomBitrate( int );
    void slotCDSize();
};

#endif
