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

#include "k3bmovixdocpreparer.h"
#include "k3bmovixdoc.h"
#include "k3bmovixprogram.h"
#include "k3bmovixfileitem.h"


#include <k3bcore.h>
#include <k3bdiritem.h>
#include <k3bfileitem.h>
#include <k3bbootitem.h>
#include <k3bexternalbinmanager.h>
#include <k3bisoimager.h>

#include <klocale.h>
#include <kdebug.h>
#include <ktempfile.h>
#include <kio/global.h>

#include <qtextstream.h>
#include <qdir.h>


class K3bMovixDocPreparer::Private
{
public:
  Private()
   : doc(0),
     playlistFile(0),
     isolinuxConfigFile(0),
     movixRcFile(0),
     isolinuxDir(0),
     movixDir(0),
     mplayerDir(0),
     playlistFileItem(0),
     structuresCreated(false) {
  }

  K3bMovixDoc* doc;
  const K3bMovixBin* eMovixBin;

  KTempFile* playlistFile;
  KTempFile* isolinuxConfigFile;
  KTempFile* movixRcFile;

  K3bDirItem* isolinuxDir;
  K3bDirItem* movixDir;
  K3bDirItem* mplayerDir;
  K3bFileItem* playlistFileItem;

  bool structuresCreated;
};


K3bMovixDocPreparer::K3bMovixDocPreparer( K3bMovixDoc* doc, QObject* parent, const char* name )
  : K3bJob( parent, name )
{
  d = new Private();
  d->doc = doc;
}


K3bMovixDocPreparer::~K3bMovixDocPreparer()
{
  removeMovixStructures();
  delete d;
}


K3bMovixDoc* K3bMovixDocPreparer::doc() const
{
  return d->doc;
}


void K3bMovixDocPreparer::start()
{
  if( d->structuresCreated )
    removeMovixStructures();
  else
    createMovixStructures();
}


void K3bMovixDocPreparer::cancel()
{
  // do nothing...
}


bool K3bMovixDocPreparer::createMovixStructures()
{
  emit started();

  removeMovixStructures();

  if( doc() )
    doc()->prepareFilenames();

  if( addMovixFiles() ) {
    d->structuresCreated = true;
    emit finished(true);
    return true;
  }
  else {
    emit finished(false);
    return false;
  }
}


void K3bMovixDocPreparer::removeMovixStructures()
{
  emit started();

  // remove movix files from doc
  // the dataitems do the cleanup in the doc
  delete d->movixDir;
  delete d->isolinuxDir;
  delete d->mplayerDir;
  delete d->playlistFileItem;

  d->movixDir = 0;
  d->isolinuxDir = 0;
  d->mplayerDir = 0;
  d->playlistFileItem = 0;

  // remove all the temp files
  delete d->playlistFile;
  delete d->isolinuxConfigFile;
  delete d->movixRcFile;

  d->playlistFile = 0;
  d->isolinuxConfigFile = 0;
  d->movixRcFile = 0;

  d->structuresCreated = false;

  emit finished(true);
}


bool K3bMovixDocPreparer::writePlaylistFile()
{
  delete d->playlistFile;
  d->playlistFile = new KTempFile();
  d->playlistFile->setAutoDelete(true);

  if( QTextStream* s = d->playlistFile->textStream() ) {

    const QPtrList<K3bMovixFileItem>& movixFileItems = d->doc->movixFileItems();

    for( QPtrListIterator<K3bMovixFileItem> it( movixFileItems );
	 *it; ++it ) {
      *s << "/cdrom/";
      *s << it.current()->writtenName();
      *s << endl;
    }

    d->playlistFile->close();
    return true;
  }
  else {
    emit infoMessage( i18n("Could not write to temporary file %1").arg(d->playlistFile->name()), ERROR );
    return false;
  }
}


bool K3bMovixDocPreparer::writeIsolinuxConfigFile()
{
  delete d->isolinuxConfigFile;
  d->isolinuxConfigFile = new KTempFile();
  d->isolinuxConfigFile->setAutoDelete(true);

  if( QTextStream* s = d->isolinuxConfigFile->textStream() ) {

    // now open the default isolinux.cfg and copy everything except the first line which contains
    // the default boot label
    QFile f( d->eMovixBin->path + "/isolinux/isolinux.cfg" );
    if( f.open( IO_ReadOnly ) ) {

      QTextStream isolinuxConfigOrig( &f );

      if(  d->doc->defaultBootLabel() != i18n("default") ) {
	isolinuxConfigOrig.readLine(); // skip first line
	*s << "default " << d->doc->defaultBootLabel()  << endl;
      }

      QString line = isolinuxConfigOrig.readLine();
      while( !line.isNull() ) {
	*s << line << endl;
	line = isolinuxConfigOrig.readLine();
      }

      d->isolinuxConfigFile->close();
      return true;
    }
    else
      return false;
  }
  else {
    emit infoMessage( i18n("Could not write to temporary file %1").arg(d->isolinuxConfigFile->name()), ERROR );
    return false;
  }
}


bool K3bMovixDocPreparer::writeMovixRcFile()
{
  delete d->movixRcFile;
  d->movixRcFile = new KTempFile();
  d->movixRcFile->setAutoDelete(true);

  if( QTextStream* s = d->movixRcFile->textStream() ) {

    if( !d->doc->additionalMPlayerOptions().isEmpty() )
      *s << "extra-mplayer-options=" << d->doc->additionalMPlayerOptions() << endl;
    if( !d->doc->unwantedMPlayerOptions().isEmpty() )
      *s << "unwanted-mplayer-options=" << d->doc->unwantedMPlayerOptions() << endl;
    *s << "loop=" << d->doc->loopPlaylist() << endl;
    if( d->doc->shutdown() )
      *s << "shut=y" << endl;
    if( d->doc->reboot() )
      *s << "reboot=y" << endl;
    if( d->doc->ejectDisk() )
      *s << "eject=y" << endl;
    if( d->doc->randomPlay() )
      *s << "random=y" << endl;
    if( d->doc->noDma() )
      *s << "dma=n" << endl;

    d->movixRcFile->close();    
    return true;
  }
  else {
    emit infoMessage( i18n("Could not write to temporary file %1").arg(d->movixRcFile->name()), ERROR );
    return false;
  }
}


bool K3bMovixDocPreparer::addMovixFiles()
{
  d->eMovixBin = dynamic_cast<const K3bMovixBin*>( k3bcore->externalBinManager()->binObject("eMovix") );
  if( d->eMovixBin ) {

    // first of all we create the directories
    d->isolinuxDir = new K3bDirItem( "isolinux", d->doc, d->doc->root() );
    d->movixDir = new K3bDirItem( "movix", d->doc, d->doc->root() );
    K3bDirItem* kernelDir = d->doc->addEmptyDir( "kernel", d->isolinuxDir );

    // add the linux kernel
    (void)new K3bFileItem( d->eMovixBin->path + "/isolinux/kernel/vmlinuz", d->doc, kernelDir );

    // add the boot image
    K3bBootItem* bootItem = d->doc->createBootItem( d->eMovixBin->path + "/isolinux/isolinux.bin",
						    d->isolinuxDir );
    bootItem->setImageType( K3bBootItem::NONE );
    bootItem->setLoadSize( 4 );
    bootItem->setBootInfoTable(true);

    // some sort weights as defined in isolinux
    d->isolinuxDir->setSortWeigth( 100 );
    kernelDir->setSortWeigth( 50 );
    bootItem->setSortWeigth( 200 );

    // rename the boot cataloge file
    d->doc->bootCataloge()->setK3bName( "isolinux.boot" );

    // the following sucks! Redesign it!

    // add all the isolinux files
    QStringList isolinuxFiles = d->eMovixBin->isolinuxFiles();
    isolinuxFiles.remove( "isolinux.bin" );
    isolinuxFiles.remove( "isolinux.cfg" );
    isolinuxFiles.remove( "kernel/vmlinuz" );
    for( QStringList::const_iterator it = isolinuxFiles.begin();
	 it != isolinuxFiles.end(); ++it ) {
      QString path = d->eMovixBin->path + "/isolinux/" + *it;
      (void)new K3bFileItem( path, d->doc, d->isolinuxDir );
    }

    const QStringList& movixFiles = d->eMovixBin->movixFiles();
    for( QStringList::const_iterator it = movixFiles.begin();
	 it != movixFiles.end(); ++it ) {
      QString path = d->eMovixBin->path + "/movix/" + *it;
      (void)new K3bFileItem( path, d->doc, d->movixDir );
    }

    // add doku files
    QString path = d->eMovixBin->languageDir( d->doc->bootMessageLanguage() );
    QDir dir(path);
    QStringList helpFiles = dir.entryList(QDir::Files);
    for( QStringList::const_iterator it = helpFiles.begin();
	 it != helpFiles.end(); ++it ) {
      // some emovix installations include backup-files, no one's perfect ;)
      if( !(*it).endsWith( "~" ) )
	(void)new K3bFileItem( path + "/" + *it, d->doc, d->isolinuxDir );
    }


    // add subtitle font dir
    if( !d->doc->subtitleFontset().isEmpty() &&
	d->doc->subtitleFontset() != i18n("none") ) {
      d->mplayerDir = new K3bDirItem( "mplayer", d->doc, d->doc->root() );

      QString fontPath = d->eMovixBin->subtitleFontDir( d->doc->subtitleFontset() );
      QFileInfo fontType( fontPath );
      if( fontType.isDir() ) {
	K3bDirItem* fontDir = new K3bDirItem( "font", d->doc, d->mplayerDir );
	QDir dir( fontPath );
	QStringList fontFiles = dir.entryList( QDir::Files );
	for( QStringList::const_iterator it = fontFiles.begin();
	     it != fontFiles.end(); ++it ) {
	  (void)new K3bFileItem( fontPath + "/" + *it, d->doc, fontDir );
	}
      }
      else {
	// just a ttf file
	// needs to be named: subfont.ttf and needs to be placed in mplayer/
	// instead of mplayer/font
	(void)new K3bFileItem( fontPath, 
			       d->doc, 
			       d->mplayerDir,
			       "subfont.ttf" );
      }
    }


    // add movix-config-file and boot-config file
    if( writeMovixRcFile() && 
	writeIsolinuxConfigFile() &&
	writePlaylistFile() ) {

      (void)new K3bFileItem( d->movixRcFile->name(), d->doc, d->movixDir, "movixrc" );
      (void)new K3bFileItem( d->isolinuxConfigFile->name(), d->doc, d->isolinuxDir, "isolinux.cfg" );
      d->playlistFileItem = new K3bFileItem( d->playlistFile->name(), d->doc, d->doc->root(), "movix.list" );

      return true;
    }
    else
      return false;
  }
  else {
    emit infoMessage( i18n("Could not find a valid eMovix installation."), ERROR );
    return false;
  }
}


#include "k3bmovixdocpreparer.moc"
