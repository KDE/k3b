/***************************************************************************
                          k3bdivxavextend.cpp  -  description
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

K3bDivxAVExtend::K3bDivxAVExtend( K3bDivxCodecData *data, QWidget *parent, const char *name ) : QGroupBox( parent,name) {
     m_data =  data;
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
    QString wt_keyframes( i18n("Keyframes (same as I-frames) are \"full\" pictures of an MPEG-stream. They are similar to a simple JPEG and have the best quality \
in a video. All pictures following are only stored as differences to a keyframe. Here you can set the maximum distance between two keyframes. In most cases this will \
rarely be used, as the codec will automatically insert a keyframe on each scene change. Default is 300."));
    QWhatsThis::add( keyframes, wt_keyframes);
    QLabel *crispness = new QLabel( i18n("Crispness:"), this );
    QString wt_crispness( i18n("Here you can smooth the video during the encoding process. It is better to leave crispness at 100% for encoding and smooth \
the video during playback instead."));
    QWhatsThis::add( crispness, wt_crispness);
    //QLabel *audiogain = new QLabel( i18n("Increase audio volume by:"), this );
    QLabel *language = new QLabel( i18n("Audio language:"), this );
    QString wt_language( i18n("Select the language and the surround mode. Typically there are \"MPEG2 Stereo\", \"AC3 2ch and 6ch\" and \"DTS 6ch\". \
You can probably choose AC3 6ch, AC3 2ch and MPEG2 Stereo in descending order, depending on your language (It doesn't matter if DTS works or not, you can still try it).") );
    QWhatsThis::add( language, wt_language);
    QLabel *deinterlace = new QLabel( i18n("Deinterlace mode:"), this );
    QString wt_deinterlace( i18n("Select a deinterlace mode if you have interlaced material. \"Fast\" is the standard deinterlace mode, \"Encoder Based\" uses \
the internal deinterlace of the codec (be aware that some codecs may have no deinterlacer), \"Zoom to Full Frame\" no matter how it works but it is rather good and slow. Use this if \"Fast\" still produces interlace \
stripes or color artifacts (red shadow, etc.), \"Drop Field/Half Height\" uses only one field and scales it." ));
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
    QWhatsThis::add( m_checkResample, i18n("If this is checked, the audio track will be downsampled from 48 kHz to 44.1 kHz. Useful because some soundcards \
have problems with a sampling rate of 48 kHz."));
    m_checkYuv = new QCheckBox( i18n( "Use YUV colorspace." ), this );
    QWhatsThis::add( m_checkYuv, i18n("If this is checked, the internal codec works with the YUV colorspace instead of using RGB. Useful because DVD \
also has YUV colorspace and it is twice fast as RGB. You only need RGB colorspace if a filter needs it (filters not yet supported)."));
    m_comboDeinterlace = new KComboBox( this );
    m_comboDeinterlace->insertItem(i18n("0 - None"));
    m_comboDeinterlace->insertItem(i18n("1 - Fast"));
    m_comboDeinterlace->insertItem(i18n("2 - Encoder Based"));
    m_comboDeinterlace->insertItem(i18n("3 - Zoom to Full Frame"));
    m_comboDeinterlace->insertItem(i18n("4 - Drop Field/Half Height"));
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
    m_comboLanguage->clear();
    m_comboLanguage->insertStringList( m_data->getAudioLanguages() );
}
void K3bDivxAVExtend::init(){
     m_editKeyframes->setText( m_data->getKeyframes() );
     //m_editAudioGain->setText( m_data->getAudioGain() );
     m_data->setCrispness( m_sliderCrispness->value() );
     m_checkYuv->setChecked( true );
     m_data->setYuv( 2 );
     m_checkResample->setChecked( true );
     m_data->setAudioResample( 2 );
     m_data->setDeinterlace( m_comboDeinterlace->currentItem() );
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
    m_data->setDeinterlace( index );
}
void K3bDivxAVExtend::slotYuv( int state ){
    m_data->setYuv( state );
}
void K3bDivxAVExtend::slotResample( int state ){
    m_data->setAudioResample( state );
}

void K3bDivxAVExtend::slotAudioLanguage( int index ){
    m_data->setAudioLanguage( index );
}

#include "k3bdivxavextend.moc"
