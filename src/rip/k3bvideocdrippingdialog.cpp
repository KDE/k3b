/*
 *
 * $Id$
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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


// kde include
#include <kcombobox.h>
#include <klocale.h>
#include <kapplication.h>
#include <kconfig.h>
#include <klistview.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <kio/global.h>
#include <kiconloader.h>
#include <kstdguiitem.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kurllabel.h>

// qt includes
#include <qgroupbox.h>
#include <qheader.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qmessagebox.h>
#include <qfont.h>
#include <qhbox.h>
#include <qtoolbutton.h>
#include <qtabwidget.h>
#include <qspinbox.h>
#include <qptrlist.h>
#include <qintdict.h>
#include <qpair.h>
#include <qvalidator.h>

// k3b includes
#include "k3bvideocdrippingdialog.h"
#include "k3bvideocdrip.h"

#include <k3bjobprogressdialog.h>
#include <k3bcore.h>
#include <k3bglobals.h>
#include <device/k3btrack.h>
#include <k3bstdguiitems.h>
#include <k3btempdirselectionwidget.h>

K3bVideoCdRippingDialog::K3bVideoCdRippingDialog( const QString ripsource, const long size, QWidget* parent, const char* name )
  : K3bInteractionDialog( parent, name ), m_videocdsize(size), m_ripsource( ripsource )
{
  setupGui();
  setupContextHelp();
  
  setTitle( i18n("VideoCd Ripping") );
}


K3bVideoCdRippingDialog::~K3bVideoCdRippingDialog()
{
}


void K3bVideoCdRippingDialog::setupGui()
{
    QWidget* frame = mainWidget();
    QGridLayout* MainLayout = new QGridLayout( frame );
    MainLayout->setSpacing( KDialog::spacingHint() );
    MainLayout->setMargin( 0 );

    // ---------------------------------------------------- Directory group ---
    QGroupBox* groupDirectory = new QGroupBox( 0, Qt::Vertical, i18n( "Destination Directory" ), frame );
    groupDirectory->layout()->setSpacing( KDialog::spacingHint() );
    groupDirectory->layout()->setMargin( KDialog::marginHint() );
    
    QGridLayout* groupDirectoryLayout = new QGridLayout( groupDirectory->layout() );
    groupDirectoryLayout->setAlignment( Qt::AlignTop );

    QLabel* rippathLabel = new QLabel( i18n("Rip files to:"), groupDirectory );
    m_editDirectory = new KURLRequester( groupDirectory, "m_editDirectory" );
    m_editDirectory->setURL( QDir::homeDirPath() );
    m_editDirectory->setMode( KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly );
        
    rippathLabel->setBuddy( m_editDirectory );

    QHBox* freeSpaceBox = new QHBox( groupDirectory );
    freeSpaceBox->setSpacing( KDialog::spacingHint() );
    (void)new QLabel( i18n( "Free space in directory:" ), freeSpaceBox, "FreeSpaceLabel" );
    m_labelFreeSpace = new QLabel( "                       ",freeSpaceBox, "m_labelFreeSpace" );
    m_labelFreeSpace->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

    QHBox* necessarySizeBox = new QHBox( groupDirectory );
    necessarySizeBox->setSpacing( KDialog::spacingHint() );
    (void)new QLabel( i18n( "Necessary storage size:" ), necessarySizeBox, "StorSize" );
    m_labelNecessarySize = new QLabel( "                        ", necessarySizeBox, "m_labelNecessarySize" );
    m_labelNecessarySize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );


    groupDirectoryLayout->addWidget( rippathLabel, 0, 0 );
    groupDirectoryLayout->addWidget( m_editDirectory, 0, 1 );
//    groupDirectoryLayout->addWidget( m_labelFreeSpace, 1, 0 );
    groupDirectoryLayout->addWidget( freeSpaceBox, 1, 1 );
//    groupDirectoryLayout->addWidget( m_labelNecessarySize, 2, 0 );
    groupDirectoryLayout->addWidget( necessarySizeBox, 2, 1 );

    // ---------------------------------------------------- Options group ---
    QGroupBox* groupOptions = new QGroupBox( 4, Qt::Vertical, i18n( "Options" ), frame );

    m_ignoreExt = new QCheckBox( i18n( "ignore /EXT/PSD_X.VCD" ), groupOptions );

    m_sector2336 = new QCheckBox( i18n( "use 2336 byte sector mode for image file" ), groupOptions );
    // Only available for image file ripping
    m_sector2336->setEnabled( false );
    m_sector2336->setChecked( false );

    m_extractXML = new QCheckBox( i18n( "extract XML structure" ), groupOptions );


    MainLayout->addWidget( groupDirectory, 0, 0);
    MainLayout->addWidget( groupOptions, 1, 0);
    MainLayout->setRowStretch( 0, 1 );
                
    setStartButtonText( i18n( "Start Ripping" ), i18n( "Starts copying the selected VideoCd tracks") );
    // ----------------------------------------------------------------------------------

}


void K3bVideoCdRippingDialog::setupContextHelp()
{
}

void K3bVideoCdRippingDialog::slotStartClicked()
{

  K3bVideoCdRip* rip = new K3bVideoCdRip();
  rip->setRipSource( m_ripsource );
  rip->setDestination( m_editDirectory->url() );
  rip->setVideoCdSize(m_videocdsize);
  
  K3bJobProgressDialog ripDialog( kapp->mainWidget(), "Ripping" );

  hide();
  ripDialog.startJob( rip );

  delete rip;

  close();
}

void K3bVideoCdRippingDialog::slotLoadK3bDefaults()
{
}

void K3bVideoCdRippingDialog::slotLoadUserDefaults()
{
}

void K3bVideoCdRippingDialog::slotSaveUserDefaults()
{
}

void K3bVideoCdRippingDialog::slotToggleAll()
{
}

#include "k3bvideocdrippingdialog.moc"
