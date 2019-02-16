/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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


#include "k3btempdirselectionwidget.h"
#include "k3bcore.h"
#include "k3bglobalsettings.h"

#include <KLineEdit>
#include <KColorScheme>
#include <KConfig>
#include <KLocalizedString>
#include <KIconLoader>
#include <KDiskFreeSpaceInfo>
#include <KIO/Global>
#include <KUrlRequester>
#include <kio_version.h>

#include <QFileInfo>
#include <QTimer>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QToolTip>


K3b::TempDirSelectionWidget::TempDirSelectionWidget( QWidget *parent )
    : QGroupBox( parent ),
      m_labelCdSize(0),
      m_requestedSize(0),
      m_defaultImageFileName( "k3b_image.iso" )
{
    QGridLayout* layout = new QGridLayout( this );

    m_imageFileLabel = new QLabel( this );
    m_editDirectory = new KUrlRequester( this );

    m_imageFileLabel->setBuddy( m_editDirectory );

    QLabel* freeSpaceLabel = new QLabel( i18n( "Free space in temporary folder:" ), this );
    m_labelFreeSpace = new QLabel( this );
    m_labelFreeSpace->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

    layout->addWidget( m_imageFileLabel, 0, 0, 1, 2 );
    layout->addWidget( m_editDirectory, 1, 0, 1, 2 );
    layout->addWidget( freeSpaceLabel, 2, 0 );
    layout->addWidget( m_labelFreeSpace, 2, 1 );

    // do not use row 3 here since that could be used in setNeededSize below
    layout->setRowStretch( 4, 1 );

    connect( m_editDirectory, SIGNAL(textChanged(QString)),
             this, SLOT(slotUpdateFreeTempSpace()) );
    connect( m_editDirectory->lineEdit(), SIGNAL(lostFocus()),
             this, SLOT(slotFixTempPath()) );

    // choose a default
    setSelectionMode( DIR );

#if KIO_VERSION >= QT_VERSION_CHECK(5, 33, 0)
    m_editDirectory->setAcceptMode(QFileDialog::AcceptSave);
#else
    m_editDirectory->fileDialog()->setAcceptMode(QFileDialog::AcceptSave);
#endif
    m_editDirectory->setUrl( QUrl::fromLocalFile( k3bcore->globalSettings()->defaultTempPath() ) );
    slotUpdateFreeTempSpace();

    // ToolTips
    // --------------------------------------------------------------------------------
    m_editDirectory->setToolTip( i18n("The folder in which to save the image files") );

    // What's This info
    // --------------------------------------------------------------------------------
    m_editDirectory->setWhatsThis( i18n("<p>This is the folder in which K3b will save the <em>image files</em>."
                                        "<p>Please make sure that it resides on a partition that has enough free space.") );
}


K3b::TempDirSelectionWidget::~TempDirSelectionWidget()
{
}


KIO::filesize_t K3b::TempDirSelectionWidget::freeTempSpace() const
{
    QString path = m_editDirectory->url().toLocalFile();

    if( !QFile::exists( path ) )
        path.truncate( path.lastIndexOf('/') );

    KDiskFreeSpaceInfo diskFreeSpaceInfo = KDiskFreeSpaceInfo::freeSpaceInfo( path );
    return diskFreeSpaceInfo.available();
}


void K3b::TempDirSelectionWidget::slotUpdateFreeTempSpace()
{
    // update the temp space
    KIO::filesize_t tempFreeSpace = freeTempSpace();

    KColorScheme::ForegroundRole role;
    if( tempFreeSpace < m_requestedSize )
        role = KColorScheme::NegativeText;
    else
        role = KColorScheme::NormalText;
    
    QPalette pal( m_labelFreeSpace->palette() );
    pal.setBrush( QPalette::Disabled, QPalette::WindowText, KColorScheme( QPalette::Disabled, KColorScheme::Window ).foreground( role ) );
    pal.setBrush( QPalette::Active,   QPalette::WindowText, KColorScheme( QPalette::Active,   KColorScheme::Window ).foreground( role ) );
    pal.setBrush( QPalette::Inactive, QPalette::WindowText, KColorScheme( QPalette::Inactive, KColorScheme::Window ).foreground( role ) );
    pal.setBrush( QPalette::Normal,   QPalette::WindowText, KColorScheme( QPalette::Normal,   KColorScheme::Window ).foreground( role ) );
    
    m_labelFreeSpace->setPalette( pal );
    m_labelFreeSpace->setText( KIO::convertSize(tempFreeSpace) );

    QTimer::singleShot( 1000, this, SLOT(slotUpdateFreeTempSpace()) );
}


void K3b::TempDirSelectionWidget::setTempPath( const QString& dir )
{
    m_editDirectory->setUrl( QUrl::fromLocalFile( dir ) );
    slotUpdateFreeTempSpace();
}


QString K3b::TempDirSelectionWidget::tempPath() const
{
    QFileInfo fi( m_editDirectory->url().toLocalFile() );

    if( fi.exists() ) {
        if( m_mode == DIR ) {
            if( fi.isDir() )
                return fi.absoluteFilePath();
            else
                return fi.absolutePath();
        }
        else {
            if( fi.isFile() )
                return fi.absoluteFilePath();
            else
                return fi.absoluteFilePath() + "/k3b_image.iso";
        }
    }
    else {
        return fi.absoluteFilePath();
    }
}


QString K3b::TempDirSelectionWidget::plainTempPath() const
{
    return m_editDirectory->url().toLocalFile();
}


QString K3b::TempDirSelectionWidget::tempDirectory() const
{
    QString td( m_editDirectory->url().toLocalFile() );

    // remove a trailing slash
    while( !td.isEmpty() && td[td.length()-1] == '/' )
        td.truncate( td.length()-1 );

    QFileInfo fi( td );
    if( fi.exists() && fi.isDir() )
        return td + '/';

    // now we treat the last section as a filename and return the path
    // in front of it
    td.truncate( td.lastIndexOf( '/' ) + 1 );
    return td;
}


void K3b::TempDirSelectionWidget::setSelectionMode( int mode )
{
    m_mode = mode;

    if( m_mode == DIR ) {
        m_editDirectory->setWindowTitle( i18n("Select Temporary Folder") );
        m_editDirectory->setMode( KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly );
        m_imageFileLabel->setText( i18n( "Wri&te image files to:" ) );
        setTitle( i18n("Temporary Folder") );
    }
    else {
        m_editDirectory->setWindowTitle( i18n("Select Temporary File") );
        m_editDirectory->setMode( KFile::File | KFile::LocalOnly );
        m_imageFileLabel->setText( i18n( "Wri&te image file to:" ) );
        setTitle( i18n("Temporary File") );
    }
}


void K3b::TempDirSelectionWidget::setNeededSize( KIO::filesize_t bytes )
{
    m_requestedSize = bytes;
    if( !m_labelCdSize ) {
        QGridLayout* grid = static_cast<QGridLayout*>( layout() );
        grid->addWidget( new QLabel( i18n( "Size of project:" ), this ), 3, 0 );
        m_labelCdSize = new QLabel( KIO::convertSize(m_requestedSize), this );
        m_labelCdSize->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
        grid->addWidget( m_labelCdSize, 3, 1 );
    }
    m_labelCdSize->setText( KIO::convertSize(m_requestedSize) );
}


void K3b::TempDirSelectionWidget::saveConfig()
{
    k3bcore->globalSettings()->setDefaultTempPath( tempDirectory() );
}


void K3b::TempDirSelectionWidget::readConfig( const KConfigGroup& c )
{
    setTempPath( c.readPathEntry( "image path", k3bcore->globalSettings()->defaultTempPath() ) );
}


void K3b::TempDirSelectionWidget::saveConfig( KConfigGroup c )
{
    c.writePathEntry( "image path", tempPath() );
}


void K3b::TempDirSelectionWidget::setDefaultImageFileName( const QString& name, bool changeImageName )
{
    if ( !name.isEmpty() ) {
        if ( selectionMode() == FILE ) {
            if ( plainTempPath().section( '/', -1 ) == m_defaultImageFileName ) {
                changeImageName = true;
            }
        }

        m_defaultImageFileName = name;
        if ( !m_defaultImageFileName.contains( '.' ) ) {
            m_defaultImageFileName += ".iso";
        }
        fixTempPath( changeImageName );
    }
}


void K3b::TempDirSelectionWidget::slotFixTempPath()
{
    fixTempPath( false );
}


void K3b::TempDirSelectionWidget::fixTempPath( bool forceNewImageName )
{
    // if in file selection mode and no image file is specified or
    // forceNewImageName is true set the default image file name
    if ( selectionMode() == FILE ) {
        if ( forceNewImageName ||
             QFileInfo( plainTempPath() ).isDir() ) {
            setTempPath( tempDirectory() + m_defaultImageFileName );
        }
    }
}


void K3b::TempDirSelectionWidget::setImageFileLabel(const QString &label)
{
    m_imageFileLabel->setText(label);
}
