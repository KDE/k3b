/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


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

class K3bDivxAVSet : public QGroupBox  {
   Q_OBJECT
public:
    K3bDivxAVSet( K3bDivxCodecData *data, QWidget *parent=0, const char *name=0 );
    ~K3bDivxAVSet();
    void updateView();
    void init();
    static void initGuiFactoryCodec( KComboBox &box );

public slots:
    void slotViewAc3Bitrate();

private:
    KComboBox *m_comboCd;
    KComboBox *m_comboMp3;
    KComboBox *m_comboCodec;
    QLabel *m_vBitrate;
    QLabel *m_aAC3Bitrate;
    QLabel *m_mp3bitrate;
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
