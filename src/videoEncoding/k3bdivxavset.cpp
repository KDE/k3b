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

#include <qlabel.h>
#include <qlayout.h>
#include <qhbuttongroup.h>
#include <qradiobutton.h>

#include <klocale.h>
#include <kdialog.h>
#include <kcombobox.h>
#include <kdebug.h>

static long finalSize[] = { 681574400L, 734000320L, 1363148800L, 1468006400L };
static long audioBitrate[] = { 64000L, 96000L, 112000L, 128000L, 160000L, 192000L };
static QString codec[] = { "xvid", "divx4", "divx5" };

K3bDivxAVSet::K3bDivxAVSet(K3bDivxCodecData *data, QWidget *parent, const char *name ) : QGroupBox( parent,name ) {
    m_data = data;
    setupGui();
}

K3bDivxAVSet::~K3bDivxAVSet(){
}

void K3bDivxAVSet::setupGui(){
    setColumnLayout(0, Qt::Vertical );
    setTitle( i18n( "Basic Audio/Video Settings" ) );
    layout()->setMargin( 0 );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );

    QLabel *cds = new QLabel( i18n("CDs:"), this );
    QLabel *mp3bitrate = new QLabel( i18n("MP3 Bitrate:"), this );
    QLabel *codec = new QLabel( i18n("Video codec:"), this );
    QLabel *codecmode = new QLabel( i18n("Codec mode:"), this );
    m_vBitrateDesc = i18n("Bitrate: ");
    m_vBitrate = new QLabel( m_vBitrateDesc, this );

    m_comboCd = new KComboBox( false, this );
    m_comboCd->insertItem( i18n("1 * 650 MB" ) );
    m_comboCd->insertItem( i18n("1 * 700 MB" ) );
    m_comboCd->insertItem( i18n("2 * 650 MB" ) );
    m_comboCd->insertItem( i18n("2 * 700 MB" ) );
    m_comboMp3 = new KComboBox( false, this );
    m_comboMp3->insertItem( " 64 kbits" );
    m_comboMp3->insertItem( " 96 kbits" );
    m_comboMp3->insertItem( "112 kbits" );
    m_comboMp3->insertItem( "128 kbits" );
    m_comboMp3->insertItem( "160 kbits" );
    m_comboMp3->insertItem( "192 kbits" );
    m_comboCodec = new KComboBox( false, this );
    m_comboCodec->insertItem( "XviD" );
    m_comboCodec->insertItem( "DivX4" );
    m_comboCodec->insertItem( "DivX5" );

    QHButtonGroup *modeGroup = new QHButtonGroup( this );
    modeGroup->layout()->setSpacing( KDialog::spacingHint() );
    modeGroup->layout()->setMargin( KDialog::marginHint() );
    modeGroup->setFrameStyle( Plain | NoFrame );
    m_buttonOnePass = new QRadioButton( i18n("1-pass"), modeGroup );
    m_buttonTwoPass = new QRadioButton( i18n("2-pass"), modeGroup );
    modeGroup->setButton( 1 );

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    mainLayout->addMultiCellWidget( cds, 0, 0, 0, 0);
    mainLayout->addMultiCellWidget( mp3bitrate, 1, 1, 0, 0);
    mainLayout->addMultiCellWidget( codec, 2, 2, 0, 0);
    mainLayout->addMultiCellWidget( codecmode, 3, 3, 0, 0);
    mainLayout->addMultiCellWidget( m_vBitrate, 0, 0, 2, 2);
    mainLayout->addMultiCellWidget( m_comboCd, 0, 0, 1, 1);
    mainLayout->addMultiCellWidget( m_comboMp3, 1, 1, 1, 1);
    mainLayout->addMultiCellWidget( m_comboCodec, 2, 2, 1, 1);
    mainLayout->addMultiCellWidget( modeGroup, 3, 3, 1, 2);
    mainLayout->addItem( spacer, 4, 1);

    connect( m_comboMp3, SIGNAL( activated( int )), this, SLOT( slotCalcBitrate( ) ));
    connect( m_comboCd, SIGNAL( activated( int )), this, SLOT( slotCalcBitrate( ) ));
    connect( m_comboCodec, SIGNAL( activated( int )), this, SLOT( slotCodecChanged( int ) ));
    connect( modeGroup, SIGNAL( clicked( int )), this, SLOT( slotModeChanged( int ) ));
}

void K3bDivxAVSet::updateView( ){
    QTime t = m_data->getTime();
    m_lengthSecs = t.hour()*3600 + t.minute()*60 + t.second();
    slotCalcBitrate();
}

void K3bDivxAVSet::slotCalcBitrate(){
     int sizeIndex = m_comboCd->currentItem();
     int aBitrateIndex = m_comboMp3->currentItem();
     kdDebug() << "(K3bDivxAVSet) VIndex: " <<  sizeIndex << "AIndex: " << aBitrateIndex << endl;
     if( m_lengthSecs < 1 ){
         kdDebug() << "(K3bDivxAVSet) Fatal error: no video length." << endl;
     }
     long vBitrate = ( finalSize[ sizeIndex ] / m_lengthSecs * 8 ) - audioBitrate[ aBitrateIndex ];
     m_vBitrate->setText( m_vBitrateDesc + QString::number( vBitrate/1000 ) + " kbits" );
     m_data->setVideoBitrate( vBitrate/1000 );
     m_data->setAudioBitrate( audioBitrate[ aBitrateIndex ]/1000 );
}

void K3bDivxAVSet::slotCodecChanged( int codecIndex ){
     m_data->setCodec( codec[ codecIndex ] );
}

void K3bDivxAVSet::slotModeChanged( int id ){
    // id = 0 is 1pass
    m_data->setCodecMode( id + 1);
}

void K3bDivxAVSet::init(){
    m_data->setCodecMode( 2 ); // button 1 is set in setupGUI = 2pass
    m_data->setCodec( codec[ 0 ] ); // default in combobox = first entry
    //updateView();
    //slotCalcBitrate();
}

#include "k3bdivxavset.moc"
