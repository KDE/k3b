/***************************************************************************
                          k3bdivxextsettings.h  -  description
                             -------------------
    begin                : Tue Jul 30 2002
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

#ifndef K3BDIVXEXTSETTINGS_H
#define K3BDIVXEXTSETTINGS_H

#include <qgroupbox.h>

class K3bDivxCodecData;
class QCheckBox;
class KLineEdit;

/**
  *@author Sebastian Trueg
  */

class K3bDivxExtSettings : public QGroupBox  {
   Q_OBJECT
public: 
    K3bDivxExtSettings(K3bDivxCodecData *data, QWidget *parent=0, const char *name=0);
    ~K3bDivxExtSettings();
public slots:
    void slotUpdateView();
private:
    K3bDivxCodecData *m_data;
    QCheckBox *m_checkShutdown;
    QCheckBox *m_checkWithoutAudio;
    QCheckBox *m_checkOnlySecondPass;
    QCheckBox *m_checkOnlyFirstPass;
    KLineEdit *m_lineTwoPassLog; 

    void setupGui();
private slots:
    void slotNoAudio( int );
    void slotShutdown( int );
};

#endif
