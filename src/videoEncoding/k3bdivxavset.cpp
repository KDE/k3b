/*
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

/***************************************************************************
      k3bdvdavset.cpp  -  description
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

#include "k3bdivxavset.h"
#include "k3bdivxcodecdata.h"
#include "k3bdivxtcprobeac3.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qhbuttongroup.h>
#include <qradiobutton.h>
#include <qwhatsthis.h>
#include <qcheckbox.h>
#include <qspinbox.h>

#include <klocale.h>
#include <kdialog.h>
#include <kcombobox.h>
#include <kdebug.h>

// order must be in sync with comboBox entries
static long finalSize[] = { 681574400L, 734000320L, 1363148800L, 1468006400L, 728760000L, 739246000L };
// order must be in sync with comboBox entries
static long audioBitrate[] = { 64000L, 96000L, 112000L, 128000L, 160000L, 192000L };
// order must be in sync with comboBox entries
static QString codec[] = { "xvid", "divx4", "divx5", "xvidcvs" };
static int cdSizes[] = { 650, 700, 1300, 1400, 695, 705 };

K3bDivxAVSet::K3bDivxAVSet( K3bDivxCodecData *data, QWidget *parent, const char *name ) : QGroupBox( parent, name ) {
    m_data = data;
    m_lengthSecs = 0;
    setupGui();
}

K3bDivxAVSet::~K3bDivxAVSet() {}

void K3bDivxAVSet::setupGui() {
    setColumnLayout( 0, Qt::Vertical );
    setTitle( i18n( "Basic Audio/Video Settings" ) );
    layout() ->setMargin( 0 );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );

    QLabel *videoBitrate = new QLabel( i18n( "Video bitrate:" ), this );
    QLabel *cds = new QLabel( i18n( "CDs:" ), this );
    QString wt_cd( i18n( "Select how many CDs the final encoded video should have. You can select CDRs with a size of 650MB and 700MB." ) );
    QWhatsThis::add( cds, wt_cd );
    QLabel *mp3bitrate = new QLabel( i18n( "MP3 bitrate:" ), this );
    QString wt_mp3( i18n( "Select bitrate of the audio track. MP3 can be encoded with constant or variable bitrate and joint stereo. AC3 passthrough must be disabled to use MP3." ) );
    QWhatsThis::add( mp3bitrate, wt_mp3 );
    QLabel *codec = new QLabel( i18n( "Video codec:" ), this );
    // PROOF READER COMMENT: I've taken out some of the subjective text and made it a bit more general
    //                       I hope you're okay with that - my goal is to make it more professional :¬)
    // Thomas: Sounds good to me. Thanks.
    QString wt_codec( i18n( "Select the video codec to encode to the final movie. XviD (www.xvid.org) is an Open Source codec and \
has similar features to DivX5. DivX4 is the predecessor to DivX5. All three codecs support 1-pass and 2-pass encoding. \
XviD (CVS) is support for the latest nightly snapshots of XviD. \
Regarding quality, try all the different codecs to find out which you prefer. Secondly, read the various forums about MPEG-4 Encoding \
(www.doom9.org, www.xvid.org, www.divx.net, ... ). The difference between a DivX4 and XviD 2-pass encoded movie \
is quite small. Sometimes DivX4 (smoother) is better and other times XviD (sharper). \
If the encoding process crashes then you probably haven't used the codec you have installed. Due to the codec libraries having the same name, you \
can only use DivX4 or DivX5 and XviD or XviD (CVS). This will be fixed in a future version, so the codecs will auto-detect and \
can be used with different install locations." ) );
    QWhatsThis::add( codec, wt_codec );
    QLabel *codecmode = new QLabel( i18n( "Codec mode:" ), this );
    QString wt_codecmode( i18n( "Select the mode for video encoding. 1-pass encoding has lower quality than 2-pass, but requires half the time to encode a video. \
In 2-pass mode the video will be encoded twice. The first time, the video will only be analyzed to get the best quality in the second encoding pass." ) );
    QWhatsThis::add( codecmode, wt_codecmode );
    m_vBitrate = new QLabel( this );

    m_comboCd = new KComboBox( false, this );
    m_comboCd->insertItem( i18n( "1 x 650 MB" ) );
    m_comboCd->insertItem( i18n( "1 x 700 MB" ) );
    m_comboCd->insertItem( i18n( "2 x 650 MB" ) );
    m_comboCd->insertItem( i18n( "2 x 700 MB" ) );
    m_comboCd->insertItem( i18n( "1 x 695 MB" ) );
    m_comboCd->insertItem( i18n( "1 x 705 MB" ) );
    m_comboCd->insertItem( i18n( "---" ) );
    QWhatsThis::add( m_comboCd, wt_cd );

    m_vBitrateCustom = new QSpinBox( 1, 2048, 1, this );
    m_vBitrateCustom->setSuffix(" MByte");
    QString wt_custombitrate( i18n( "Select individual filesize of the encoded video instead using \"number of CDs\"." ) );
    QWhatsThis::add( m_vBitrateCustom, wt_custombitrate );

    m_checkAc3Passthrough = new QCheckBox( i18n( "AC3 pass-through mode" ), this );
    QWhatsThis::add( m_checkAc3Passthrough, i18n( "Enable this if you want the orginal digital sound (AC3)." ) );
    m_aAC3Bitrate = new QLabel( "", this );

    m_comboMp3 = new KComboBox( false, this );
    m_comboMp3->insertItem( i18n( " 64 kbits" ) );
    m_comboMp3->insertItem( i18n( " 96 kbits" ) );
    m_comboMp3->insertItem( i18n( "112 kbits" ) );
    m_comboMp3->insertItem( i18n( "128 kbits" ) );
    m_comboMp3->insertItem( i18n( "160 kbits" ) );
    m_comboMp3->insertItem( i18n( "192 kbits" ) );
    QWhatsThis::add( m_comboMp3, wt_mp3 );
    m_comboCodec = new KComboBox( false, this );
    m_comboCodec->insertItem( "XviD" );
    m_comboCodec->insertItem( "DivX4" );
    m_comboCodec->insertItem( "DivX5" );
    m_comboCodec->insertItem( "XviD (CVS)" );
    QWhatsThis::add( m_comboCodec, wt_codec );

    QHButtonGroup *modeGroup = new QHButtonGroup( this );
    modeGroup->layout() ->setSpacing( KDialog::spacingHint() );
    modeGroup->layout() ->setMargin( KDialog::marginHint() );
    modeGroup->setFrameStyle( Plain | NoFrame );
    m_buttonOnePass = new QRadioButton( i18n( "1-pass" ), modeGroup );
    m_buttonTwoPass = new QRadioButton( i18n( "2-pass" ), modeGroup );
    modeGroup->setButton( 1 );
    QWhatsThis::add( modeGroup, wt_codecmode );

    m_mp3modeGroup = new QHButtonGroup( this );
    m_mp3modeGroup->layout() ->setSpacing( KDialog::spacingHint() );
    m_mp3modeGroup->layout() ->setMargin( 0 ); //KDialog::marginHint() );
    m_mp3modeGroup->setFrameStyle( Plain | NoFrame );
    m_buttonCbr = new QRadioButton( i18n( "Constant bitrate", "CBR" ), m_mp3modeGroup );
    m_buttonVbr = new QRadioButton( i18n( "Variable bitrate", "VBR" ), m_mp3modeGroup );
    m_buttonVbr->setEnabled( false );
    m_mp3modeGroup->setButton( 0 );
    QWhatsThis::add( m_mp3modeGroup, i18n( "If set to CBR the MP3 encoding is done with a constant bitrate. \
If set to VBR then a variable bitrate is used. Typically a variable bitrate gets better quality but is often out of sync. \
You should try it first with some test encodings." ) );

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    mainLayout->addMultiCellWidget( videoBitrate, 0, 0, 0, 1 );
    mainLayout->addMultiCellWidget( m_vBitrate, 0, 0, 2, 2 );
    mainLayout->addMultiCellWidget( cds, 1, 1, 0, 0 );
    mainLayout->addMultiCellWidget( m_comboCd, 1, 1, 1, 1 );
    mainLayout->addMultiCellWidget( m_vBitrateCustom, 1, 1, 2, 2 );
    mainLayout->addMultiCellWidget( m_checkAc3Passthrough, 2, 2, 0, 1 );
    mainLayout->addMultiCellWidget( m_aAC3Bitrate, 2, 2, 2, 2 );
    mainLayout->addMultiCellWidget( mp3bitrate, 3, 3, 0, 0 );
    mainLayout->addMultiCellWidget( m_comboMp3, 3, 3, 1, 1 );
    mainLayout->addMultiCellWidget( m_mp3modeGroup, 3, 3, 2, 2 );
    mainLayout->addMultiCellWidget( m_comboCodec, 4, 4, 1, 1 );
    mainLayout->addMultiCellWidget( codec, 4, 4, 0, 0 );
    mainLayout->addMultiCellWidget( codecmode, 5, 5, 0, 0 );
    mainLayout->addMultiCellWidget( modeGroup, 5, 5, 1, 2 );
    mainLayout->addItem( spacer, 6, 1 );

    connect( m_comboMp3, SIGNAL( activated( int ) ), this, SLOT( slotCalcBitrate( ) ) );
    connect( m_comboCd, SIGNAL( activated( int ) ), this, SLOT( slotCDSize( ) ) );
    connect( m_comboCodec, SIGNAL( activated( int ) ), this, SLOT( slotCodecChanged( int ) ) );
    connect( modeGroup, SIGNAL( clicked( int ) ), this, SLOT( slotModeChanged( int ) ) );
    connect( m_mp3modeGroup, SIGNAL( clicked( int ) ), this, SLOT( slotMp3ModeChanged( int ) ) );
    connect( m_checkAc3Passthrough, SIGNAL( stateChanged( int ) ), this, SLOT( slotAc3Passthrough( int ) ) );
    connect( m_vBitrateCustom, SIGNAL( valueChanged( int ) ), this, SLOT( slotCustomBitrate( int ) ) );

}

void K3bDivxAVSet::updateView( ) {
    QTime t = m_data->getTime();
    m_lengthSecs = t.hour() * 3600 + t.minute() * 60 + t.second();
    //m_checkAc3Passthrough->setChecked( false );
    slotCalcBitrate();
    if ( m_checkAc3Passthrough->isChecked() ) {
        slotViewAc3Bitrate();
    }
}

void K3bDivxAVSet::init() {
    m_data->setCodecMode( 2 ); // button 1 is set in setupGUI = 2pass
    m_data->setCodec( codec[ 0 ] ); // default in combobox = first entry
    m_comboCd->setCurrentItem( 1 ); // 1*700MB128 kbits
    m_vBitrateCustom->setValue( 700 );
    m_comboMp3->setCurrentItem( 3 ); // 128 kbits
    m_data->setAudioBitrate( 128 );
    //updateView();
    slotCalcBitrate();
}
void K3bDivxAVSet::slotCDSize() {
   if ( !(m_comboCd->currentItem()==6) ) {
       m_fixedCDSize = true;
       m_vBitrateCustom->setValue( cdSizes[ m_comboCd->currentItem() ] );
    }
    slotCalcBitrate();
}
void K3bDivxAVSet::slotCalcBitrate() {
    kdDebug() << "(K3bDivxAVSet::slotCalcBitrate)" << endl;
    int sizeIndex = m_comboCd->currentItem();
    int aBitrateIndex = m_comboMp3->currentItem();
    if ( m_lengthSecs < 1 ) {
        kdDebug() << "(K3bDivxAVSet) Warning: no video length. You must load an project file" << endl;
        return ;
    }
    long vBitrate = 0L;
    if ( m_fixedCDSize  && !(m_comboCd->currentItem()==6) ) {
        vBitrate = ( finalSize[ sizeIndex ] / m_lengthSecs * 8 ); // one correct 1.024 K->1024
    } else {
        vBitrate = ( long int ) ( m_vBitrateCustom->value() * 1024 * 1024 / m_lengthSecs * 8 );
    }
    if ( m_comboCodec->currentItem() == 1 ) {
        vBitrate = ( long int ) ( ( ( double ) vBitrate ) * 1.024 ); // correction vor divx4
    }
    // ac3 - mp3 bitrate calculation
    if ( m_checkAc3Passthrough->isChecked() ) {
        kdDebug() << "(K3bDivxAVSet) Set bitrate with ac3 sound." << endl;
        vBitrate = vBitrate - ( m_data->getAudioLanguageAc3Bitrate( m_data->getAudioLanguage() ).toInt() * 1000 );
    } else {
        vBitrate = vBitrate - audioBitrate[ aBitrateIndex ];
    }
    m_vBitrate->setText( i18n( "%1 kbits" ).arg( vBitrate / 1000 ) );
    m_data->setVideoBitrate( vBitrate / 1000 );
    m_data->setAudioBitrate( audioBitrate[ aBitrateIndex ] / 1000 );
}

void K3bDivxAVSet::slotCodecChanged( int codecIndex ) {
    m_data->setCodec( codec[ codecIndex ] );
    slotCalcBitrate();
}

void K3bDivxAVSet::slotModeChanged( int id ) {
    // id = 0 is 1pass
    m_data->setCodecMode( id + 1 );
}

void K3bDivxAVSet::slotMp3ModeChanged( int id ) {
    // id = 0 is cbr, 1 is vbr
    m_data->setMp3CodecMode( id );
}
void K3bDivxAVSet::slotAc3Passthrough( int mode ) {
    kdDebug() << "(K3bDivxAVSet::slotAc3Passthrough)" << endl;
    m_data->setAc3( mode );
    updateView();
    if ( mode == 0 ) {
        m_mp3modeGroup->setEnabled( true );
        m_comboMp3->setEnabled( true );
    } else {
        m_mp3modeGroup->setEnabled( false );
        m_comboMp3->setEnabled( false );
        if ( !m_data->isAc3Set() ) {
            m_parser = new K3bDivXTcprobeAc3();
            m_parser->parseAc3Bitrate( m_data );
            connect( m_parser, SIGNAL( finished() ), this, SLOT( slotAc3Scaned() ) );
        }
    }
}

void K3bDivxAVSet::slotViewAc3Bitrate() {
    kdDebug() << "(K3bDivxAVSet::slotViewAc3Bitrate)" << endl;
    QString ac3bitrate = m_data->getAudioLanguageAc3Bitrate( m_data->getAudioLanguage() );
    if ( m_checkAc3Passthrough->isChecked() ) {
        m_aAC3Bitrate->setText( "(" + ac3bitrate + " kbps)" );
    } else {
        m_aAC3Bitrate->setText( "" );
    }
}

void K3bDivxAVSet::slotAc3Scaned() {
    updateView();
    disconnect( m_parser, SIGNAL( finished() ), this, SLOT( slotAc3Scaned() ) );
    delete m_parser;
}

void K3bDivxAVSet::slotCustomBitrate( int size ) {
    kdDebug() << "(K3bDivxAVSet::slotCustomBitrate)" << endl;
    int i = 0;
    m_fixedCDSize = false;
    for ( i = 0; i < 6; i++ ) {
        if ( size == cdSizes[ i ] ) {
            m_comboCd->setCurrentItem( i );
            m_fixedCDSize = true;
        }
    }
    if ( !m_fixedCDSize ) {
        m_comboCd->setCurrentItem( 6 ); // "---"
    }
    slotCalcBitrate( );
}

#include "k3bdivxavset.moc"
