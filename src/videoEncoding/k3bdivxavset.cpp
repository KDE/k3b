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
#include <qwhatsthis.h>

#include <klocale.h>
#include <kdialog.h>
#include <kcombobox.h>
#include <kdebug.h>

static long finalSize[] = { 681574400L, 734000320L, 1363148800L, 1468006400L };
static long audioBitrate[] = { 64000L, 96000L, 112000L, 128000L, 160000L, 192000L };
// order must be in sync with comboBox entries
static QString codec[] = { "xvid", "divx4", "divx5" };

K3bDivxAVSet::K3bDivxAVSet(K3bDivxCodecData *data, QWidget *parent, const char *name ) : QGroupBox( parent,name ) {
    m_data = data;
    m_lengthSecs = 0;
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
    QString wt_cd( i18n("Select how many CDs the final encoded video should be. You can select CDRs with size of 650MB and 700MB."));
    QWhatsThis::add( cds, wt_cd );
    QLabel *mp3bitrate = new QLabel( i18n("MP3 Bitrate:"), this );
    QString wt_mp3( i18n("Select bitrate of the audio track. MP3 will encode with constant bitrate and joint stereo."));
    QWhatsThis::add( mp3bitrate, wt_mp3 );
    QLabel *codec = new QLabel( i18n("Video codec:"), this );
    QString wt_codec( i18n("Select the video codec to encode to the final movie. XviD (www.xvid.org) is an Open Source codec and \
has similar features than DivX5. DivX4 is the predecessor of DivX5. All three codecs support 1-pass and 2-pass encoding. \
About quality, well, first try all codec and find your favorite. Second you may read the various forums out there about MPEG-4 Encoding \
(www.doom9.org, www.xvid.org, www.divx.net, ... ).  And third you may find out that the difference between a DivX4 and XviD 2-pass encoded movie \
is really tiny and one time DivX4 (smoother) is better and the other time XviD (sharper). Some people reported that DivX5 is currently not as good as the other two. \
I haven't do any tests with DivX5."));
    QWhatsThis::add( codec, wt_codec );
    QLabel *codecmode = new QLabel( i18n("Codec mode:"), this );
    QString wt_codecmode( i18n("Select the mode for video encoding. 1-pass encoding has worse quality than 2-pass, but needs half time to encode a video.\
In 2-pass mode the video will be encoded twice. The first time the video will only be analysed to get the best quality in the second encoding pass."));
    QWhatsThis::add( codecmode, wt_codecmode );
    m_vBitrateDesc = i18n("Bitrate: ");
    m_vBitrate = new QLabel( m_vBitrateDesc, this );

    m_comboCd = new KComboBox( false, this );
    m_comboCd->insertItem( i18n("1 * 650 MB" ) );
    m_comboCd->insertItem( i18n("1 * 700 MB" ) );
    m_comboCd->insertItem( i18n("2 * 650 MB" ) );
    m_comboCd->insertItem( i18n("2 * 700 MB" ) );
    QWhatsThis::add( m_comboCd, wt_cd );
    m_comboMp3 = new KComboBox( false, this );
    m_comboMp3->insertItem( i18n(" 64 kbits") );
    m_comboMp3->insertItem( i18n(" 96 kbits") );
    m_comboMp3->insertItem( i18n("112 kbits") );
    m_comboMp3->insertItem( i18n("128 kbits") );
    m_comboMp3->insertItem( i18n("160 kbits") );
    m_comboMp3->insertItem( i18n("192 kbits") );
    QWhatsThis::add( m_comboMp3, wt_mp3 );
    m_comboCodec = new KComboBox( false, this );
    m_comboCodec->insertItem( "XviD" );
    m_comboCodec->insertItem( "DivX4" );
    m_comboCodec->insertItem( "DivX5" );
    QWhatsThis::add( m_comboCodec, wt_codec );

    QHButtonGroup *modeGroup = new QHButtonGroup( this );
    modeGroup->layout()->setSpacing( KDialog::spacingHint() );
    modeGroup->layout()->setMargin( KDialog::marginHint() );
    modeGroup->setFrameStyle( Plain | NoFrame );
    m_buttonOnePass = new QRadioButton( i18n("1-pass"), modeGroup );
    m_buttonTwoPass = new QRadioButton( i18n("2-pass"), modeGroup );
    modeGroup->setButton( 1 );
    QWhatsThis::add( modeGroup, wt_codecmode );

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

void K3bDivxAVSet::init(){
    m_data->setCodecMode( 2 ); // button 1 is set in setupGUI = 2pass
    m_data->setCodec( codec[ 0 ] ); // default in combobox = first entry
    m_comboCd->setCurrentItem( 1 ); // 1*700MB128 kbits
    m_comboMp3->setCurrentItem( 3 ); // 128 kbits
    m_data->setAudioBitrate( 128 );
    //updateView();
    slotCalcBitrate();
}
void K3bDivxAVSet::slotCalcBitrate(){
     int sizeIndex = m_comboCd->currentItem();
     int aBitrateIndex = m_comboMp3->currentItem();
     if( m_lengthSecs < 1 ){
         kdDebug() << "(K3bDivxAVSet) Fatal error: no video length. You must load an project file" << endl;
         return;
     }
     long vBitrate = ( finalSize[ sizeIndex ] / m_lengthSecs * 8 ) *1.024 - audioBitrate[ aBitrateIndex ]; // one correct 1.024 K->1024
     m_vBitrate->setText( m_vBitrateDesc + i18n("%1 kbits").arg(vBitrate/1000) );
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


#include "k3bdivxavset.moc"
