/***************************************************************************
                          k3bdvdavextend.cpp  -  description
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

#include "k3bdvdavextend.h"

#include <qlabel.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qframe.h>

#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>
#include <kcombobox.h>

K3bDvdAVExtend::K3bDvdAVExtend(QWidget *parent, const char *name ) : QGroupBox(parent,name) {
     setupGui();
}

K3bDvdAVExtend::~K3bDvdAVExtend(){
}

void K3bDvdAVExtend::setupGui(){
    setColumnLayout(0, Qt::Vertical );
    setTitle( i18n( "Extended Audio/Video Settings" ) );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );

    QLabel *keyframes = new QLabel( i18n("Keyframes:"), this );
    QLabel *crispness = new QLabel( i18n("Crispness:"), this );
    QLabel *audiogain = new QLabel( i18n("Increase audio volume by:"), this );
    QLabel *language = new QLabel( i18n("Audio language:"), this );
    QLabel *deinterlace = new QLabel( i18n("De-interlace mode:"), this );

    m_sliderCrispness = new QSlider ( 0, 100, 1, 90, Horizontal, this);
    m_checkResample = new QCheckBox( i18n( "Resample to 44.1 kHz" ), this );
    m_checkYuv = new QCheckBox( i18n( "Use YUV colorspace." ), this );
    m_comboDeinterlace = new KComboBox( this );
    m_comboLanguage = new KComboBox( this );
    m_editAudioGain = new KLineEdit( this );
    m_editKeyframes = new KLineEdit( this );

    QFrame* line = new QFrame( this, "line" );
    line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

    mainLayout->addMultiCellWidget( m_checkYuv, 0, 0, 0, 0);
    mainLayout->addMultiCellWidget( deinterlace, 1, 1, 0, 0);
    mainLayout->addMultiCellWidget( m_comboDeinterlace, 1, 1, 1, 1);
    mainLayout->addMultiCellWidget( keyframes, 2, 2, 0, 0);
    mainLayout->addMultiCellWidget( m_editKeyframes, 2, 2, 1, 1);
    mainLayout->addMultiCellWidget( crispness, 3, 3, 0, 0);
    mainLayout->addMultiCellWidget( m_sliderCrispness, 3, 3, 1, 1);
    mainLayout->addMultiCellWidget( line, 4, 4, 0, 2);
    mainLayout->addMultiCellWidget( m_checkResample, 5, 5, 0, 0);
    mainLayout->addMultiCellWidget( language, 6, 6, 0, 0);
    mainLayout->addMultiCellWidget( m_comboLanguage, 6, 6, 1, 1);
    mainLayout->addMultiCellWidget( audiogain, 7, 7, 0, 0);
    mainLayout->addMultiCellWidget( m_editAudioGain, 7, 7, 1, 1);

}

#include "k3bdvdavextend.moc"