/*
*
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
* Copyright (C) 2008-2009 Sebastian Trueg <trueg@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* See the file "COPYING" for the exact licensing terms.
*/

#include "k3bvideocdrippingdialog.h"
#include "k3bvideocdrip.h"
#include "k3bjobprogressdialog.h"
#include "k3bcore.h"
#include "k3bglobals.h"
#include "k3bstdguiitems.h"

#include <KColorScheme>
#include <KConfig>
#include <KLocalizedString>
#include <KUrlRequester>
#include <KMessageBox>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QTimer>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QToolTip>


K3b::VideoCdRippingDialog::VideoCdRippingDialog( K3b::VideoCdRippingOptions* options, QWidget* parent )
    : K3b::InteractionDialog( parent,
                            i18n( "Video CD Ripping" ),
                            QString(),
                            START_BUTTON|CANCEL_BUTTON,
                            START_BUTTON,
                            "Video CD Ripping" ), // config group
      m_videooptions( options )
{
    setupGui();
    setupContextHelp();
}


K3b::VideoCdRippingDialog::~VideoCdRippingDialog()
{
}


void K3b::VideoCdRippingDialog::setupGui()
{
    QWidget * frame = mainWidget();
    QGridLayout* MainLayout = new QGridLayout( frame );
    MainLayout->setContentsMargins( 0, 0, 0, 0 );

    // ---------------------------------------------------- Directory group ---
    QGroupBox* groupDirectory = new QGroupBox( i18n( "Destination Folder" ), frame );

    QGridLayout* groupDirectoryLayout = new QGridLayout( groupDirectory );
    groupDirectoryLayout->setAlignment( Qt::AlignTop );

    QLabel* rippathLabel = new QLabel( i18n( "Rip files to:" ), groupDirectory );
    m_editDirectory = new KUrlRequester( groupDirectory );
    m_editDirectory->setMode( KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly );

    rippathLabel->setBuddy( m_editDirectory );

    QHBoxLayout* freeSpaceBox = new QHBoxLayout;
    freeSpaceBox->addWidget( new QLabel( i18n( "Free space in folder:" ), groupDirectory ) );
    m_labelFreeSpace = new QLabel( "                       ", groupDirectory );
    m_labelFreeSpace->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    freeSpaceBox->addWidget( m_labelFreeSpace );

    QHBoxLayout* necessarySizeBox = new QHBoxLayout;
    necessarySizeBox->addWidget( new QLabel( i18n( "Necessary storage size:" ), groupDirectory ) );
    m_labelNecessarySize = new QLabel( "                        ", groupDirectory );
    m_labelNecessarySize->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    necessarySizeBox->addWidget( m_labelNecessarySize );

    groupDirectoryLayout->addWidget( rippathLabel, 0, 0 );
    groupDirectoryLayout->addWidget( m_editDirectory, 0, 1 );
    groupDirectoryLayout->addLayout( freeSpaceBox, 1, 1 );
    groupDirectoryLayout->addLayout( necessarySizeBox, 2, 1 );

    // ---------------------------------------------------- Options group ---
    QGroupBox* groupOptions = new QGroupBox( i18n( "Settings" ), frame );
    m_ignoreExt = new QCheckBox( i18n( "Ignore /EXT/PSD_X.VCD" ), groupOptions );
    m_sector2336 = new QCheckBox( i18n( "Use 2336 byte sector mode for image file" ), groupOptions );
    // Only available for image file ripping
    m_sector2336->setEnabled( false );
    m_sector2336->setChecked( false );
    m_extractXML = new QCheckBox( i18n( "Extract XML structure" ), groupOptions );
    QVBoxLayout* groupOptionsLayout = new QVBoxLayout( groupOptions );
    groupOptionsLayout->addWidget( m_ignoreExt );
    groupOptionsLayout->addWidget( m_sector2336 );
    groupOptionsLayout->addWidget( m_extractXML );

    MainLayout->addWidget( groupDirectory, 0, 0 );
    MainLayout->addWidget( groupOptions, 1, 0 );
    MainLayout->setRowStretch( 0, 1 );

    setStartButtonText( i18n( "Start Ripping" ), i18n( "Starts extracting the selected VideoCd tracks" ) );
    // ----------------------------------------------------------------------------------

    connect( m_editDirectory, SIGNAL(textChanged(QString)), this, SLOT(slotUpdateFreeSpace()) );

    m_labelNecessarySize ->setText( KIO::convertSize( m_videooptions ->getVideoCdSize() ) );
}


void K3b::VideoCdRippingDialog::setupContextHelp()
{
    m_labelFreeSpace->setToolTip( i18n("Free space in destination folder: %1", m_editDirectory ->url().url() ) );

    m_labelNecessarySize->setToolTip( i18n("Necessary space for extracted files") );

    m_ignoreExt->setToolTip( i18n("Ignore extended PSD") );
    m_ignoreExt->setWhatsThis( i18n("<p>Ignore extended PSD (located in the ISO 9660 filesystem under `/EXT/PSD_X.VCD') and use the <em>standard</em> PSD.</p>") );

    m_sector2336->setToolTip( i18n("Assume a 2336-byte sector mode") );
    m_sector2336->setWhatsThis( i18n("<p>This option only makes sense if you are reading from a BIN CD disk image. This indicates to `vcdxrip' to assume a 2336-byte sector mode for image file.</p>"
                                     "<b>Note: This option is slated to disappear.</b>") );

    m_extractXML->setToolTip( i18n("Create XML description file.") );
    m_extractXML->setWhatsThis( i18n("<p>This option creates an XML description file with all video CD information.</p>"
                                     "<p>This file will always contain all of the information.</p>"
                                     "<p>Example: If you only extract sequences, the description file will also hold the information for files and segments.</p>"
                                     "<p>The filename is the same as the video CD name, with a .xml extension. The default is VIDEOCD.xml.</p>") );
}

void K3b::VideoCdRippingDialog::slotStartClicked()
{

    QStringList filesExists;
    QDir d;
    d.setPath( m_editDirectory ->url().toLocalFile() );
    if( !d.exists() ) {
        if( KMessageBox::warningYesNo( this, i18n("Image folder '%1' does not exist. Do you want K3b to create it?", m_editDirectory->url().toLocalFile() ) )
            == KMessageBox::Yes ) {
            if( !QDir().mkpath( m_editDirectory->url().toLocalFile() ) ) {
                KMessageBox::error( this, i18n("Failed to create folder '%1'.", m_editDirectory->url().toLocalFile() ) );
                return;
            }
        }
        else {
            return;
        }
    }
    foreach( const QFileInfo& fi, d.entryInfoList() )
    {
        if ( fi.fileName() != "." && fi .fileName() != ".." )
            filesExists.append( QString( "%1 (%2)" ).arg( QString(QFile::encodeName( fi.fileName() )) ).arg( KIO::convertSize( fi.size() ) ) );
    }

    if( !filesExists.isEmpty() )
        if( KMessageBox::questionYesNoList( this,
                                            i18n("Continue although the folder is not empty?"),
                                            filesExists,
                                            i18n("Files Exist"),KStandardGuiItem::cont(),KStandardGuiItem::cancel() ) == KMessageBox::No )
            return;

    m_videooptions ->setVideoCdIgnoreExt( m_ignoreExt ->isChecked() );
    m_videooptions ->setVideoCdSector2336( m_sector2336 ->isChecked() );
    m_videooptions ->setVideoCdExtractXml( m_extractXML ->isChecked() );
    m_videooptions ->setVideoCdDestination( m_editDirectory ->url().toLocalFile() );

    K3b::JobProgressDialog dlg( parentWidget(), "Ripping" );
    K3b::VideoCdRip* job = new K3b::VideoCdRip( &dlg, m_videooptions, &dlg );

    hide();
    dlg.startJob( job );
    close();
}

void K3b::VideoCdRippingDialog::slotFreeSpace(const QString&,
                                            unsigned long,
                                            unsigned long,
                                            unsigned long kbAvail)
{
    m_labelFreeSpace->setText( KIO::convertSizeFromKiB(kbAvail) );

    m_freeSpace = kbAvail;

    const KColorScheme colorScheme( isEnabled() ? QPalette::Normal : QPalette::Disabled, KColorScheme::Window );
    QColor textColor;
    if( m_freeSpace < m_videooptions ->getVideoCdSize() / 1024 )
        textColor = colorScheme.foreground( KColorScheme::NegativeText ).color();
    else
        textColor = colorScheme.foreground( KColorScheme::NormalText ).color();

    QPalette pal( m_labelNecessarySize->palette() );
    pal.setColor( m_labelNecessarySize->foregroundRole(), textColor );
    m_labelNecessarySize->setPalette( pal );

    QTimer::singleShot( 1000, this, SLOT(slotUpdateFreeSpace()) );
}


void K3b::VideoCdRippingDialog::slotUpdateFreeSpace()
{
    QString path = m_editDirectory->url().toLocalFile();

    if( !QFile::exists( path ) )
        path.truncate( path.lastIndexOf('/') );

    unsigned long size, avail;
    if( K3b::kbFreeOnFs( path, size, avail ) )
        slotFreeSpace( path, size, 0, avail );
    else
        m_labelFreeSpace->setText("-");
}

void K3b::VideoCdRippingDialog::loadSettings( const KConfigGroup& c )
{
    m_editDirectory ->setUrl( QUrl::fromLocalFile( c.readEntry( "last ripping directory", K3b::defaultTempPath() ) ) );
    m_ignoreExt ->setChecked( c.readEntry( "ignore ext", false ) );
    m_sector2336 ->setChecked( c.readEntry( "sector 2336", false ) );
    m_extractXML ->setChecked( c.readEntry( "extract xml", false ) );

    slotUpdateFreeSpace();
}

void K3b::VideoCdRippingDialog::saveSettings( KConfigGroup c )
{
    c.writeEntry( "last ripping directory", m_editDirectory->url() );
    c.writeEntry( "ignore ext", m_ignoreExt ->isChecked( ) );
    c.writeEntry( "sector 2336", m_sector2336 ->isChecked( ) );
    c.writeEntry( "extract xml", m_extractXML ->isChecked( ) );
}


