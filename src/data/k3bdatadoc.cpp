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


#include "k3bdatadoc.h"
#include "k3bfileitem.h"
#include "k3bdiritem.h"
#include "k3bdataview.h"
#include "k3bdatajob.h"
#include "k3bbootitem.h"
#include "k3bspecialdataitem.h"
#include <k3b.h>
#include <k3bcore.h>
#include <tools/k3bglobals.h>

#include <stdlib.h>

#include <qdir.h>
#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qtimer.h>
#include <qdom.h>
#include <qptrlist.h>

#include <kstandarddirs.h>
#include <kurl.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <klineeditdlg.h>
#include <kmimemagic.h>
#include <kmessagebox.h>
#include <kfilemetainfo.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kprogress.h>
#include <kconfig.h>


K3bDataDoc::K3bDataDoc( QObject* parent )
  : K3bDoc( parent )
{
  m_root = 0;

  m_queuedToAddItemsTimer = new QTimer( this );
  connect( m_queuedToAddItemsTimer, SIGNAL(timeout()), this, SLOT(slotAddQueuedItems()) );
}

K3bDataDoc::~K3bDataDoc()
{
  delete m_root;	
}


bool K3bDataDoc::newDocument()
{
  m_bootImages.clear();
  m_bootCataloge = 0;

  if( m_root )
    delete m_root;
		
  m_root = new K3bRootItem( this );
  //  m_size = 0;

	
  m_name = "Dummyname";

  m_multisessionMode = NONE;
  m_dataMode = K3b::AUTO;

  m_isoOptions = K3bIsoOptions();

  return K3bDoc::newDocument();
}


K3bView* K3bDataDoc::newView( QWidget* parent )
{
  return new K3bDataView( this, parent );
}


void K3bDataDoc::addView(K3bView* view)
{
  K3bDoc::addView( view );
}


void K3bDataDoc::addUrls( const KURL::List& urls )
{
  slotAddUrlsToDir( urls );
}


void K3bDataDoc::slotAddUrlsToDir( const KURL::List& urls, K3bDirItem* dirItem )
{
  if( !dirItem ) {
    kdDebug() << "(K3bDataDoc) DirItem = 0 !!!!!" << endl;

    // get current dir from first view (it should better be the current active view)
    K3bDataView* view = (K3bDataView*)firstView();
    dirItem = view->currentDir();
  }

  for( KURL::List::ConstIterator it = urls.begin(); it != urls.end(); ++it ) 
    {
      const KURL& url = *it;
      if( url.isLocalFile() && QFile::exists(url.path()) ) {

	// mkisofs seems to have a bug that prevents us to use filenames 
	// that contain one or more backslashes
	// -----------------------------------------------------------------------
	if( url.path().contains( "\\\\" ) ) {
	  m_mkisofsBuggyFiles.append( url.path() );
	}
	// -----------------------------------------------------------------------

	else
	  m_queuedToAddItems.append( new PrivateItemToAdd(url.path(), dirItem ) );
      }
      else
	m_notFoundFiles.append( url.path() );
    }

  m_queuedToAddItemsTimer->start(0);
  k3bMain()->showBusyInfo( i18n( "Adding files to Project %1..." ).arg( isoOptions().volumeID() ) );
}


void K3bDataDoc::slotAddQueuedItems()
{
  m_queuedToAddItems.first();
  PrivateItemToAdd* item = m_queuedToAddItems.take();
  if( item ) {
    m_queuedToAddItemsTimer->stop();

    setModified( true );

    if( !item->fileInfo.exists() )
      return;

    if( item->fileInfo.isDir() && !item->fileInfo.isSymLink() ) {
      createDirItem( item->fileInfo, item->parent );
    }
    else {
      createFileItem( item->fileInfo, item->parent );
    }
    
    m_numberAddedItems++;
    if( m_numberAddedItems >= 500 ) {
      emit newFileItems();
      m_numberAddedItems = 0;
    }

    delete item;
    m_queuedToAddItemsTimer->start(0);
  }
  else {
    m_numberAddedItems = 0;
    m_queuedToAddItemsTimer->stop();
    emit newFileItems();
    k3bMain()->endBusy();
    informAboutNotFoundFiles();
  }
}


K3bDirItem* K3bDataDoc::createDirItem( QFileInfo& f, K3bDirItem* parent )
{
  QString newName = f.fileName();

  if( newName.isEmpty() ) {
    kdDebug() << "(K3bDataDoc) tried to create dir without name." << endl;
    return 0;
  }


  // check for recursion
  // if the added file is a symlink we check if it is a subdirectory
  // of the resolved one
  if( f.isSymLink() ) {
    QFileInfo link( f );

    while( link.isSymLink() ) {
      if( link.readLink().startsWith("/") )
	link.setFile( link.readLink() );
      else
	link.setFile( link.dirPath() + "/" + link.readLink() );
    }

    // symLink resolved
    if( f.absFilePath().startsWith( link.absFilePath() ) ) {
      KMessageBox::error( firstView(), i18n("Found recursion in directory tree. Omitting\n%1").arg(f.absFilePath()) );
      return 0;
    }
  }


  if( nameAlreadyInDir( newName, parent ) ) {
    k3bcore->config()->setGroup("Data project settings");
    bool dropDoubles = k3bcore->config()->readBoolEntry( "Drop doubles", false );
    if( dropDoubles )
      return 0;

    bool ok = true;
    while( ok && nameAlreadyInDir( newName, parent ) ) {
      newName = KLineEditDlg::getText( i18n("A directory with that name already exists. Please enter a new name."), 
				       newName, &ok, firstView() );
    }
    if( !ok )
      return 0;
  }

  K3bDirItem* newDirItem = new K3bDirItem( newName, this, parent );

  KConfig* c = kapp->config();
  c->setGroup( "Data project settings" );

  int dirFilter = QDir::All;
  if( c->readBoolEntry( "List hidden files", false ) )
    dirFilter |= QDir::Hidden;
  if( c->readBoolEntry( "List system files", false ) )
    dirFilter |= QDir::System;

  QStringList dlist = QDir( f.absFilePath() ).entryList( dirFilter );
  dlist.remove(".");
  dlist.remove("..");
  
  for( QStringList::Iterator it = dlist.begin(); it != dlist.end(); ++it ) {
    QFileInfo newF(f.absFilePath() + "/" + *it);
    if( newF.isDir() )
      m_queuedToAddItems.append( new PrivateItemToAdd( newF, newDirItem ) );
    else
      createFileItem( newF, newDirItem );
  }

  return newDirItem;
}


K3bFileItem* K3bDataDoc::createFileItem( QFileInfo& f, K3bDirItem* parent )
{
  QString newName = f.fileName();


  // filter symlinks and follow them
//   while( f.isSymLink() ) {
//     if( f.readLink().startsWith("/") )
//       f.setFile( f.readLink() );
//     else
//       f.setFile( f.dirPath() + "/" + f.readLink() );
    
//     // check if it was a corrupted symlink
//     if( !f.exists() ) {
//       kdDebug() << "(K3bDataDoc) corrupted symlink: " << f.absFilePath() << endl;
//       m_notFoundFiles.append( f.absFilePath() );
//       return;
//     }
//   }

  QString mimetype = KMimeMagic::self()->findFileType(f.absFilePath())->mimeType();
  KConfig* c = kapp->config();
  c->setGroup( "Data project settings" );


  // sometimes ogg-vorbis files go as "application/x-ogg"
  if( c->readBoolEntry( "Use ID3 Tag for mp3 renaming", false ) && 
      ( mimetype.contains( "audio" ) || mimetype.contains("ogg") ) ) {

    KFileMetaInfo metaInfo( f.absFilePath() );
    if( !metaInfo.isEmpty() && metaInfo.isValid() ) {
		
      KFileMetaInfoItem artistItem = metaInfo.item( "Artist" );
      KFileMetaInfoItem titleItem = metaInfo.item( "Title" );

      if( artistItem.isValid() && titleItem.isValid() ) {
	newName = artistItem.string() + " - " + titleItem.string() + "." + f.extension(false);
      }
    }
  }


  if( nameAlreadyInDir( newName, parent ) ) {
    k3bcore->config()->setGroup("Data project settings");
    bool dropDoubles = k3bcore->config()->readBoolEntry( "Drop doubles", false );
    if( dropDoubles )
      return 0;

    bool ok = true;
    do {
      newName = KLineEditDlg::getText( i18n("A file with that name already exists. Please enter a new name."), 
				       newName, &ok, firstView() );
    } while( ok && nameAlreadyInDir( newName, parent ) );

    if( !ok )
      return 0;
  }


  K3bFileItem* newK3bItem = new K3bFileItem( f.absFilePath(), this, parent, newName );
  //  m_size += newK3bItem->k3bSize();

  return newK3bItem;
}


bool K3bDataDoc::nameAlreadyInDir( const QString& name, K3bDirItem* dir )
{
  if( !dir ) {
    return false;
  }

  QPtrListIterator<K3bDataItem> it( *dir->children() );
  for( ; it.current(); ++it ) {
    if( it.current()->k3bName() == name ) {
      kdDebug() << "(K3bDataDoc) already a file with that name in directory: " << name << endl;
      return true;
    }
  }
  
  return false;
}


K3bDirItem* K3bDataDoc::addEmptyDir( const QString& name, K3bDirItem* parent )
{
  K3bDirItem* item = new K3bDirItem( name, this, parent );

  setModified( true );

  emit newFileItems();

  return item;
}


KIO::filesize_t K3bDataDoc::size() const
{
  //return m_size;	
  return root()->k3bSize();
}


K3b::Msf K3bDataDoc::length() const
{
  // 1 block consists of 2048 bytes real data
  // and 1 block equals to 1 audio frame
  // so this is the way to calculate:

  return K3b::Msf(size() / 2048);
}


QString K3bDataDoc::documentType() const
{
  return QString::fromLatin1("data");
}


bool K3bDataDoc::loadDocumentData( QDomElement* rootElem )
{
  if( !root() )
    newDocument();

  QDomNodeList nodes = rootElem->childNodes();

  if( nodes.item(0).nodeName() != "general" ) {
    kdDebug() << "(K3bDataDoc) could not find 'general' section." << endl;
    return false;
  }
  if( !readGeneralDocumentData( nodes.item(0).toElement() ) )
    return false;


  // parse options
  // -----------------------------------------------------------------
  if( nodes.item(1).nodeName() != "options" ) {
    kdDebug() << "(K3bDataDoc) could not find 'options' section." << endl;
    return false;
  }
  if( !loadDocumentDataOptions( nodes.item(1).toElement() ) )
    return false;
  // -----------------------------------------------------------------



  // parse header
  // -----------------------------------------------------------------
  if( nodes.item(2).nodeName() != "header" ) {
    kdDebug() << "(K3bDataDoc) could not find 'header' section." << endl;
    return false;
  }
  if( !loadDocumentDataHeader( nodes.item(2).toElement() ) )
    return false;
  // -----------------------------------------------------------------



  // parse files
  // -----------------------------------------------------------------
  if( nodes.item(3).nodeName() != "files" ) {
    kdDebug() << "(K3bDataDoc) could not find 'files' section." << endl;
    return false;
  }

  if( m_root == 0 )
    m_root = new K3bRootItem( this );    

  QDomNodeList filesList = nodes.item(3).childNodes();
  for( uint i = 0; i < filesList.count(); i++ ) {

    QDomElement e = filesList.item(i).toElement();
    if( !loadDataItem( e, root() ) )
      return false;
  }

  // -----------------------------------------------------------------

  emit newFileItems();

  informAboutNotFoundFiles();

  return true;
}


bool K3bDataDoc::loadDocumentDataOptions( QDomElement elem )
{
  QDomNodeList headerList = elem.childNodes();
  for( uint i = 0; i < headerList.count(); i++ ) {

    QDomElement e = headerList.item(i).toElement();
    if( e.isNull() )
      return false;

    if( e.nodeName() == "rock_ridge")
      isoOptions().setCreateRockRidge( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "joliet")
      isoOptions().setCreateJoliet( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "udf")
      isoOptions().setCreateUdf( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_allow_lowercase")
      isoOptions().setISOallowLowercase( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_allow_period_at_begin")
      isoOptions().setISOallowPeriodAtBegin( e.attributeNode( "activated" ).value() == "yes" );
      
    else if( e.nodeName() == "iso_allow_31_char")
      isoOptions().setISOallow31charFilenames( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_omit_version_numbers")
      isoOptions().setISOomitVersionNumbers( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_omit_trailing_period")
      isoOptions().setISOomitTrailingPeriod( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_max_filename_length")
      isoOptions().setISOmaxFilenameLength( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_relaxed_filenames")
      isoOptions().setISOrelaxedFilenames( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_no_iso_translate")
      isoOptions().setISOnoIsoTranslate( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_allow_multidot")
      isoOptions().setISOallowMultiDot( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_untranslated_filenames")
      isoOptions().setISOuntranslatedFilenames( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "follow_symbolic_links")
      isoOptions().setFollowSymbolicLinks( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "create_trans_tbl")
      isoOptions().setCreateTRANS_TBL( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "hide_trans_tbl")
      isoOptions().setHideTRANS_TBL( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_level")
      isoOptions().setISOLevel( e.text().toInt() );

    else if( e.nodeName() == "discard_symlinks")
      isoOptions().setDiscardSymlinks( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "discard_broken_symlinks")
      isoOptions().setDiscardBrokenSymlinks( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "preserve_file_permissions")
      isoOptions().setPreserveFilePermissions( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "force_input_charset")
      isoOptions().setForceInputCharset( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "input_charset")
      isoOptions().setInputCharset( e.text() );

    else if( e.nodeName() == "whitespace_treatment" ) {
      if( e.text() == "strip" )
	isoOptions().setWhiteSpaceTreatment( K3bIsoOptions::strip );
      else if( e.text() == "extended" )
	isoOptions().setWhiteSpaceTreatment( K3bIsoOptions::extended );
      else if( e.text() == "extended" )
	isoOptions().setWhiteSpaceTreatment( K3bIsoOptions::replace );
      else
	isoOptions().setWhiteSpaceTreatment( K3bIsoOptions::noChange );
    }

    else if( e.nodeName() == "whitespace_replace_string")
      isoOptions().setWhiteSpaceTreatmentReplaceString( e.text() );

    else if( e.nodeName() == "data_track_mode" ) {
      if( e.text() == "mode1" )
	m_dataMode = K3b::MODE1;
      else if( e.text() == "mode2" )
	m_dataMode = K3b::MODE2;
      else
	m_dataMode = K3b::AUTO;
    }

    else if( e.nodeName() == "multisession" ) {
      QString mode = e.text();
      if( mode == "start" )
	setMultiSessionMode( START );
      else if( mode == "continue" )
	setMultiSessionMode( CONTINUE );
      else if( mode == "finish" )
	setMultiSessionMode( FINISH );
      else
	setMultiSessionMode( NONE );
    }

    else
      kdDebug() << "(K3bDataDoc) unknown option entry: " << e.nodeName() << endl;
  }

  return true;
}


bool K3bDataDoc::loadDocumentDataHeader( QDomElement headerElem )
{
  QDomNodeList headerList = headerElem.childNodes();
  for( uint i = 0; i < headerList.count(); i++ ) {

    QDomElement e = headerList.item(i).toElement();
    if( e.isNull() )
      return false;

    if( e.nodeName() == "volume_id" )
      isoOptions().setVolumeID( e.text() );

    else if( e.nodeName() == "application_id" )
      isoOptions().setApplicationID( e.text() );
    
    else if( e.nodeName() == "publisher" )
      isoOptions().setPublisher( e.text() );
    
    else if( e.nodeName() == "preparer" )
      isoOptions().setPreparer( e.text() );

    else if( e.nodeName() == "volume_set_id" )
      isoOptions().setVolumeSetId( e.text() );

    else if( e.nodeName() == "volume_set_size" )
      isoOptions().setVolumeSetSize( e.text().toInt() );

    else if( e.nodeName() == "volume_set_number" )
      isoOptions().setVolumeSetNumber( e.text().toInt() );

    else if( e.nodeName() == "system_id" )
      isoOptions().setSystemId( e.text() );

    else
      kdDebug() << "(K3bDataDoc) unknown header entry: " << e.nodeName() << endl;
  }

  return true;
}


bool K3bDataDoc::loadDataItem( QDomElement& elem, K3bDirItem* parent )
{
  if( elem.nodeName() == "file" ) {
    QDomElement urlElem = elem.firstChild().toElement();
    if( urlElem.isNull() ) {
      kdDebug() << "(K3bDataDoc) file-element without url!" << endl;
      return false;
    }

    if( !QFile::exists( urlElem.text() ) )
      m_notFoundFiles.append( urlElem.text() );
    else {

      // mkisofs seems to have a bug that prevents us to use filenames 
      // that contain one or more backslashes
      // -----------------------------------------------------------------------
      if( urlElem.text().contains( "\\\\" ) ) {
	m_mkisofsBuggyFiles.append( urlElem.text() );
      }
      // -----------------------------------------------------------------------

      else if( !elem.attribute( "bootimage" ).isEmpty() ) {
	K3bBootItem* bootItem = new K3bBootItem( urlElem.text(), this, parent, elem.attributeNode( "name" ).value() );
	if( elem.attribute( "bootimage" ) == "floppy" )
	  bootItem->setImageType( K3bBootItem::FLOPPY );
	else if( elem.attribute( "bootimage" ) == "harddisk" )
	  bootItem->setImageType( K3bBootItem::HARDDISK );
	else
	  bootItem->setImageType( K3bBootItem::NONE );
	bootItem->setNoBoot( elem.attribute( "no_boot" ) == "yes" );
	bootItem->setBootInfoTable( elem.attribute( "boot_info_table" ) == "yes" );
	bootItem->setLoadSegment( elem.attribute( "load_segment" ).toInt() );
	bootItem->setLoadSize( elem.attribute( "load_size" ).toInt() );

	m_bootImages.append(bootItem);

	// TODO: save location of the cataloge file
	createBootCatalogeItem(parent);
      }
      else {
	(void)new K3bFileItem( urlElem.text(), this, parent, elem.attributeNode( "name" ).value() );
      }
    }
  }
  else if( elem.nodeName() == "directory" ) {
    K3bDirItem* newDirItem = new K3bDirItem( elem.attributeNode( "name" ).value(), this, parent );
    QDomNodeList childNodes = elem.childNodes();
    for( uint i = 0; i < childNodes.count(); i++ ) {
      
      QDomElement e = childNodes.item(i).toElement();
      if( !loadDataItem( e, newDirItem ) )
	return false;
    }

  }
  else {
    kdDebug() << "(K3bDataDoc) wrong tag in files-section: " << elem.nodeName() << endl;
    return false;
  }



  return true;
}


bool K3bDataDoc::saveDocumentData( QDomElement* docElem )
{
  QDomDocument doc = docElem->ownerDocument();

  saveGeneralDocumentData( docElem );

  // all options
  // ----------------------------------------------------------------------
  QDomElement optionsElem = doc.createElement( "options" );
  saveDocumentDataOptions( optionsElem );
  docElem->appendChild( optionsElem );
  // ----------------------------------------------------------------------

  // the header stuff
  // ----------------------------------------------------------------------
  QDomElement headerElem = doc.createElement( "header" );
  docElem->appendChild( headerElem );


  // now do the "real" work: save the entries
  // ----------------------------------------------------------------------
  QDomElement topElem = doc.createElement( "files" );

  QPtrListIterator<K3bDataItem> it( *root()->children() );
  for( ; it.current(); ++it ) {
    saveDataItem( it.current(), &doc, &topElem );
  }

  docElem->appendChild( topElem );
  // ----------------------------------------------------------------------

  return true;
}


void K3bDataDoc::saveDocumentDataOptions( QDomElement& optionsElem )
{
  QDomDocument doc = optionsElem.ownerDocument();

  QDomElement topElem = doc.createElement( "rock_ridge" );
  topElem.setAttribute( "activated", isoOptions().createRockRidge() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "joliet" );
  topElem.setAttribute( "activated", isoOptions().createJoliet() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "udf" );
  topElem.setAttribute( "activated", isoOptions().createUdf() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "iso_allow_lowercase" );
  topElem.setAttribute( "activated", isoOptions().ISOallowLowercase() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "iso_allow_period_at_begin" );
  topElem.setAttribute( "activated", isoOptions().ISOallowPeriodAtBegin() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "iso_allow_31_char" );
  topElem.setAttribute( "activated", isoOptions().ISOallow31charFilenames() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "iso_omit_version_numbers" );
  topElem.setAttribute( "activated", isoOptions().ISOomitVersionNumbers() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "iso_omit_trailing_period" );
  topElem.setAttribute( "activated", isoOptions().ISOomitTrailingPeriod() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "iso_max_filename_length" );
  topElem.setAttribute( "activated", isoOptions().ISOmaxFilenameLength() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "iso_relaxed_filenames" );
  topElem.setAttribute( "activated", isoOptions().ISOrelaxedFilenames() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "iso_no_iso_translate" );
  topElem.setAttribute( "activated", isoOptions().ISOnoIsoTranslate() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "iso_allow_multidot" );
  topElem.setAttribute( "activated", isoOptions().ISOallowMultiDot() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "iso_untranslated_filenames" );
  topElem.setAttribute( "activated", isoOptions().ISOuntranslatedFilenames() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "follow_symbolic_links" );
  topElem.setAttribute( "activated", isoOptions().followSymbolicLinks() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "create_trans_tbl" );
  topElem.setAttribute( "activated", isoOptions().createTRANS_TBL() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "hide_trans_tbl" );
  topElem.setAttribute( "activated", isoOptions().hideTRANS_TBL() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "iso_level" );
  topElem.appendChild( doc.createTextNode( QString::number(isoOptions().ISOLevel()) ) );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "discard_symlinks" );
  topElem.setAttribute( "activated", isoOptions().discardSymlinks() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "discard_broken_symlinks" );
  topElem.setAttribute( "activated", isoOptions().discardBrokenSymlinks() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "preserve_file_permissions" );
  topElem.setAttribute( "activated", isoOptions().preserveFilePermissions() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "force_input_charset" );
  topElem.setAttribute( "activated", isoOptions().forceInputCharset() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "input_charset" );
  topElem.appendChild( doc.createTextNode( isoOptions().inputCharset() ) );
  optionsElem.appendChild( topElem );


  topElem = doc.createElement( "whitespace_treatment" );
  switch( isoOptions().whiteSpaceTreatment() ) {
  case K3bIsoOptions::strip:
    topElem.appendChild( doc.createTextNode( "strip" ) );
    break;
  case K3bIsoOptions::extended:
    topElem.appendChild( doc.createTextNode( "extended" ) );
    break;
  case K3bIsoOptions::replace:
    topElem.appendChild( doc.createTextNode( "replace" ) );
    break;
  default:
    topElem.appendChild( doc.createTextNode( "noChange" ) );
    break;
  }
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "whitespace_replace_string" );
  topElem.appendChild( doc.createTextNode( isoOptions().whiteSpaceTreatmentReplaceString() ) );
  optionsElem.appendChild( topElem );

  topElem = doc.createElement( "data_track_mode" );
  if( m_dataMode == K3b::MODE1 )
    topElem.appendChild( doc.createTextNode( "mode1" ) );
  else if( m_dataMode == K3b::MODE2 )
    topElem.appendChild( doc.createTextNode( "mode2" ) );
  else
    topElem.appendChild( doc.createTextNode( "auto" ) );
  optionsElem.appendChild( topElem );


  // save multisession
  topElem = doc.createElement( "multisession" );
  switch( m_multisessionMode ) {
  case START:
    topElem.appendChild( doc.createTextNode( "start" ) );
    break;
  case CONTINUE:
    topElem.appendChild( doc.createTextNode( "continue" ) );
    break;
  case FINISH:
    topElem.appendChild( doc.createTextNode( "finish" ) );
    break;
  default:
    topElem.appendChild( doc.createTextNode( "none" ) );
    break;
  }
  optionsElem.appendChild( topElem );
  // ----------------------------------------------------------------------
}


void K3bDataDoc::saveDocumentDataHeader( QDomElement& headerElem )
{
  QDomDocument doc = headerElem.ownerDocument();

  QDomElement topElem = doc.createElement( "volume_id" );
  topElem.appendChild( doc.createTextNode( isoOptions().volumeID() ) );
  headerElem.appendChild( topElem );

  topElem = doc.createElement( "volume_set_id" );
  topElem.appendChild( doc.createTextNode( isoOptions().volumeSetId() ) );
  headerElem.appendChild( topElem );

  topElem = doc.createElement( "volume_set_size" );
  topElem.appendChild( doc.createTextNode( QString::number(isoOptions().volumeSetSize()) ) );
  headerElem.appendChild( topElem );

  topElem = doc.createElement( "volume_set_number" );
  topElem.appendChild( doc.createTextNode( QString::number(isoOptions().volumeSetNumber()) ) );
  headerElem.appendChild( topElem );

  topElem = doc.createElement( "system_id" );
  topElem.appendChild( doc.createTextNode( isoOptions().systemId() ) );
  headerElem.appendChild( topElem );

  topElem = doc.createElement( "application_id" );
  topElem.appendChild( doc.createTextNode( isoOptions().applicationID() ) );
  headerElem.appendChild( topElem );

  topElem = doc.createElement( "publisher" );
  topElem.appendChild( doc.createTextNode( isoOptions().publisher() ) );
  headerElem.appendChild( topElem );

  topElem = doc.createElement( "preparer" );
  topElem.appendChild( doc.createTextNode( isoOptions().preparer() ) );
  headerElem.appendChild( topElem );
  // ----------------------------------------------------------------------
}


void K3bDataDoc::saveDataItem( K3bDataItem* item, QDomDocument* doc, QDomElement* parent )
{
  if( K3bFileItem* fileItem = dynamic_cast<K3bFileItem*>( item ) ) {
    if( m_oldSession.contains( fileItem ) ) {
      kdDebug() << "(K3bDataDoc) ignoring fileitem " << fileItem->k3bName() << " from old session while saving..." << endl;
    }
    else {
      QDomElement topElem = doc->createElement( "file" );
      topElem.setAttribute( "name", fileItem->k3bName() );
      QDomElement subElem = doc->createElement( "url" );
      subElem.appendChild( doc->createTextNode( fileItem->localPath() ) );
      topElem.appendChild( subElem );
      
      parent->appendChild( topElem );
      
      // add boot options as attributes to preserve compatibility to older K3b versions
      if( K3bBootItem* bootItem = dynamic_cast<K3bBootItem*>( fileItem ) ) {
	if( bootItem->imageType() == K3bBootItem::FLOPPY )
	  topElem.setAttribute( "bootimage", "floppy" );
	else if( bootItem->imageType() == K3bBootItem::HARDDISK )
	  topElem.setAttribute( "bootimage", "harddisk" );
	else
	  topElem.setAttribute( "bootimage", "none" );
	
	topElem.setAttribute( "no_boot", bootItem->noBoot() ? "yes" : "no" );
	topElem.setAttribute( "boot_info_table", bootItem->bootInfoTable() ? "yes" : "no" );
	topElem.setAttribute( "load_segment", QString::number( bootItem->loadSegment() ) );
	topElem.setAttribute( "load_size", QString::number( bootItem->loadSize() ) );
      }
    }
  }
  else if( K3bDirItem* dirItem = dynamic_cast<K3bDirItem*>( item ) ) {
    QDomElement topElem = doc->createElement( "directory" );
    topElem.setAttribute( "name", dirItem->k3bName() );

    QPtrListIterator<K3bDataItem> it( *dirItem->children() );
    for( ; it.current(); ++it ) {
      saveDataItem( it.current(), doc, &topElem );
    }

    parent->appendChild( topElem );
  }
}


void K3bDataDoc::removeItem( K3bDataItem* item )
{
  if( !item )
    return;

  if( item->isRemoveable() ) {
    // the item takes care of it's parent!
    //    m_size -= item->k3bSize();
    emit itemRemoved( item );

    // check if any items are pending to be added to this dir (if it's a dir)
    if( item->isDir() ) {
      PrivateItemToAdd* pi = m_queuedToAddItems.first();
      while( pi ) {
	if( ((K3bDirItem*)item)->isSubItem( pi->parent ) ) {
	  PrivateItemToAdd* pi2 = m_queuedToAddItems.take();
	  delete pi2;
	  pi = m_queuedToAddItems.current();
	}
	else
	  pi = m_queuedToAddItems.next();
      }
    }

    delete item;
  }
  else
    kdDebug() << "(K3bDataDoc) tried to remove non-removable entry!" << endl;
}


void K3bDataDoc::moveItem( K3bDataItem* item, K3bDirItem* newParent )
{
  if( !item || !newParent ) {
    kdDebug() << "(K3bDataDoc) item or parentitem was NULL while moving." << endl;
    return;
  }

  if( !item->isMoveable() ) {
    kdDebug() << "(K3bDataDoc) item is not movable! " << endl;
    return;
  }

  // check if newParent is subdir of item
  if( K3bDirItem* dirItem = dynamic_cast<K3bDirItem*>(item) ) {
    if( dirItem->isSubItem( newParent ) ) {
      return;
    }
  }


  item->reparent( newParent );

  emit newFileItems();
}


void K3bDataDoc::moveItems( QPtrList<K3bDataItem> itemList, K3bDirItem* newParent )
{
  if( !newParent ) {
    kdDebug() << "(K3bDataDoc) tried to move items to nowhere...!" << endl;
    return;
  }

  QPtrListIterator<K3bDataItem> it( itemList );
  for( ; it.current(); ++it ) {
    // check if newParent is subdir of item
    if( K3bDirItem* dirItem = dynamic_cast<K3bDirItem*>( it.current() ) ) {
      if( dirItem->isSubItem( newParent ) ) {
	continue;
      }
    }

    it.current()->reparent( newParent );
  }

  emit newFileItems();
}


QString K3bDataDoc::writePathSpec( const QString& filename )
{
  QFile file( filename );
  if( !file.open( IO_WriteOnly ) ) {
    kdDebug() << "(K3bDataDoc) Could not open path-spec-file " << filename << endl;
    return QString::null;
  }
	
  QTextStream t(&file);

  // start writing the path-specs
  // iterate over all the dataItems
  K3bDataItem* item = root()->nextSibling();
	
  while( item ) {
    t << treatWhitespace(item->k3bPath()) << "=" << item->localPath() << "\n";
		
    item = item->nextSibling();
  }
	
  file.close();
  return filename;
}


const QString& K3bDataDoc::dummyDir()
{
  QDir _appDir( locateLocal( "appdata", "temp/" ) );
  if( !_appDir.cd( "dummydir" ) ) {
    _appDir.mkdir( "dummydir" );
    _appDir.cd( "dummydir" );
  }
  m_dummyDir = _appDir.absPath() + "/";
	
  return m_dummyDir;
}

K3bBurnJob* K3bDataDoc::newBurnJob()
{
  return new K3bDataJob( this );
}


QString K3bDataDoc::treatWhitespace( const QString& path )
{

  // TODO:
  // It could happen that two files with different names
  // will have the same name after the treatment
  // Perhaps we should add a number at the end or something
  // similar (s.a.)


  if( isoOptions().whiteSpaceTreatment() != K3bIsoOptions::noChange ) {
    QString _result;
    int _startPos = path.findRev('/');
    if( _startPos == -1 ) _startPos = 0;
    else _startPos += 1;
    _result = path.left( _startPos );
  	
    if( isoOptions().whiteSpaceTreatment() == K3bIsoOptions::replace ) {
      const QString& rs = isoOptions().whiteSpaceTreatmentReplaceString();
      for( uint i = _startPos; i < path.length(); i++ ) {
	if( path[i] == ' ' )
	  _result.append(rs);
	else
	  _result.append( path[i] );
      }
    }
    else if( isoOptions().whiteSpaceTreatment() == K3bIsoOptions::strip ) {
      for( uint i = _startPos; i < path.length(); i++ ) {
	if( path[i] != ' ' )
	  _result.append( path[i] );
      }
    }
    else if( isoOptions().whiteSpaceTreatment() == K3bIsoOptions::extended ) {
      for( uint i = _startPos; i < path.length(); i++ ) {
	if( path[i] == ' ' ) {
	  if( path[i+1] != ' ' )
	    _result.append( path[++i].upper() );
	}
	else
	  _result.append( path[i] );
      }
    }
		
    kdDebug() << "(K3bDataDoc) converted " << path << " to " << _result << endl;
    return _result;
  }
  else
    return path;
}


void K3bDataDoc::informAboutNotFoundFiles()
{
  if( !m_notFoundFiles.isEmpty() ) {
    KMessageBox::informationList( firstView(), i18n("Could not find the following files:"), 
 				  m_notFoundFiles, i18n("Not found") );
    m_notFoundFiles.clear();
  }


  // mkisofs seems to have a bug that prevents us to use filenames 
  // that contain one or more backslashes
  // -----------------------------------------------------------------------
  if( !m_mkisofsBuggyFiles.isEmpty() ) {
     KMessageBox::informationList( firstView(), i18n("Due to a bug in mkisofs, K3b is unable to handle "
 						  "filenames that contain more than one backslash:"),
	          				  m_mkisofsBuggyFiles, i18n("Sorry") );
    m_mkisofsBuggyFiles.clear();
  }
  // -----------------------------------------------------------------------
}


void K3bDataDoc::loadDefaultSettings( KConfig* c )
{
  K3bDoc::loadDefaultSettings(c);

  m_isoOptions = K3bIsoOptions::load( c );

  QString datamode = c->readEntry( "data_track_mode" );
  if( datamode == "mode1" )
    setDataMode( K3b::MODE1 );
  else if( datamode == "mode2" )
    setDataMode( K3b::MODE2 );
  else
    setDataMode( K3b::AUTO );
}


void K3bDataDoc::setMultiSessionMode( int mode )
{
  m_multisessionMode = mode;

  if( m_multisessionMode != CONTINUE && m_multisessionMode != FINISH )
    clearImportedSession();
}


void K3bDataDoc::importSession( const QString& path )
{
  // remove previous imported sessions
  clearImportedSession();

  // set multisession option
  if( m_multisessionMode != CONTINUE && m_multisessionMode != FINISH )
    m_multisessionMode = CONTINUE;

  // add all files from cd as readonly fileitems
  KProgressDialog d( 0, 0, i18n("Importing session"), i18n("Importing old session from %1").arg(path) );
  d.show();

  int dirFilter = QDir::All | QDir::Hidden | QDir::System;

  QStringList dlist = QDir( path ).entryList( dirFilter );
  dlist.remove(".");
  dlist.remove("..");
  
  d.progressBar()->setTotalSteps( dlist.count() );

  for( QStringList::Iterator it = dlist.begin(); it != dlist.end(); ++it ) {
    createSessionImportItems( path + "/" + *it, root(), &d );
    if( d.wasCancelled() ) {
      clearImportedSession();
      return;
    }
    d.progressBar()->setValue( d.progressBar()->value() + 1 );
  }

  emit newFileItems();

  d.hide();
}


void K3bDataDoc::createSessionImportItems( const QString& path, K3bDirItem* parent, KProgressDialog* d )
{
  kapp->processEvents();

  if( d->wasCancelled() ) {
    return;
  }

  QFileInfo newF(path);
  K3bDataItem* oldItem = parent->find( newF.fileName() );
  if( oldItem ) {
    // remove the item already in the project since mkisofs is not
    // capable of overwriting files
    if( (oldItem->isDir() && !newF.isDir()) ||
	!oldItem->isDir() ) {
	delete oldItem;
	oldItem = 0;
    }
  }

  if( newF.isDir() && !newF.isSymLink() ) {
    K3bDirItem* dir = 0;
    if( !oldItem ) {
      dir = new K3bDirItem( newF.fileName(), this, parent );
      dir->setRemoveable(false);
      dir->setRenameable(false);
      dir->setMoveable(false);
      dir->setHideable(false);
      dir->setWriteToCd(false);
      dir->setExtraInfo( i18n("From previous session") );
      m_oldSession.append( dir );
    }
    else
      dir = (K3bDirItem*)oldItem;

    int dirFilter = QDir::All | QDir::Hidden | QDir::System;
    QStringList dlist = QDir( path ).entryList( dirFilter );
    dlist.remove(".");
    dlist.remove("..");

    for( QStringList::Iterator it = dlist.begin(); it != dlist.end(); ++it ) {
      createSessionImportItems( path + "/" + *it, dir, d );
    }
  }
  else {
    K3bFileItem* item = new K3bFileItem( newF.absFilePath(), this, parent, newF.fileName() );
    item->setRemoveable(false);
    item->setRenameable(false);
    item->setMoveable(false);
    item->setHideable(false);
    item->setWriteToCd(false);
    item->setExtraInfo( i18n("From previous session") );
    m_oldSession.append( item );
    //    m_size += item->k3bSize();
  }
}


void K3bDataDoc::clearImportedSession()
{
  m_oldSession.setAutoDelete(false);
  K3bDataItem* item = m_oldSession.first();
  while( !m_oldSession.isEmpty() ) {
    if( item == 0 )
      item = m_oldSession.first();

    if( item->isDir() ) {
      K3bDirItem* dir = (K3bDirItem*)item;
      if( dir->numDirs() + dir->numFiles() == 0 ) {
	// this imported dir is not needed anymore 
	// since it is empty
	m_oldSession.remove();
	emit itemRemoved( item );
	delete dir;
      }
      else {
	for( QPtrListIterator<K3bDataItem> it( *dir->children() ); it.current(); ++it ) {
	  if( !m_oldSession.contains(it.current()) ) {
	    m_oldSession.remove();
	    // now the dir becomes a totally normal dir
	    dir->setRemoveable(true);
	    dir->setRenameable(true);
	    dir->setMoveable(true);
	    dir->setHideable(true);
	    dir->setWriteToCd(true);
	    dir->setExtraInfo( "" );
	    break;
	  }
	}
      }
    }
    else {
      m_oldSession.remove();
      //      m_size -= item->k3bSize();
      emit itemRemoved( item );
      delete item;
    }

    item = m_oldSession.next();
  }
}


K3bDirItem* K3bDataDoc::bootImageDir()
{
  K3bDataItem* b = m_root->find( "boot" );
  if( !b ) {
    b = new K3bDirItem( "boot", this, m_root );
    setModified( true );
    emit newFileItems();
  }

  // if we cannot create the dir because there is a file named boot just use the root dir
  if( !b->isDir() )
    return m_root;
  else 
    return static_cast<K3bDirItem*>(b);
}


K3bBootItem* K3bDataDoc::createBootItem( const QString& filename, K3bDirItem* dir )
{
  if( !dir )
    dir = bootImageDir();

  // TODO: check if a file with the same name already exists
  K3bBootItem* boot = new K3bBootItem( filename, 
				       this, dir );

  m_bootImages.append(boot);

  createBootCatalogeItem(dir);

  emit newFileItems();

  return boot;
}


K3bDataItem* K3bDataDoc::createBootCatalogeItem( K3bDirItem* dir )
{
  if( !m_bootCataloge ) {
    K3bSpecialDataItem* b = new K3bSpecialDataItem( this, 0, dir, "boot.cataloge" );
    m_bootCataloge = b;
    m_bootCataloge->setRemoveable(false);
    m_bootCataloge->setHideable(false);
    m_bootCataloge->setWriteToCd(false);
    m_bootCataloge->setExtraInfo( i18n("El Torito boot cataloge file") );
    b->setMimeType( i18n("Boot cataloge") );
  }

  return m_bootCataloge;
}


void K3bDataDoc::removeBootItem( K3bBootItem* item )
{
  m_bootImages.removeRef(item);
  if( m_bootImages.isEmpty() ) {
    emit itemRemoved( m_bootCataloge );
    delete m_bootCataloge;
    m_bootCataloge = 0;
  }
}

#include "k3bdatadoc.moc"
