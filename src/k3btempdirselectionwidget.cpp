/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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
#include <q3groupbox.h>
#include <qlayout.h>
#include <qtimer.h>
#include <q3hbox.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include <qfileinfo.h>

#include <kconfig.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kdialog.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kurlrequester.h>
#include <kio/global.h>
#include <kconfig.h>
#include <klineedit.h>


K3bTempDirSelectionWidget::K3bTempDirSelectionWidget( QWidget *parent, const char *name )
  : Q3GroupBox( 4, Qt::Vertical, parent, name ),
    m_labelCdSize(0),
    m_defaultImageFileName( "k3b_image.iso" )
{
  layout()->setSpacing( KDialog::spacingHint() );
  layout()->setMargin( KDialog::marginHint() );

  m_imageFileLabel = new QLabel( this );
  m_editDirectory = new KUrlRequester( this, "m_editDirectory" );

  m_imageFileLabel->setBuddy( m_editDirectory );

  Q3HBox* freeTempSpaceBox = new Q3HBox( this );
  freeTempSpaceBox->setSpacing( KDialog::spacingHint() );
  (void)new QLabel( i18n( "Free space in temporary directory:" ), freeTempSpaceBox, "TextLabel2" );
  m_labelFreeSpace = new QLabel( "                       ",freeTempSpaceBox, "m_labelFreeSpace" );
  m_labelFreeSpace->setAlignment( int( QLabel::Qt::AlignVCenter | QLabel::Qt::AlignRight ) );


  connect( m_editDirectory, SIGNAL(openFileDialog(KUrlRequester*)),
	   this, SLOT(slotTempDirButtonPressed(KUrlRequester*)) );
  connect( m_editDirectory, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotUpdateFreeTempSpace()) );
  connect( m_editDirectory->lineEdit(), SIGNAL(lostFocus()),
           this, SLOT(slotFixTempPath()) );

  // choose a default
  setSelectionMode( DIR );

  m_editDirectory->setURL( K3b::defaultTempPath() );
  slotUpdateFreeTempSpace();

  // ToolTips
  // --------------------------------------------------------------------------------
  QToolTip::add( m_editDirectory, i18n("The directory in which to save the image files") );

  // What's This info
  // --------------------------------------------------------------------------------
  Q3WhatsThis::add( m_editDirectory, i18n("<p>This is the directory in which K3b will save the <em>image files</em>."
					 "<p>Please make sure that it resides on a partition that has enough free space.") );
}


K3bTempDirSelectionWidget::~K3bTempDirSelectionWidget()
{
}


unsigned long K3bTempDirSelectionWidget::freeTempSpace() const
{
  QString path = m_editDirectory->url();

  if( !QFile::exists( path ) )
    path.truncate( path.findRev('/') );

  unsigned long size;
  K3b::kbFreeOnFs( path, size, m_freeTempSpace );

  return m_freeTempSpace;
}


void K3bTempDirSelectionWidget::slotUpdateFreeTempSpace()
{
  // update the temp space
  freeTempSpace();

  m_labelFreeSpace->setText( KIO::convertSizeFromKiB(m_freeTempSpace) );

  if( m_labelCdSize ) {
    if( m_freeTempSpace < m_requestedSize/1024 )
      m_labelCdSize->setPaletteForegroundColor( red );
    else
      m_labelCdSize->setPaletteForegroundColor( m_labelFreeSpace->paletteForegroundColor() );
  }
  QTimer::singleShot( 1000, this, SLOT(slotUpdateFreeTempSpace()) );
}


void K3bTempDirSelectionWidget::slotTempDirButtonPressed( KUrlRequester* r )
{
  // set the correct mode for the filedialog
  if( m_mode == DIR ) {
    r->setCaption( i18n("Select Temporary Directory") );
    r->setMode( KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly );
  }
  else {
    r->setCaption( i18n("Select Temporary File") );
    r->setMode( KFile::File | KFile::LocalOnly );
  }
}


void K3bTempDirSelectionWidget::setTempPath( const QString& dir )
{
  m_editDirectory->setURL( dir );
  slotUpdateFreeTempSpace();
}


QString K3bTempDirSelectionWidget::tempPath() const
{
  QFileInfo fi( m_editDirectory->url() );

  if( fi.exists() ) {
    if( m_mode == DIR ) {
      if( fi.isDir() )
	return fi.absoluteFilePath();
      else
	return fi.dirPath( true );
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


QString K3bTempDirSelectionWidget::plainTempPath() const
{
  return m_editDirectory->url();
}


QString K3bTempDirSelectionWidget::tempDirectory() const
{
  QString td( m_editDirectory->url() );

  // remove a trailing slash
  while( !td.isEmpty() && td[td.length()-1] == '/' )
    td.truncate( td.length()-1 );

  QFileInfo fi( td );
  if( fi.exists() && fi.isDir() )
    return td + "/";

  // now we treat the last section as a filename and return the path
  // in front of it
  td.truncate( td.findRev( '/' ) + 1 );
  return td;
}


void K3bTempDirSelectionWidget::setSelectionMode( int mode )
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


void K3bTempDirSelectionWidget::setNeededSize( KIO::filesize_t bytes )
{
  m_requestedSize = bytes;
  if( !m_labelCdSize ) {
    Q3HBox* cdSizeBox = new Q3HBox( this );
    cdSizeBox->setSpacing( KDialog::spacingHint() );
    (void)new QLabel( i18n( "Size of project:" ), cdSizeBox, "TextLabel4" );
    m_labelCdSize = new QLabel( KIO::convertSize(bytes), cdSizeBox, "m_labelCdSize" );
    m_labelCdSize->setAlignment( int( QLabel::Qt::AlignVCenter | QLabel::Qt::AlignRight ) );
  }
  m_labelCdSize->setText( KIO::convertSize(bytes) );
}


void K3bTempDirSelectionWidget::saveConfig()
{
  KConfigGroup grp( k3bcore->config(), "General Options" );
  grp.writePathEntry( "Temp Dir", tempDirectory() );
}


void K3bTempDirSelectionWidget::readConfig( KConfigBase* c )
{
  setTempPath( c->readPathEntry( "image path", K3b::defaultTempPath() ) );
}


void K3bTempDirSelectionWidget::saveConfig( KConfigBase* c )
{
  c->writePathEntry( "image path", tempPath() );
}


void K3bTempDirSelectionWidget::setDefaultImageFileName( const QString& name, bool changeImageName )
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


void K3bTempDirSelectionWidget::slotFixTempPath()
{
    fixTempPath( false );
}


void K3bTempDirSelectionWidget::fixTempPath( bool forceNewImageName )
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
