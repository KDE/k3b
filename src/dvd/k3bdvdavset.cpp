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

#include "k3bdvdavset.h"
#include "k3bdvdcodecdata.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qhbuttongroup.h>
#include <qradiobutton.h>

#include <klocale.h>
#include <kdialog.h>
#include <kcombobox.h>

K3bDvdAVSet::K3bDvdAVSet(QWidget *parent, const char *name ) : K3bDivXDataGui( parent,name ) {
     setupGui();
}
K3bDvdAVSet::~K3bDvdAVSet(){
}

void K3bDvdAVSet::setupGui(){
    setColumnLayout(0, Qt::Vertical );
    setTitle( i18n( "Basic Audio/Video Settings" ) );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );

    QLabel *cds = new QLabel( i18n("CDs:"), this );
    QLabel *mp3bitrate = new QLabel( i18n("MP3 Bitrate:"), this );
    QLabel *codec = new QLabel( i18n("Video codec:"), this );
    QLabel *codecmode = new QLabel( i18n("Codec mode:"), this );
    QLabel *bitrate = new QLabel( i18n("Bitrate: "), this );

    m_comboCd = new KComboBox( false, this );
    m_comboCd->insertItem( i18n("1 * 650 MB" ) );
    m_comboCd->insertItem( i18n("1 * 700 MB" ) );
    m_comboCd->insertItem( i18n("2 * 650 MB" ) );
    m_comboCd->insertItem( i18n("2 * 700 MB" ) );
    m_comboMp3 = new KComboBox( false, this );
    m_comboMp3->insertItem( " 96 kbits" );
    m_comboMp3->insertItem( "128 kbits" );
    m_comboMp3->insertItem( "160 kbits" );
    m_comboMp3->insertItem( "192 kbits" );
    m_comboCodec = new KComboBox( false, this );
    m_comboCodec->insertItem( "XviD" );
    m_comboCodec->insertItem( "DivX4" );

    QHButtonGroup *modeGroup = new QHButtonGroup( this );
    m_buttonOnePass = new QRadioButton( i18n("1-pass"), modeGroup );
    m_buttonTwoPass = new QRadioButton( i18n("2-pass"), modeGroup );
    modeGroup->insert( m_buttonOnePass  );
    modeGroup->insert( m_buttonTwoPass  );

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    mainLayout->addMultiCellWidget( cds, 0, 0, 0, 0);
    mainLayout->addMultiCellWidget( mp3bitrate, 1, 1, 0, 0);
    mainLayout->addMultiCellWidget( codec, 2, 2, 0, 0);
    mainLayout->addMultiCellWidget( codecmode, 3, 3, 0, 0);
    mainLayout->addMultiCellWidget( bitrate, 0, 0, 2, 2);
    mainLayout->addMultiCellWidget( m_comboCd, 0, 0, 1, 1);
    mainLayout->addMultiCellWidget( m_comboMp3, 1, 1, 1, 1);
    mainLayout->addMultiCellWidget( m_comboCodec, 2, 2, 1, 1);
    mainLayout->addMultiCellWidget( modeGroup, 3, 3, 1, 2);
    mainLayout->addItem( spacer, 4, 1);

}

void K3bDvdAVSet::updateData( K3bDvdCodecData *data){
    QTime t = data->getTime();
    m_lengthSecs = t.hour()*3600 + t.minute()*60 + t.second();
}

#include "k3bdvdavset.moc"
