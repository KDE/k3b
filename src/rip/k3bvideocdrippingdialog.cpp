/*
*
* $Id$
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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


// kde include
#include <klocale.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kurlrequester.h>
#include <kdebug.h>
#include <kmessagebox.h>

// qt includes
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <qhbox.h>

// k3b includes
#include "k3bvideocdrippingdialog.h"
#include "k3bvideocdrip.h"

#include <k3bjobprogressdialog.h>
#include <k3bcore.h>
#include <k3bglobals.h>
#include <k3bstdguiitems.h>

K3bVideoCdRippingDialog::K3bVideoCdRippingDialog( K3bVideoCdRippingOptions* options, QWidget* parent, const char* name )
        : K3bInteractionDialog( parent, name ), m_videooptions( options )
{
    setupGui();
    setupContextHelp();

    setTitle( i18n( "Video CD Ripping" ) );
}


K3bVideoCdRippingDialog::~K3bVideoCdRippingDialog()
{
}


void K3bVideoCdRippingDialog::setupGui()
{
    QWidget * frame = mainWidget();
    QGridLayout* MainLayout = new QGridLayout( frame );
    MainLayout->setSpacing( KDialog::spacingHint() );
    MainLayout->setMargin( 0 );

    // ---------------------------------------------------- Directory group ---
    QGroupBox* groupDirectory = new QGroupBox( 0, Qt::Vertical, i18n( "Destination Directory" ), frame );
    groupDirectory->layout() ->setSpacing( KDialog::spacingHint() );
    groupDirectory->layout() ->setMargin( KDialog::marginHint() );

    QGridLayout* groupDirectoryLayout = new QGridLayout( groupDirectory->layout() );
    groupDirectoryLayout->setAlignment( Qt::AlignTop );

    QLabel* rippathLabel = new QLabel( i18n( "Rip files to:" ), groupDirectory );
    m_editDirectory = new KURLRequester( groupDirectory, "m_editDirectory" );
    m_editDirectory->setURL( QDir::homeDirPath() );
    m_editDirectory->setMode( KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly );

    rippathLabel->setBuddy( m_editDirectory );

    QHBox* freeSpaceBox = new QHBox( groupDirectory );
    freeSpaceBox->setSpacing( KDialog::spacingHint() );
    ( void ) new QLabel( i18n( "Free space in directory:" ), freeSpaceBox, "FreeSpaceLabel" );
    m_labelFreeSpace = new QLabel( "                       ", freeSpaceBox, "m_labelFreeSpace" );
    m_labelFreeSpace->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

    QHBox* necessarySizeBox = new QHBox( groupDirectory );
    necessarySizeBox->setSpacing( KDialog::spacingHint() );
    ( void ) new QLabel( i18n( "Necessary storage size:" ), necessarySizeBox, "StorSize" );
    m_labelNecessarySize = new QLabel( "                        ", necessarySizeBox, "m_labelNecessarySize" );
    m_labelNecessarySize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );


    groupDirectoryLayout->addWidget( rippathLabel, 0, 0 );
    groupDirectoryLayout->addWidget( m_editDirectory, 0, 1 );
    groupDirectoryLayout->addWidget( freeSpaceBox, 1, 1 );
    groupDirectoryLayout->addWidget( necessarySizeBox, 2, 1 );

    // ---------------------------------------------------- Options group ---
    QGroupBox* groupOptions = new QGroupBox( 4, Qt::Vertical, i18n( "Options" ), frame );

    m_ignoreExt = new QCheckBox( i18n( "ignore /EXT/PSD_X.VCD" ), groupOptions );

    m_sector2336 = new QCheckBox( i18n( "use 2336 byte sector mode for image file" ), groupOptions );
    // Only available for image file ripping
    m_sector2336->setEnabled( false );
    m_sector2336->setChecked( false );

    m_extractXML = new QCheckBox( i18n( "extract XML structure" ), groupOptions );


    MainLayout->addWidget( groupDirectory, 0, 0 );
    MainLayout->addWidget( groupOptions, 1, 0 );
    MainLayout->setRowStretch( 0, 1 );

    setStartButtonText( i18n( "Start Ripping" ), i18n( "Starts extracting the selected VideoCd tracks" ) );
    // ----------------------------------------------------------------------------------

    connect( m_editDirectory, SIGNAL(textChanged(const QString&)), this, SLOT(slotUpdateFreeSpace()) );

    slotLoadUserDefaults();

    m_labelNecessarySize ->setText( KIO::convertSize( m_videooptions ->getVideoCdSize() ) );    
}


void K3bVideoCdRippingDialog::setupContextHelp()
{
    QToolTip::add( m_labelFreeSpace, i18n("Free space on destination directory: %1").arg( m_editDirectory ->url() ) );

    QToolTip::add( m_labelNecessarySize, i18n("Necessary space for extracted files") );

    QToolTip::add( m_ignoreExt, i18n("Ignore extended PSD") );
    QWhatsThis::add( m_ignoreExt, i18n("<p>Ignore extended PSD (located in the ISO-9660 filesystem under `/EXT/PSD_X.VCD') and use the <em>standard</em> PSD.</p>") );

    QToolTip::add( m_sector2336, i18n("Assume a 2336-byte sector mode") );
    QWhatsThis::add( m_sector2336, i18n("<p>This option only makes sense if you are reading from a BIN CD disk image. This indicates to `vcdxrip' to assume a 2336-byte sector mode for image file.</p>"
                                                            "<b>Note: This option is slated to disappear.</b>") );

    QToolTip::add( m_extractXML, i18n("Create XML description file.") );
    QWhatsThis::add( m_extractXML, i18n("<p>This option creates an XML description file with all video CD information.</p>"
					"<p>This file will always contain all of the information.</p>"
					"<p>Example: If you only extract sequences, the description file will also hold the information for files and segments.</p>"
					"<p>The filename is the same as the video CD name, with a .xml extension. The default is VIDEOCD.xml.</p>") );
}

void K3bVideoCdRippingDialog::slotStartClicked()
{

    QStringList filesExists;
    QDir d;
    d.setPath( m_editDirectory ->url() );
    const QFileInfoList* list = d.entryInfoList();
    QFileInfoListIterator it( *list );
    QFileInfo* fi;
    while ( ( fi = it.current() ) != 0 ) {
        if ( fi ->fileName() != "." && fi ->fileName() != ".." )
            filesExists.append( QString( "%1 (%2)" ).arg( QFile::encodeName( fi ->fileName() ) ).arg( KIO::convertSize( fi ->size() ) ) );
        ++it;
    }

    if( !filesExists.isEmpty() )
        if( KMessageBox::questionYesNoList( this,
                                i18n("Continue although the folder is not empty?"),
                                filesExists,
                                i18n("Files exist") ) == KMessageBox::No )
        return;

    m_videooptions ->setVideoCdIgnoreExt( m_ignoreExt ->isChecked() );
    m_videooptions ->setVideoCdSector2336( m_sector2336 ->isChecked() );
    m_videooptions ->setVideoCdExtractXml( m_extractXML ->isChecked() );
    m_videooptions ->setVideoCdDestination( m_editDirectory ->url() );

    K3bJobProgressDialog ripDialog( kapp->mainWidget(), "Ripping" );
    K3bVideoCdRip * rip = new K3bVideoCdRip( &ripDialog, m_videooptions );

    hide();
    ripDialog.startJob( rip );

    delete rip;

    close();
}

void K3bVideoCdRippingDialog::slotFreeSpace(const QString&,
						  unsigned long,
						  unsigned long,
						  unsigned long kbAvail)
{
    m_labelFreeSpace->setText( KIO::convertSizeFromKB(kbAvail) );

    m_freeSpace = kbAvail;

    if( m_freeSpace < m_videooptions ->getVideoCdSize() /1024 )
        m_labelNecessarySize->setPaletteForegroundColor( red );
    else
        m_labelNecessarySize->setPaletteForegroundColor( m_labelFreeSpace->paletteForegroundColor() );

    QTimer::singleShot( 1000, this, SLOT(slotUpdateFreeSpace()) );
}


void K3bVideoCdRippingDialog::slotUpdateFreeSpace()
{
    QString path = m_editDirectory->url();

    if( !QFile::exists( path ) )
        path.truncate( path.findRev('/') );

    unsigned long size, avail;
    if( K3b::kbFreeOnFs( path, size, avail ) )
        slotFreeSpace( path, size, 0, avail );
    else
        m_labelFreeSpace->setText("-");
}

void K3bVideoCdRippingDialog::slotLoadK3bDefaults()
{
    m_editDirectory->setURL( QDir::homeDirPath() );
    m_ignoreExt ->setChecked( false );
    m_sector2336 ->setChecked( false );
    m_extractXML ->setChecked( false );

    slotUpdateFreeSpace();
}

void K3bVideoCdRippingDialog::slotLoadUserDefaults()
{
    KConfig* c = k3bcore->config();
    c->setGroup( "Video CD Ripping" );

    m_editDirectory ->setURL( c->readPathEntry( "last ripping directory", QDir::homeDirPath() ) );
    m_ignoreExt ->setChecked( c->readBoolEntry( "ignore ext", false ) );
    m_sector2336 ->setChecked( c->readBoolEntry( "sector 2336", false ) );
    m_extractXML ->setChecked( c->readBoolEntry( "extract xml", false ) );

    slotUpdateFreeSpace();
}

void K3bVideoCdRippingDialog::slotSaveUserDefaults()
{
    KConfig* c = k3bcore->config();
    c->setGroup( "Video CD Ripping" );

    c->writePathEntry( "last ripping directory", m_editDirectory->url() );
    c->writeEntry( "ignore ext", m_ignoreExt ->isChecked( ) );
    c->writeEntry( "sector 2336", m_sector2336 ->isChecked( ) );
    c->writeEntry( "extract xml", m_extractXML ->isChecked( ) );
}

#include "k3bvideocdrippingdialog.moc"
