/***************************************************************************
                          K3bDivxOptionTab.cpp  -  description
                             -------------------
    begin                : Mon Feb 17 2003
    copyright            : (C) 2003 by Sebastian Trueg
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

#include "k3bdivxoptiontab.h"

#include <kconfig.h>
#include <kapp.h>
#include <kcombobox.h>
#include <knuminput.h>

#include <qcheckbox.h>


K3bDivxOptionTab::K3bDivxOptionTab(QWidget* parent,  const char* name )
  : base_K3bDivxOptions( parent, name )
{
    //QStringList list("xvid"
    //m_comboCodec->insertStringList();
}


K3bDivxOptionTab::~K3bDivxOptionTab()
{
}

void K3bDivxOptionTab::readSettings(){
  KConfig* c = kapp->config();

  c->setGroup( "Divx" );

  m_checkYuv->setChecked( c->readBoolEntry("use yuv", true ));
  
}


void K3bDivxOptionTab::saveSettings(){
  
  KConfig* c = kapp->config();

  c->setGroup( "Divx" );

  c->writeEntry( "use yuv", m_checkYuv->isChecked() );
  c->writeEntry( "avi size", m_inputAvisize->value() );
  c->writeEntry( "keyframes", m_spinKeyframes->value() );
  c->writeEntry( "codec", m_comboCodec->currentText() );
/*
QButtonGroup* m_buttonCodecMode;
    QComboBox* m_comboCodec;
    KComboBox* m_comboInterlace;
    QLabel* m_labelCrispnessPercent;
    QSlider* m_sliderCrispness;
    QCheckBox* m_checkResample;
    KComboBox* m_comboLanguage;
    QButtonGroup* m_buttonAudio;
    QButtonGroup* m_buttonResize;
    QButtonGroup* m_buttonTranscode;
    QCheckBox* m_checkAutoCrop;
    QButtonGroup* m_buttonAutoQuality;
    KDoubleNumInput* m_inputVideoQuality;
    KIntNumInput* m_inputWidth;
    KLineEdit* m_lineAviDir;
    KLineEdit* m_lineInputDir;
    KIconButton* m_buttonAviDir;
    KIconButton* m_buttonInputDir;
*/

}

#include "k3bdivxoptiontab.moc"

