/***************************************************************************
                          k3bdvddirectories.cpp  -  description
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

#include "k3bdvddirectories.h"
#include "k3bdivxdatagui.h"
#include "k3bdvdcodecdata.h"

#include <qpushbutton.h>
#include <qlayout.h>
#include <qdir.h>

#include <klineedit.h>
#include <klocale.h>
#include <kdialog.h>
#include <kfiledialog.h>


K3bDvdDirectories::K3bDvdDirectories( QWidget *parent, const char *name) : K3bDivXDataGui( parent, name ){
    setupGui();
}

K3bDvdDirectories::~K3bDvdDirectories(){
}

void K3bDvdDirectories::setupGui(){
    setColumnLayout(0, Qt::Vertical );
    setTitle( i18n( "Source/Destination Directories" ) );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );

    m_editVideoPath = new KLineEdit( this, "videopath");
    m_editVideoPath->setText( QDir::homeDirPath() );
    m_buttonVideoDir = new QPushButton( this, "m_buttonVidoeDir" );
    m_buttonVideoDir->setText( tr( "..." ) );

    m_editAudioPath = new KLineEdit( this, "audiopath");
    m_editAudioPath->setText( QDir::homeDirPath() );
    m_buttonAudioDir = new QPushButton( this, "m_buttonAudioDir" );
    m_buttonAudioDir->setText( tr( "..." ) );

    m_editAviPath = new KLineEdit( this, "Avipath");
    m_editAviPath->setText( QDir::homeDirPath() );

    m_buttonAviDir = new QPushButton( this, "m_buttonAviDir" );
    m_buttonAviDir->setText( tr( "..." ) );
    // TODO
    m_editAudioPath->setEnabled( false );
    m_buttonAudioDir->setEnabled( false );

    mainLayout->addMultiCellWidget( m_editVideoPath, 0, 0, 0, 1);
    mainLayout->addMultiCellWidget( m_buttonVideoDir, 0, 0, 2, 2);
    mainLayout->addMultiCellWidget( m_editAudioPath, 1, 1, 0, 1);
    mainLayout->addMultiCellWidget( m_buttonAudioDir, 1, 1, 2, 2);
    mainLayout->addMultiCellWidget( m_editAviPath, 2, 2, 0, 1);
    mainLayout->addMultiCellWidget( m_buttonAviDir, 2, 2, 2, 2);

    connect( m_buttonVideoDir, SIGNAL( clicked() ), this, SLOT( slotVideoClicked() ) );
    connect( m_buttonAviDir, SIGNAL( clicked() ), this, SLOT( slotAviClicked() ) );
    connect( m_buttonAudioDir, SIGNAL( clicked() ), this, SLOT( slotAudioClicked() ) );
}

void K3bDvdDirectories::updateData( K3bDvdCodecData *data ){
      qDebug("(K3bDvdDirectories) update data: %s", m_editVideoPath->text().latin1() );
      data->setProjectFile( m_editVideoPath->text() );
      data->setAviFile( m_editAviPath->text() );
}

void K3bDvdDirectories::slotVideoClicked(){
    QString path = KFileDialog::getOpenFileName( m_editVideoPath->text(), "*.xml", this, i18n("Select project file") );
    if( !path.isEmpty() ) {
        m_editVideoPath->setText( path );
    }
    emit dataChanged( this );
}

void K3bDvdDirectories::slotAviClicked(){
    QString path = KFileDialog::getOpenFileName( m_editAviPath->text(), "*.avi", this, i18n("Select AVI file") );
    if( !path.isEmpty() ) {
        m_editAviPath->setText( path );
    }
    emit dataChanged( this );
}
void K3bDvdDirectories::slotAudioClicked(){
}

#include "k3bdvddirectories.moc"
