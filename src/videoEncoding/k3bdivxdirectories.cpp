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


#include "k3bdivxdirectories.h"
#include "k3bdivxcodecdata.h"

#include <qtoolbutton.h>
#include <qlayout.h>
#include <qdir.h>
#include <qlabel.h>

#include <klineedit.h>
#include <kcompletion.h>
#include <kcompletionbox.h>
#include <klocale.h>
#include <kdialog.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kdiroperator.h>


K3bDivxDirectories::K3bDivxDirectories( K3bDivxCodecData *data, QWidget *parent, const char *name) 
  : QGroupBox( parent, name )
{
  m_data = data;
  setupGui();
  init();
}

K3bDivxDirectories::~K3bDivxDirectories()
{
}

void K3bDivxDirectories::setupGui(){
    setColumnLayout(0, Qt::Vertical );
    layout()->setSpacing( 0 );
    layout()->setMargin( 0 );
    setTitle( i18n( "Source/Destination Directories" ) );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );

    QLabel *video = new QLabel( i18n("K3b DVD ripping file:"), this );
    m_editVideoPath = new KLineEdit( this, "videopath");
    m_editVideoPath->setText( QDir::homeDirPath() );
    m_buttonVideoDir = new QToolButton( this, "m_buttonVidoeDir" );
    m_buttonVideoDir->setIconSet( SmallIconSet( "fileopen" ) );

    QLabel *audio = new QLabel( i18n("External audio file:"), this );
    m_editAudioPath = new KLineEdit( this, "audiopath");
    m_editAudioPath->setText( QDir::homeDirPath() );
    m_buttonAudioDir = new QToolButton( this, "m_buttonAudioDir" );
    m_buttonAudioDir->setIconSet( SmallIconSet( "fileopen" ) );

    QLabel *avi = new QLabel( i18n("Final AVI file name:"), this );
    m_editAviPath = new KLineEdit( this, "Avipath");
    m_editAviPath->setText( QDir::homeDirPath() );

    m_buttonAviDir = new QToolButton(  this, "m_buttonAviDir" );
    m_buttonAviDir->setIconSet( SmallIconSet( "filesave" ) );
    // TODO
    m_editAudioPath->setEnabled( false );
    m_buttonAudioDir->setEnabled( false );

    mainLayout->addMultiCellWidget( video, 0, 0, 0, 2);
    mainLayout->addMultiCellWidget( m_editVideoPath, 1, 1, 0, 1);
    mainLayout->addMultiCellWidget( m_buttonVideoDir, 1, 1, 2, 2);
    mainLayout->addMultiCellWidget( audio, 2, 2, 0, 2);
    mainLayout->addMultiCellWidget( m_editAudioPath, 3, 3, 0, 1);
    mainLayout->addMultiCellWidget( m_buttonAudioDir, 3, 3, 2, 2);
    mainLayout->addMultiCellWidget( avi, 4, 4, 0, 2);
    mainLayout->addMultiCellWidget( m_editAviPath, 5, 5, 0, 1);
    mainLayout->addMultiCellWidget( m_buttonAviDir, 5, 5, 2, 2);
    mainLayout->setColStretch( 0, 20 );

    //m_ops = new KDirOperator("/home/ft0001", this, "K3bDivxdirectorys::ops");
    //ops->setOnlyDoubleClickSelectsFiles( true );
    m_editVideoPath->setHandleSignals( true );
    //(void) m_editVideoPath->completionBox();

    //m_editVideoPath->setFocus();
//     locationEdit->setCompletionObject( new KURLCompletion() );
//     locationEdit->setAutoDeleteCompletionObject( true );
    //m_editVideoPath->setCompletionObject( m_ops->completionObject(), false );

    //connect( m_editVideoPath, SIGNAL( returnPressed() ),
    //         this, SLOT( slotOk()));
    //connect(m_editVideoPath, SIGNAL( activated( const QString&  )),
    //        this,  SLOT( locationActivated( const QString& ) ));
    //connect( m_editVideoPath, SIGNAL( completion( const QString& )),
    //         SLOT( fileCompletion( const QString& )));
    //connect(m_editVideoPath, SIGNAL( activated( const QString&  )),
    //        this,  SLOT( slotCompletion( const QString& ) ));
    connect( m_editVideoPath, SIGNAL( completion( const QString& )),
             SLOT( slotCompletion( const QString& )));

    //connect( m_editVideoPath, SIGNAL( textRotation(KCompletionBase::KeyBindingType) ),
    //         m_editVideoPath, SLOT( rotateText(KCompletionBase::KeyBindingType) ));


    //m_completionBox = m_editVideoPath->completionBox();
    (void) m_editVideoPath->completionObject();
    m_editVideoPath->setCompletionMode( KGlobalSettings::CompletionAuto );
    //connect(m_editVideoPath,SIGNAL(completion(const QString&)),this,SLOT(slotCompletion(const QString&)));

    connect( m_buttonVideoDir, SIGNAL( clicked() ), this, SLOT( slotVideoClicked() ) );
    connect( m_buttonAviDir, SIGNAL( clicked() ), this, SLOT( slotAviClicked() ) );
    connect( m_editVideoPath, SIGNAL( textChanged( const QString& )), this, SLOT( slotVideoEdited( const QString& ) ) );
    connect( m_editAviPath, SIGNAL( textChanged( const QString& ) ), this, SLOT( slotAviEdited( const QString& ) ) );

    connect( m_buttonAudioDir, SIGNAL( clicked() ), this, SLOT( slotAudioClicked() ) );
}

void K3bDivxDirectories::init(){
    if( m_data->getProjectFile().length() > 1 ){
        m_editVideoPath->setText( m_data->getProjectFile() );
    }
    if( !m_data->getAviFile().isEmpty() ){
        m_editAviPath->setText( m_data->getAviFile() );
    }
}

void K3bDivxDirectories::slotVideoClicked(){
    QString path = KFileDialog::getOpenFileName( m_editVideoPath->text(), "*.xml", this, i18n("Select Project File") );
    if( !path.isEmpty() ) {
        m_editVideoPath->setText( path );
        emit dataChanged( );
    }
}
void K3bDivxDirectories::slotAviClicked(){
    QString aviName = KFileDialog::getSaveFileName( m_editAviPath->text(), "*.avi", this, i18n("Save Video As") );
    if( !aviName.isEmpty() ) {
        if( !aviName.endsWith(".avi") ){
            aviName += ".avi";
        }
        m_editAviPath->setText( aviName );
        m_data->setAviFile( m_editAviPath->text() );
    }
    if( m_data->getProjectDir().length() > 1 ){
        emit dataChanged( );
    }
}
void K3bDivxDirectories::slotCompletion( const QString& st ){
    QString s = st;
    QStringList list;
    QDir testDir(s);
    if( testDir.exists() ){
        testDir.setNameFilter("k3bDVDRip.xml");
        testDir.setMatchAllDirs(true);
        list = testDir.entryList();
        list.remove( list.first() );
        list.remove( list.first() );
    } else {
       QString search = s.right( s.length()-s.findRev("/")-1 );
       s = s.mid( 0, s.findRev("/"));
       QDir d(s);
       d.setNameFilter(search +"*;k3bDVDRip.xml");
       list = d.entryList();
    }
    if( !s.endsWith("/"))
        s = s + "/";
    if (!list.isEmpty() ){
        m_editVideoPath->setCompletedText( s + list[0] );
   }
}

void K3bDivxDirectories::slotAudioClicked(){
}

void K3bDivxDirectories::slotVideoEdited( const QString& text){
    if( text == m_data->getProjectFile() )
        return;
    QDir testDir(text);
    if( testDir.exists() ){
        testDir.setMatchAllDirs(true);
    }
    if( testDir.exists() ){
        kdDebug() << "(K3bDivxDirectories) Directory exists: " << testDir.path() << endl;
    } else if( QFile::exists(text) ){
        kdDebug() << "(K3bDivxDirectories) File exists: " << text << endl;
        m_data->setProjectFile( text );
        if( !m_data->projectLoaded() ){
            KMessageBox::error( this, i18n("Error loading project"), i18n("Error while parsing file: %1").arg(text) );
            m_data->setProjectFile( "" );
            return;
        }
        emit dataChanged( );
    }
}

void K3bDivxDirectories::slotAviEdited( const QString& text){
    m_data->setAviFile( text );
    if( m_data->getProjectDir().length() > 1 ){
        emit dataChanged( );
    }
}



#include "k3bdivxdirectories.moc"
