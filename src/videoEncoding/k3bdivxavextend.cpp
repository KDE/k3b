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


#include "k3bdivxavextend.h"
#include "k3bdivxcodecdata.h"

#include <qlabel.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qframe.h>
#include <qbutton.h>
#include <qwhatsthis.h>

#include <kdialog.h>
#include <klocale.h>
#include <krestrictedline.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kmessagebox.h>


K3bDivxAVExtend::K3bDivxAVExtend( K3bDivxCodecData *data, QWidget *parent, const char *name ) : QGroupBox( parent,name) {
     m_data =  data;
     m_wrongsettings = i18n("Wrong setting");
     m_dilError = i18n("The deinterlace filter YUVDeinterlaceMMX doesn't work with RGB mode. "
		       "You must enable using YUV colorspace.");
     m_smartError = i18n("The deinterlace filter SmartDeinterlace doesn't work with YUV "
			 "colorspace. It only works with RGB. You must disable using YUV colorspace.");
     setupGui();
}

K3bDivxAVExtend::~K3bDivxAVExtend(){
}

void K3bDivxAVExtend::setupGui(){
    setColumnLayout(0, Qt::Vertical );
    setTitle( i18n( "Extended Audio/Video Settings" ) );
    layout()->setMargin( 0 );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );

    QLabel *keyframes = new QLabel( i18n("Keyframes:"), this );
    QString wt_keyframes( i18n("Keyframes (same as I-frames) are \"full\" pictures of an MPEG-stream. "
			       "They are similar to a simple JPEG and have the best quality "
			       "in a video. All pictures following are only stored as differences to a "
			       "keyframe. Here you can set the maximum distance between two keyframes. "
			       "In most cases this will rarely be used, as the codec will automatically "
			       "insert a keyframe on each scene change. Default is 300."));
    QWhatsThis::add( keyframes, wt_keyframes);
    QLabel *crispness = new QLabel( i18n("Crispness:"), this );
    QString wt_crispness( i18n("Here you can smooth the video during the encoding process. "
			       "It is better to leave crispness at 100% for encoding and smooth "
			       "the video during playback instead."));
    QWhatsThis::add( crispness, wt_crispness);
    //QLabel *audiogain = new QLabel( i18n("Increase audio volume by:"), this );
    QLabel *language = new QLabel( i18n("Audio language:"), this );
    QString wt_language( i18n("Select the language and the surround mode. Typically there are "
			      "\"MPEG2 Stereo\", \"AC3 2ch and 6ch\" and \"DTS 6ch\". "
			      "You can probably choose AC3 6ch, AC3 2ch and MPEG2 Stereo in descending "
			      "order, depending on your language (It doesn't matter if DTS works or not, "
			      "you can still try it).") );
    QWhatsThis::add( language, wt_language);
    QLabel *deinterlace = new QLabel( i18n("Deinterlace mode:"), this );
    QString wt_deinterlace( i18n("Select a deinterlace mode if you have interlaced material."
				 "<ul><li><strong>Fast</strong> is the standard deinterlace mode</li>"
				 "<li><strong>Encoder Based</strong> uses the internal deinterlace of the "
				 "codec (be aware that some codecs may have no deinterlacer)</li>"
				 "<li><strong>Zoom to Full Frame</strong> no matter how it works but "
				 "it is rather good and slow. Use this if <strong>Fast</strong> still "
				 "produces interlace stripes or color artifacts (red shadow, etc.)<li>"
				 "<li><strong>Drop Field/Half Height</strong> uses only one field and "
				 "scales it.</li>"
				 "<li><strong>SmartDeinterlace</strong> provides a smart, motion-based "
				 "deinterlacing capability. Ported from VirtualDub by Tilmann Bitterberg. "
				 "Operates in RGB space only. Tilmann did some testing of deinterlace filters "
				 "available in transcode. You can view and read the result on tibit.org/video/. </li>"
				 "<li><strong>YUVDeinterlaceMMX</strong> is a deinterlace for YUV mode by "
				 "Thomas Oestreich.</li></ul>"));
    QWhatsThis::add( deinterlace, wt_deinterlace);
    QGroupBox *groupCrispness = new QGroupBox( this );
    groupCrispness->setColumnLayout(0, Qt::Horizontal );
    groupCrispness->setFrameStyle( Plain | NoFrame );
    groupCrispness->layout()->setSpacing( 0 );
    groupCrispness->layout()->setMargin( 0 );
    QGridLayout *crispLayout = new QGridLayout( groupCrispness->layout() );
    crispLayout->setSpacing( 0 );
    crispLayout->setMargin( 0 );

    m_sliderCrispness = new QSlider ( 0, 100, 1, 100, Horizontal, groupCrispness);
    m_labelCrispness = new QLabel( "100 %", groupCrispness );
    crispLayout->addWidget( m_sliderCrispness, 0, 0 );
    crispLayout->addWidget( m_labelCrispness, 0, 1 );
    QWhatsThis::add( groupCrispness, wt_crispness);

    m_checkResample = new QCheckBox( i18n( "Resample to 44.1 kHz" ), this );
    QWhatsThis::add( m_checkResample, i18n("If this is checked, the audio track will be downsampled "
					   "from 48 kHz to 44.1 kHz. Useful because some soundcards "
					   "have problems with a sampling rate of 48 kHz."));
    m_checkYuv = new QCheckBox( i18n( "Use YUV colorspace." ), this );
    QWhatsThis::add( m_checkYuv, i18n("If this is checked, the internal codec works with the YUV "
				      "colorspace instead of using RGB. Useful because DVD "
				      "also has YUV colorspace and it is twice fast as RGB. You only "
				      "need RGB colorspace if a filter needs it (filters not yet supported)."));
    m_comboDeinterlace = new KComboBox( this );
    m_comboDeinterlace->insertItem(i18n("0 - None"));
    m_comboDeinterlace->insertItem(i18n("1 - Fast"));
    m_comboDeinterlace->insertItem(i18n("2 - Encoder Based"));
    m_comboDeinterlace->insertItem(i18n("3 - Zoom to Full Frame"));
    m_comboDeinterlace->insertItem(i18n("4 - Drop Field/Half Height"));
    m_comboDeinterlace->insertItem(i18n("Filter - SmartDeinterlace"));
    m_comboDeinterlace->insertItem(i18n("Filter - YUVDeinterlaceMMX"));
    
    QWhatsThis::add( m_comboDeinterlace, wt_deinterlace);
    m_comboLanguage = new KComboBox( this );
    QWhatsThis::add( m_comboLanguage, wt_language);
    //m_editAudioGain = new KRestrictedLine( this, "audioline", "0123456789,." );
    m_editKeyframes = new KRestrictedLine( this, "keyframesline", "0123456789" );
    QWhatsThis::add( m_editKeyframes, wt_keyframes);

    QFrame* line = new QFrame( this, "line" );
    line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

    mainLayout->addMultiCellWidget( m_checkYuv, 0, 0, 0, 0);
    mainLayout->addMultiCellWidget( deinterlace, 1, 1, 0, 0);
    mainLayout->addMultiCellWidget( m_comboDeinterlace, 1, 1, 1, 1);
    mainLayout->addMultiCellWidget( keyframes, 2, 2, 0, 0);
    mainLayout->addMultiCellWidget( m_editKeyframes, 2, 2, 1, 1);
    mainLayout->addMultiCellWidget( crispness, 3, 3, 0, 0);
    mainLayout->addMultiCellWidget( groupCrispness, 3, 3, 1, 1);
    mainLayout->addMultiCellWidget( line, 4, 4, 0, 2);
    mainLayout->addMultiCellWidget( m_checkResample, 5, 5, 0, 0);
    mainLayout->addMultiCellWidget( language, 6, 6, 0, 0);
    mainLayout->addMultiCellWidget( m_comboLanguage, 6, 6, 1, 1);
    //mainLayout->addMultiCellWidget( audiogain, 7, 7, 0, 0);
    //mainLayout->addMultiCellWidget( m_editAudioGain, 7, 7, 1, 1);

    //connect( m_editKeyframes, SIGNAL( returnPressed( const QString& )), this, SLOT( slotKeyframes( const QString& ) ));
    connect( m_editKeyframes, SIGNAL( textChanged( const QString& )), this, SLOT( slotKeyframes( const QString& ) ));
    //connect( m_editAudioGain, SIGNAL( returnPressed( const QString& )), this, SLOT( slotAudioGain( const QString& ) ));
    //connect( m_editAudioGain, SIGNAL( textChanged( const QString& )), this, SLOT( slotAudioGain( const QString& ) ));
    connect( m_sliderCrispness, SIGNAL( valueChanged( int ) ), this, SLOT( slotCrispness( int ) ));
    connect( m_checkResample, SIGNAL( stateChanged( int )), this, SLOT( slotResample( int ) ));
    connect( m_checkYuv, SIGNAL( stateChanged( int ) ), this, SLOT( slotYuv( int ) ));
    connect( m_comboDeinterlace, SIGNAL( activated( int )), this, SLOT( slotDeinterlace( int )) );
    connect( m_comboLanguage, SIGNAL( activated( int )), this, SLOT( slotAudioLanguage( int )) );
}

void K3bDivxAVExtend::updateView( ){
    //kdDebug() << "(K3bDivxAVExtend::updateView) Readd languages." << endl;
    //m_comboLanguage->clear();
    //m_comboLanguage->insertStringList( m_data->getAudioLanguages() );
}
void K3bDivxAVExtend::initView(){
     m_editKeyframes->setText( m_data->getKeyframes() );
     //m_editAudioGain->setText( m_data->getAudioGain() );
     m_data->setCrispness( m_sliderCrispness->value() );
     m_checkYuv->setChecked( true );
     m_data->setYuv( 2 );
     m_checkResample->setChecked( true );
     m_data->setAudioResample( 2 );
     m_data->setDeinterlace( m_comboDeinterlace->currentItem() );
     m_comboLanguage->clear();
     m_comboLanguage->insertStringList( m_data->getAudioLanguages() );
     m_data->setAudioLanguage( m_comboLanguage->currentItem() );
}

void K3bDivxAVExtend::slotKeyframes( const QString &text ){
     m_data->setKeyframes( text );
}
void K3bDivxAVExtend::slotAudioGain( const QString &text ){
     m_data->setAudioGain( text );
}

void K3bDivxAVExtend::slotCrispness( int value ){
     m_data->setCrispness( value );
     if( value < 100 ){
         m_labelCrispness->setText( " " + QString::number( value ) + "%");
     } else {
         m_labelCrispness->setText( QString::number( value ) + " %");
     }
}
void K3bDivxAVExtend::slotDeinterlace( int index ){
    if( index == SMARTDEINTER && m_checkYuv->isChecked() ){
        // smartdeinterlace, only rgb
        KMessageBox::error( this, m_smartError,m_wrongsettings );
        m_comboDeinterlace->setCurrentItem(0);
        return;
    } else if ( index == DILYUVMMX && ! m_checkYuv->isChecked() ){
        // yuvdilmmx, only yuv
        KMessageBox::error( this, m_dilError, m_wrongsettings); 
        m_comboDeinterlace->setCurrentItem(0);
        return;
    }
    m_data->setDeinterlace( index );
}

void K3bDivxAVExtend::slotYuv( int state ){
    if ( state == 0 && m_data->getDeinterlace() == DILYUVMMX ){
        KMessageBox::error( this, m_dilError, m_wrongsettings); 
        m_checkYuv->setEnabled( true );
        return;
    } else if (state == 2 && m_data->getDeinterlace() == SMARTDEINTER ){
        KMessageBox::error( this, m_smartError,m_wrongsettings );
        m_checkYuv->setEnabled( false );
        return;
    }
    m_data->setYuv( state );
}
void K3bDivxAVExtend::slotResample( int state ){
    m_data->setAudioResample( state );
}

void K3bDivxAVExtend::slotAudioLanguage( int index ){
    m_data->setAudioLanguage( index );
    emit dataChanged();
}

#include "k3bdivxavextend.moc"
