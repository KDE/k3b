/***************************************************************************
                          k3bdatadoc.cpp  -  description
                             -------------------
    begin                : Sun Apr 22 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bdatadoc.h"
#include "k3bfileitem.h"
#include "k3bdiritem.h"
#include "k3bdataview.h"
#include "k3bdatajob.h"
#include "../k3b.h"
#include "../kstringlistdialog.h"

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



K3bDataDoc::K3bDataDoc( QObject* parent )
  : K3bDoc( parent )
{
  m_docType = DATA;
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
  if( m_root )
    delete m_root;
		
  m_root = new K3bRootItem( this );
  m_size = 0;
	
  m_name = "Dummyname";
  m_applicationID = "K3B";    // thy name on every cd!
  m_isoImage = QString::null;

  m_multisessionMode = NONE;


  m_ISOallowLowercase = false;   // -allow-lowercase
  m_ISOallowPeriodAtBegin = false;   // -L
  m_ISOallow31charFilenames = false;  // -I
  m_ISOomitVersionNumbers = false;   // -N
  m_ISOmaxFilenameLength = false;     // -max-iso9660-filenames (forces -N)
  m_ISOrelaxedFilenames = false;      // -relaxed-filenames
  m_ISOnoIsoTranslate = false;        // -no-iso-translate
  m_ISOallowMultiDot = false;          // -allow-multidot
  m_ISOuntranslatedFilenames = false;   // -U (forces -d, -I, -L, -N, -relaxed-filenames, -allow-lowercase, -allow-multidot, -no-iso-translate)
  m_noDeepDirectoryRelocation = false;   // -D
  m_followSymbolicLinks = false;       // -f
  m_hideRR_MOVED = false;  // -hide-rr-moved
  m_createTRANS_TBL = false;    // -T
  m_hideTRANS_TBL = false;    // -hide-joliet-trans-tbl
  m_padding = false;           // -pad

  m_bForceInputCharset = false;
  m_inputCharset = "iso8859-1";
	
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


void K3bDataDoc::addUrl( const KURL& url )
{
  KURL::List urls;
  urls.append(url);
  slotAddUrlsToDir( urls );
}


void K3bDataDoc::addUrls( const KURL::List& urls )
{
  slotAddUrlsToDir( urls );
}


void K3bDataDoc::slotAddUrlsToDir( const KURL::List& urls, K3bDirItem* dirItem )
{
  if( !dirItem ) {
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
	  m_queuedToAddItems.enqueue( new PrivateItemToAdd(url.path(), dirItem ) );
      }
      else
	m_notFoundFiles.append( url.path() );
    }

  m_queuedToAddItemsTimer->start(0);
  k3bMain()->showBusyInfo( i18n( "Adding files to Project %1..." ).arg( volumeID() ) );
}


void K3bDataDoc::slotAddQueuedItems()
{
  PrivateItemToAdd* item = m_queuedToAddItems.dequeue();
  if( item ) {
    m_queuedToAddItemsTimer->stop();

    setModified( true );

    if( !item->fileInfo.exists() )
      return;
	
    if( item->fileInfo.isDir() ) {
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


void K3bDataDoc::createDirItem( QFileInfo& f, K3bDirItem* parent )
{
  QString newName = f.fileName();

  if( newName.isEmpty() ) {
    kdDebug() << "(K3bDataDoc) tried to create dir without name." << endl;
    return;
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
      KMessageBox::error( k3bMain(), i18n("Found recursion in directory tree. Omitting\n%1").arg(f.absFilePath()) );
      return;
    }
  }


  if( nameAlreadyInDir( newName, parent ) ) {
    k3bMain()->config()->setGroup("Data project settings");
    bool dropDoubles = k3bMain()->config()->readBoolEntry( "Drop doubles", false );
    if( dropDoubles )
      return;

    bool ok = true;
    while( ok && nameAlreadyInDir( newName, parent ) ) {
      newName = KLineEditDlg::getText( i18n("Directory with that name already exists. Please enter new name."), 
				       newName, &ok, k3bMain() );
    }
    if( !ok )
      return;
  }

  K3bDirItem* newDirItem = new K3bDirItem( newName, this, parent );
  
  QStringList dlist = QDir( f.absFilePath() ).entryList();
  dlist.remove(".");
  dlist.remove("..");
  
  for( QStringList::Iterator it = dlist.begin(); it != dlist.end(); ++it ) {
    QFileInfo newF(f.absFilePath() + "/" + *it);
    if( newF.isDir() )
      m_queuedToAddItems.enqueue( new PrivateItemToAdd( newF, newDirItem ) );
    else
      createFileItem( newF, newDirItem );
  }
}


void K3bDataDoc::createFileItem( QFileInfo& f, K3bDirItem* parent )
{
  QString newName = f.fileName();


  // filter symlinks and follow them
  while( f.isSymLink() ) {
    if( f.readLink().startsWith("/") )
      f.setFile( f.readLink() );
    else
      f.setFile( f.dirPath() + "/" + f.readLink() );
    
    // check if it was a corrupted symlink
    if( !f.exists() ) {
      kdDebug() << "(K3bDataDoc) corrupted symlink: " << f.absFilePath() << endl;
      m_notFoundFiles.append( f.absFilePath() );
      return;
    }
  }

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
    k3bMain()->config()->setGroup("Data project settings");
    bool dropDoubles = k3bMain()->config()->readBoolEntry( "Drop doubles", false );
    if( dropDoubles )
      return;

    bool ok = true;
    do {
      newName = KLineEditDlg::getText( i18n("File with that name already exists. Please enter new name."), 
				       newName, &ok, k3bMain() );
    } while( ok && nameAlreadyInDir( newName, parent ) );

    if( !ok )
      return;
  }


  K3bFileItem* newK3bItem = new K3bFileItem( f.absFilePath(), this, parent, newName );
  m_size += newK3bItem->k3bSize();
}


bool K3bDataDoc::nameAlreadyInDir( const QString& name, K3bDirItem* dir )
{
  if( !dir ) {
    return false;
  }

  QListIterator<K3bDataItem> it( *dir->children() );
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


unsigned long K3bDataDoc::size() const
{
  return m_size;	
  //  return root()->k3bSize();
}


unsigned long K3bDataDoc::length() const
{
  // 1 block consists of 2048 bytes real data
  // and 1 block equals to 1 audio frame
  // so this is the way to calculate:

  return size() / 2048;
}


QString K3bDataDoc::documentType() const
{
  return QString::fromLatin1("k3b_data_project");
}


bool K3bDataDoc::loadDocumentData( QDomDocument* doc )
{
  if( doc->doctype().name() != documentType() )
    return false;

  if( !root() )
    newDocument();

  QDomNodeList nodes = doc->documentElement().childNodes();

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
  QDomNodeList optionList = nodes.item(1).childNodes();
  for( uint i = 0; i < optionList.count(); i++ ) {

    QDomElement e = optionList.item(i).toElement();
    if( e.isNull() )
      return false;

    if( e.nodeName() == "rock_ridge")
      setCreateRockRidge( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "joliet")
      setCreateJoliet( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_allow_lowercase")
      setISOallowLowercase( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_allow_period_at_begin")
      setISOallowPeriodAtBegin( e.attributeNode( "activated" ).value() == "yes" );
      
    else if( e.nodeName() == "iso_allow_31_char")
      setISOallow31charFilenames( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_omit_version_numbers")
      setISOomitVersionNumbers( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_max_filename_length")
      setISOmaxFilenameLength( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_relaxed_filenames")
      setISOrelaxedFilenames( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_no_iso_translate")
      setISOnoIsoTranslate( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_allow_multidot")
      setISOallowMultiDot( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_untranslated_filenames")
      setISOuntranslatedFilenames( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "no_deep_dir_relocation")
      setNoDeepDirectoryRelocation( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "follow_symbolic_links")
      setFollowSymbolicLinks( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "hide_rr_moved")
      setHideRR_MOVED( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "create_trans_tbl")
      setCreateTRANS_TBL( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "hide_trans_tbl")
      setHideTRANS_TBL( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "padding")
      setPadding( e.attributeNode( "activated" ).value() == "yes" );

    else if( e.nodeName() == "iso_level")
      setISOLevel( e.text().toInt() );

    else
      kdDebug() << "(K3bDataDoc) unknown option entry: " << e.nodeName() << endl;
  }
  // -----------------------------------------------------------------



  // parse header
  // -----------------------------------------------------------------
  if( nodes.item(2).nodeName() != "header" ) {
    kdDebug() << "(K3bDataDoc) could not find 'header' section." << endl;
    return false;
  }
  QDomNodeList headerList = nodes.item(2).childNodes();
  for( uint i = 0; i < headerList.count(); i++ ) {

    QDomElement e = headerList.item(i).toElement();
    if( e.isNull() )
      return false;

    if( e.nodeName() == "volume_id" )
      setVolumeID( e.text() );

    else if( e.nodeName() == "application_id" )
      setApplicationID( e.text() );
    
    else if( e.nodeName() == "publisher" )
      setPublisher( e.text() );
    
    else if( e.nodeName() == "preparer" )
      setPreparer( e.text() );

    else if( e.nodeName() == "volume_set_id" )
      setVolumeSetId( e.text() );

    else if( e.nodeName() == "system_id" )
      setSystemId( e.text() );

    else
      kdDebug() << "(K3bDataDoc) unknown header entry: " << e.nodeName() << endl;
    
  }
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

      else {
	K3bFileItem* newK3bItem = new K3bFileItem( urlElem.text(), this, parent, elem.attributeNode( "name" ).value() );
	m_size += newK3bItem->k3bSize();
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


bool K3bDataDoc::saveDocumentData( QDomDocument* doc )
{
  doc->appendChild( doc->createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );

  QDomElement docElem = doc->createElement( documentType() );

  saveGeneralDocumentData( &docElem );


  // all options
  // ----------------------------------------------------------------------
  QDomElement optionsElem = doc->createElement( "options" );

  QDomElement topElem = doc->createElement( "rock_ridge" );
  topElem.setAttribute( "activated", createRockRidge() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "joliet" );
  topElem.setAttribute( "activated", createJoliet() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "iso_allow_lowercase" );
  topElem.setAttribute( "activated", ISOallowLowercase() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "iso_allow_period_at_begin" );
  topElem.setAttribute( "activated", ISOallowPeriodAtBegin() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "iso_allow_31_char" );
  topElem.setAttribute( "activated", ISOallow31charFilenames() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "iso_omit_version_numbers" );
  topElem.setAttribute( "activated", ISOomitVersionNumbers() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "iso_max_filename_length" );
  topElem.setAttribute( "activated", ISOmaxFilenameLength() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "iso_relaxed_filenames" );
  topElem.setAttribute( "activated", ISOrelaxedFilenames() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "iso_no_iso_translate" );
  topElem.setAttribute( "activated", ISOnoIsoTranslate() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "iso_allow_multidot" );
  topElem.setAttribute( "activated", ISOallowMultiDot() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "iso_untranslated_filenames" );
  topElem.setAttribute( "activated", ISOuntranslatedFilenames() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "no_deep_dir_relocation" );
  topElem.setAttribute( "activated", noDeepDirectoryRelocation() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "follow_symbolic_links" );
  topElem.setAttribute( "activated", followSymbolicLinks() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "hide_rr_moved" );
  topElem.setAttribute( "activated", hideRR_MOVED() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "create_trans_tbl" );
  topElem.setAttribute( "activated", createTRANS_TBL() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "hide_trans_tbl" );
  topElem.setAttribute( "activated", hideTRANS_TBL() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "padding" );
  topElem.setAttribute( "activated", padding() ? "yes" : "no" );
  optionsElem.appendChild( topElem );

  topElem = doc->createElement( "iso_level" );
  topElem.appendChild( doc->createTextNode( QString::number(ISOLevel()) ) );
  optionsElem.appendChild( topElem );

  docElem.appendChild( optionsElem );
  // ----------------------------------------------------------------------


  // the header stuff
  // ----------------------------------------------------------------------
  QDomElement headerElem = doc->createElement( "header" );

  topElem = doc->createElement( "volume_id" );
  topElem.appendChild( doc->createTextNode( volumeID() ) );
  headerElem.appendChild( topElem );

  topElem = doc->createElement( "volume_set_id" );
  topElem.appendChild( doc->createTextNode( volumeSetId() ) );
  headerElem.appendChild( topElem );

  topElem = doc->createElement( "system_id" );
  topElem.appendChild( doc->createTextNode( systemId() ) );
  headerElem.appendChild( topElem );

  topElem = doc->createElement( "application_id" );
  topElem.appendChild( doc->createTextNode( applicationID() ) );
  headerElem.appendChild( topElem );

  topElem = doc->createElement( "publisher" );
  topElem.appendChild( doc->createTextNode( publisher() ) );
  headerElem.appendChild( topElem );

  topElem = doc->createElement( "preparer" );
  topElem.appendChild( doc->createTextNode( preparer() ) );
  headerElem.appendChild( topElem );

  docElem.appendChild( headerElem );
  // ----------------------------------------------------------------------



  // now do the "real" work: save the entries
  // ----------------------------------------------------------------------
  topElem = doc->createElement( "files" );

  QListIterator<K3bDataItem> it( *root()->children() );
  for( ; it.current(); ++it ) {
    saveDataItem( it.current(), doc, &topElem );
  }

  docElem.appendChild( topElem );
  // ----------------------------------------------------------------------

  doc->appendChild( docElem );


  return true;
}



void K3bDataDoc::saveDataItem( K3bDataItem* item, QDomDocument* doc, QDomElement* parent )
{
  if( K3bFileItem* fileItem = dynamic_cast<K3bFileItem*>( item ) ) {
    QDomElement topElem = doc->createElement( "file" );
    topElem.setAttribute( "name", fileItem->k3bName() );
    QDomElement subElem = doc->createElement( "url" );
    subElem.appendChild( doc->createTextNode( fileItem->localPath() ) );
    topElem.appendChild( subElem );

    parent->appendChild( topElem );
  }
  else if( K3bDirItem* dirItem = dynamic_cast<K3bDirItem*>( item ) ) {
    QDomElement topElem = doc->createElement( "directory" );
    topElem.setAttribute( "name", dirItem->k3bName() );

    QListIterator<K3bDataItem> it( *dirItem->children() );
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

  if( item == root() )
    kdDebug() << "(K3bDataDoc) tried to remove root-entry!" << endl;
  else {
    emit itemRemoved( item );
    
    m_size -= item->k3bSize();

    // the item takes care of it's parent!
    delete item;
  }
}


void K3bDataDoc::moveItem( K3bDataItem* item, K3bDirItem* newParent )
{
  if( !item || !newParent ) {
    kdDebug() << "(K3bDataDoc) item or parentitem was NULL while moving." << endl;
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

  QListIterator<K3bDataItem> it( itemList );
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
  }
  m_dummyDir = _appDir.absPath() + "/";
	
  // TODO: test if dummy dir is empty
	
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


  if( whiteSpaceTreatment() != K3bDataDoc::normal ) {
    QString _result;
    int _startPos = path.findRev('/');
    if( _startPos == -1 ) _startPos = 0;
    else _startPos += 1;
    _result = path.left( _startPos );
  	
    if( whiteSpaceTreatment() == K3bDataDoc::convertToUnderScore ) {
      // if QString is saved as an array this code is OK
      for( uint i = _startPos; i < path.length(); i++ ) {
	if( path[i] == ' ' )
	  _result.append('_');
	else
	  _result.append( path[i] );
      }
    }
    else if( whiteSpaceTreatment() == K3bDataDoc::strip ) {
      // if QString is saved as an array this code is OK
      for( uint i = _startPos; i < path.length(); i++ ) {
	if( path[i] != ' ' )
	  _result.append( path[i] );
      }
    }
    else if( whiteSpaceTreatment() == K3bDataDoc::extendedStrip ) {
      // if QString is saved as an array this code is OK
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
    KStringListDialog d( m_notFoundFiles, i18n("Not found"), i18n("Could not find the following files:"), 
			 true, k3bMain(), "notFoundFilesInfoDialog" );
    d.exec();

    m_notFoundFiles.clear();
  }


  // mkisofs seems to have a bug that prevents us to use filenames 
  // that contain one or more backslashes
  // -----------------------------------------------------------------------
  if( !m_mkisofsBuggyFiles.isEmpty() ) {
    KStringListDialog d( m_mkisofsBuggyFiles, i18n("Sorry"), i18n("Due to a bug in mkisofs K3b is not able to handle "
								  "filenames that contain one or more backslashes:"), 
			 true, k3bMain() );
    d.exec();

    m_mkisofsBuggyFiles.clear();
  }
  // -----------------------------------------------------------------------
}


void K3bDataDoc::loadDefaultSettings()
{
  KConfig* c = k3bMain()->config();

  c->setGroup( "default data settings" );

  setDummy( c->readBoolEntry( "dummy_mode", false ) );
  setDao( c->readBoolEntry( "dao", true ) );
  setOnTheFly( c->readBoolEntry( "on_the_fly", true ) );
  setBurnproof( c->readBoolEntry( "burnproof", true ) );

  m_createRockRidge = c->readBoolEntry( "rock_ridge", true );
  m_createJoliet = c->readBoolEntry( "joliet", false );
  m_deleteImage = c->readBoolEntry( "remove_image", true );
  m_onlyCreateImage = c->readBoolEntry( "only_create_image", false );
  m_isoLevel = c->readNumEntry( "iso_level", 1 );

  QString w = c->readEntry( "white_space_treatment", "normal" );
  if( w == "convert" )
    m_whiteSpaceTreatment = K3bDataDoc::convertToUnderScore;
  else if( w == "strip" )
    m_whiteSpaceTreatment = K3bDataDoc::strip;
  else if( w == "extended" )
    m_whiteSpaceTreatment = K3bDataDoc::extendedStrip;
  else
    m_whiteSpaceTreatment = K3bDataDoc::normal;
}


#include "k3bdatadoc.moc"
