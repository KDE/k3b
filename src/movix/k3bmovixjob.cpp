/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bmovixjob.h"
#include "k3bmovixdoc.h"
#include "k3bmovixinstallation.h"
#include "k3bmovixfileitem.h"

#include <data/k3bdatajob.h>
#include <data/k3bbootitem.h>
#include <data/k3bdiritem.h>
#include <data/k3bfileitem.h>
#include <data/k3bisoimager.h>
#include <tools/k3bexternalbinmanager.h>

#include <klocale.h>
#include <kdebug.h>
#include <ktempfile.h>
#include <kio/global.h>

#include <qtextstream.h>
#include <qdir.h>


K3bMovixJob::K3bMovixJob( K3bMovixDoc* doc, QObject* parent )
  : K3bBurnJob( parent ),
    m_doc(doc),
    m_installation(0),
    m_playlistFile(0),
    m_isolinuxConfigFile(0),
    m_movixRcFile(0),
    m_isolinuxDir(0),
    m_movixDir(0),
    m_mplayerDir(0)
{
  m_dataJob = new K3bDataJob( doc, this );

  // pipe signals
  connect( m_dataJob, SIGNAL(percent(int)), this, SIGNAL(percent(int)) );
  connect( m_dataJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_dataJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  connect( m_dataJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_dataJob, SIGNAL(bufferStatus(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_dataJob, SIGNAL(writeSpeed(int)), this, SIGNAL(writeSpeed(int)) );
  connect( m_dataJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_dataJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_dataJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  connect( m_dataJob, SIGNAL(infoMessage(const QString&, int)),
	   this, SIGNAL(infoMessage(const QString&, int)) );

  // we need to clean up here
  connect( m_dataJob, SIGNAL(finished(bool)), this, SLOT(slotDataJobFinished(bool)) );
}


K3bMovixJob::~K3bMovixJob()
{
  cleanUp();
}


K3bDevice* K3bMovixJob::writer() const
{
  return m_dataJob->writer();
}


K3bDoc* K3bMovixJob::doc() const
{
  return m_doc; 
}


void K3bMovixJob::start()
{
  emit started();

  m_canceled = false;
  m_dataJob->setWritingApp( writingApp() );

  if( addMovixFiles() ) {
    m_dataJob->start();
  }
  else {
    cleanUp();
    emit finished(false);
  }
}


void K3bMovixJob::cancel()
{
  m_canceled = true;
  m_dataJob->cancel();
}


void K3bMovixJob::slotDataJobFinished( bool success )
{
  cleanUp();

  if( m_canceled )
    emit canceled();

  emit finished( success );
}


bool K3bMovixJob::writePlaylistFile()
{
  delete m_playlistFile;
  m_playlistFile = new KTempFile();
  m_playlistFile->setAutoDelete(true);

  if( QTextStream* s = m_playlistFile->textStream() ) {

    const QPtrList<K3bMovixFileItem>& movixFileItems = m_doc->movixFileItems();

    if( m_doc->isoOptions().createJoliet() )
      K3bIsoImager::createJolietFilenames( m_doc->root() );      

    for( QPtrListIterator<K3bMovixFileItem> it( movixFileItems );
	 *it; ++it ) {
      *s << "/cdrom/";
      if( m_doc->isoOptions().createJoliet() )
	*s << it.current()->jolietName();
      else
	*s << it.current()->k3bName();
      *s << endl;
    }

    m_playlistFile->close();
    return true;
  }
  else {
    emit infoMessage( i18n("Could not write to temporary file %1").arg(m_playlistFile->name()), ERROR );
    return false;
  }
}


bool K3bMovixJob::writeIsolinuxConfigFile()
{
  delete m_isolinuxConfigFile;
  m_isolinuxConfigFile = new KTempFile();
  m_isolinuxConfigFile->setAutoDelete(true);

  if( QTextStream* s = m_isolinuxConfigFile->textStream() ) {

    // now open the default isolinux.cfg and copy everything except the first line which contains
    // the default boot label
    QFile f( m_installation->path() + "/isolinux/isolinux.cfg" );
    if( f.open( IO_ReadOnly ) ) {

      QTextStream isolinuxConfigOrig( &f );

      if(  m_doc->defaultBootLabel() != i18n("default") ) {
	isolinuxConfigOrig.readLine(); // skip first line
	*s << "default " << m_doc->defaultBootLabel()  << endl;
      }

      QString line = isolinuxConfigOrig.readLine();
      while( !line.isNull() ) {
	*s << line << endl;
	line = isolinuxConfigOrig.readLine();
      }

      m_isolinuxConfigFile->close();
      return true;
    }
    else
      return false;
  }
  else {
    emit infoMessage( i18n("Could not write to temporary file %1").arg(m_isolinuxConfigFile->name()), ERROR );
    return false;
  }
}


bool K3bMovixJob::writeMovixRcFile()
{
  delete m_movixRcFile;
  m_movixRcFile = new KTempFile();
  m_movixRcFile->setAutoDelete(true);

  if( QTextStream* s = m_movixRcFile->textStream() ) {

    if( !m_doc->additionalMPlayerOptions().isEmpty() )
      *s << "extra-mplayer-options=" << m_doc->additionalMPlayerOptions() << endl;
    if( !m_doc->unwantedMPlayerOptions().isEmpty() )
      *s << "unwanted-mplayer-options=" << m_doc->unwantedMPlayerOptions() << endl;
    *s << "loop=" << m_doc->loopPlaylist() << endl;
    if( m_doc->shutdown() )
      *s << "shut=y" << endl;
    if( m_doc->reboot() )
      *s << "reboot=y" << endl;
    if( m_doc->ejectDisk() )
      *s << "eject=y" << endl;
    if( m_doc->randomPlay() )
      *s << "random=y" << endl;

    m_movixRcFile->close();    
    return true;
  }
  else {
    emit infoMessage( i18n("Could not write to temporary file %1").arg(m_movixRcFile->name()), ERROR );
    return false;
  }
}


bool K3bMovixJob::addMovixFiles()
{
  if( m_installation )
    delete m_installation;

  QString path = K3bExternalBinManager::self()->binPath("eMovix");
  m_installation = K3bMovixInstallation::probeInstallation( path );
  if( m_installation ) {

    // first of all we create the directories
    m_isolinuxDir = new K3bDirItem( "isolinux", m_doc, m_doc->root() );
    m_movixDir = new K3bDirItem( "movix", m_doc, m_doc->root() );

    // add the boot image
    K3bBootItem* bootItem = m_doc->createBootItem( m_installation->path() + "/isolinux/isolinux.bin",
						   m_isolinuxDir );
    bootItem->setImageType( K3bBootItem::NONE );
    bootItem->setLoadSize( 4 );
    bootItem->setBootInfoTable(true);

    // rename the boot cataloge file
    m_doc->bootCataloge()->setK3bName( "isolinux.boot" );

    // the following sucks! Redesign it!

    // add all the isolinux files
    QStringList isolinuxFiles = m_installation->isolinuxFiles();
    isolinuxFiles.remove( "isolinux.bin" );
    isolinuxFiles.remove( "isolinux.cfg" );
    isolinuxFiles.remove( "kernel/vmlinuz" );
    for( QStringList::const_iterator it = isolinuxFiles.begin();
	 it != isolinuxFiles.end(); ++it ) {
      QString path = m_installation->path() + "/isolinux/" + *it;
      (void)new K3bFileItem( path, m_doc, m_isolinuxDir );
    }

    K3bDirItem* kernelDir = m_doc->addEmptyDir( "kernel", m_isolinuxDir );
    (void)new K3bFileItem( m_installation->path() + "/isolinux/kernel/vmlinuz", m_doc, kernelDir );

    QStringList movixFiles = m_installation->movixFiles();
    for( QStringList::const_iterator it = movixFiles.begin();
	 it != movixFiles.end(); ++it ) {
      QString path = m_installation->path() + "/movix/" + *it;
      (void)new K3bFileItem( path, m_doc, m_movixDir );
    }

    // add doku files
    QString path = m_installation->languageDir( m_doc->bootMessageLanguage() );
    QDir dir(path);
    QStringList helpFiles = dir.entryList(QDir::Files);
    for( QStringList::const_iterator it = helpFiles.begin();
	 it != helpFiles.end(); ++it )
      (void)new K3bFileItem( path + "/" + *it, m_doc, m_isolinuxDir );


    // add subtitle font dir
    if( !m_doc->subtitleFontset().isEmpty() &&
	m_doc->subtitleFontset() != i18n("none") ) {
      m_mplayerDir = new K3bDirItem( "mplayer", m_doc, m_doc->root() );
      K3bDirItem* fontDir = new K3bDirItem( "font", m_doc, m_mplayerDir );

      QString fontPath = m_installation->subtitleFontDir( m_doc->subtitleFontset() );
      QDir d( fontPath );
      QStringList fontFiles = d.entryList( QDir::Files );
      for( QStringList::const_iterator it = fontFiles.begin();
	   it != fontFiles.end(); ++it ) {
	(void)new K3bFileItem( fontPath + "/" + *it, m_doc, fontDir );
      }
    }


    // add movix-config-file and boot-config file
    if( writeMovixRcFile() && 
	writeIsolinuxConfigFile() &&
	writePlaylistFile() ) {

      (void)new K3bFileItem( m_movixRcFile->name(), m_doc, m_movixDir, "movixrc" );
      (void)new K3bFileItem( m_isolinuxConfigFile->name(), m_doc, m_isolinuxDir, "isolinux.cfg" );
      m_playlistFileItem = new K3bFileItem( m_playlistFile->name(), m_doc, m_doc->root(), "movix.list" );

      return true;
    }
    else
      return false;
  }
  else {
    emit infoMessage( i18n("Could not find eMovix installation in %1").arg(path), ERROR );
    return false;
  }
}


void K3bMovixJob::cleanUp()
{
  // remove movix files from doc
  // the dataitems do the cleanup in the doc
  delete m_movixDir;
  delete m_isolinuxDir;
  delete m_mplayerDir;
  delete m_playlistFileItem;

  m_movixDir = 0;
  m_isolinuxDir = 0;
  m_mplayerDir = 0;
  m_playlistFileItem = 0;

  delete m_installation;

  // remove all the temp files
  delete m_playlistFile;
  delete m_isolinuxConfigFile;
  delete m_movixRcFile;

  m_installation = 0;
  m_playlistFile = 0;
  m_isolinuxConfigFile = 0;
  m_movixRcFile = 0;
}

QString K3bMovixJob::jobDescription() const
{
  if( m_doc->isoOptions().volumeID().isEmpty() )
    return i18n("Writing eMovix cd");
  else
    return i18n("Writing eMovix cd (%1)").arg(m_doc->isoOptions().volumeID());
}


QString K3bMovixJob::jobDetails() const
{
  return i18n("1 file (%1)", "%n files (%1)", m_doc->movixFileItems().count()).arg(KIO::convertSize(m_doc->size()));
}

#include "k3bmovixjob.moc"
