/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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
#include <k3bglobals.h>
#include <k3bcore.h>

#include <qlabel.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qtimer.h>

#include <qtooltip.h>

#include <qfileinfo.h>

#include <kconfig.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kdialog.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kurlrequester.h>
#include <kio/global.h>
#include <klineedit.h>


K3b::TempDirSelectionWidget::TempDirSelectionWidget( QWidget *parent )
    : QGroupBox( parent ),
      m_labelCdSize(0),
      m_defaultImageFileName( "k3b_image.iso" )
{
    QGridLayout* layout = new QGridLayout( this );
    layout->setSpacing( KDialog::spacingHint() );
    layout->setMargin( KDialog::marginHint() );

    m_imageFileLabel = new QLabel( this );
    m_editDirectory = new KUrlRequester( this );

    m_imageFileLabel->setBuddy( m_editDirectory );

    QLabel* freeSpaceLabel = new QLabel( i18n( "Free space in temporary directory:" ), this );
    m_labelFreeSpace = new QLabel( this );
    m_labelFreeSpace->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

    layout->addWidget( m_imageFileLabel, 0, 0, 1, 2 );
    layout->addWidget( m_editDirectory, 1, 0, 1, 2 );
    layout->addWidget( freeSpaceLabel, 2, 0 );
    layout->addWidget( m_labelFreeSpace, 2, 1 );

    // do not use row 3 here since that could be used in setNeededSize below
    layout->setRowStretch( 4, 1 );

    connect( m_editDirectory, SIGNAL(openFileDialog(KUrlRequester*)),
             this, SLOT(slotTempDirButtonPressed(KUrlRequester*)) );
    connect( m_editDirectory, SIGNAL(textChanged(const QString&)),
             this, SLOT(slotUpdateFreeTempSpace()) );
    connect( m_editDirectory->lineEdit(), SIGNAL(lostFocus()),
             this, SLOT(slotFixTempPath()) );

    // choose a default
    setSelectionMode( DIR );

    m_editDirectory->setUrl( K3b::defaultTempPath() );
    slotUpdateFreeTempSpace();

    // ToolTips
    // --------------------------------------------------------------------------------
    m_editDirectory->setToolTip( i18n("The directory in which to save the image files") );

    // What's This info
    // --------------------------------------------------------------------------------
    m_editDirectory->setWhatsThis( i18n("<p>This is the directory in which K3b will save the <em>image files</em>."
                                        "<p>Please make sure that it resides on a partition that has enough free space.") );
}


K3b::TempDirSelectionWidget::~TempDirSelectionWidget()
{
}


unsigned long K3b::TempDirSelectionWidget::freeTempSpace() const
{
    QString path = m_editDirectory->url().path();

    if( !QFile::exists( path ) )
        path.truncate( path.lastIndexOf('/') );

    unsigned long size;
    K3b::kbFreeOnFs( path, size, m_freeTempSpace );

    return m_freeTempSpace;
}


void K3b::TempDirSelectionWidget::slotUpdateFreeTempSpace()
{
    // update the temp space
    freeTempSpace();

    m_labelFreeSpace->setText( KIO::convertSizeFromKiB(m_freeTempSpace) );

    if( m_labelCdSize ) {
        QPalette pal( m_labelCdSize->palette() );
        if( m_freeTempSpace < m_requestedSize/1024 )
            pal.setColor( QPalette::Text, Qt::red );
        else
            pal.setColor( QPalette::Text, palette().color( QPalette::Text ) );
        m_labelCdSize->setPalette( pal );
    }

    QTimer::singleShot( 1000, this, SLOT(slotUpdateFreeTempSpace()) );
}


void K3b::TempDirSelectionWidget::slotTempDirButtonPressed( KUrlRequester* r )
{
    // set the correct mode for the filedialog
    if( m_mode == DIR ) {
        r->setWindowTitle( i18n("Select Temporary Directory") );
        r->setMode( KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly );
    }
    else {
        r->setWindowTitle( i18n("Select Temporary File") );
        r->setMode( KFile::File | KFile::LocalOnly );
    }
}


void K3b::TempDirSelectionWidget::setTempPath( const QString& dir )
{
    m_editDirectory->setUrl( dir );
    slotUpdateFreeTempSpace();
}


QString K3b::TempDirSelectionWidget::tempPath() const
{
    QFileInfo fi( m_editDirectory->url().path() );

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
    return m_editDirectory->url().path();
}


QString K3b::TempDirSelectionWidget::tempDirectory() const
{
    QString td( m_editDirectory->url().path() );

    // remove a trailing slash
    while( !td.isEmpty() && td[td.length()-1] == '/' )
        td.truncate( td.length()-1 );

    QFileInfo fi( td );
    if( fi.exists() && fi.isDir() )
        return td + "/";

    // now we treat the last section as a filename and return the path
    // in front of it
    td.truncate( td.lastIndexOf( '/' ) + 1 );
    return td;
}


void K3b::TempDirSelectionWidget::setSelectionMode( int mode )
{
    m_mode = mode;

    if( m_mode == DIR ) {
        m_imageFileLabel->setText( i18n( "Wri&te image files to:" ) );
        setTitle( i18n("Temporary Directory") );
    }
    else {
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
        m_labelCdSize = new QLabel( KIO::convertSize(bytes), this );
        m_labelCdSize->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
        grid->addWidget( m_labelCdSize, 3, 1 );
    }
    m_labelCdSize->setText( KIO::convertSize(bytes) );
}


void K3b::TempDirSelectionWidget::saveConfig()
{
    KConfigGroup grp( KGlobal::config(), "General Options" );
    grp.writePathEntry( "Temp Dir", tempDirectory() );
}


void K3b::TempDirSelectionWidget::readConfig( const KConfigGroup& c )
{
    setTempPath( c.readPathEntry( "image path", K3b::defaultTempPath() ) );
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

#include "k3btempdirselectionwidget.moc"
