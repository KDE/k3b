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

#include <qdir.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qtimer.h>
#include <qdom.h>

#include <kstddirs.h>
#include <kurl.h>
#include <kstatusbar.h>
#include <klocale.h>

#include <id3/tag.h>
#include <id3/misc_support.h>


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
  m_isoImage = QString::null;

  m_createRockRidge = true;
  m_createJoliet = false;
  m_deleteImage = true;
  m_onlyCreateImage = false;
  m_followSymbolicLinks = false;
  m_isoLevel = 1;
  m_whiteSpaceTreatment = K3bDataDoc::normal;;
	
  return K3bDoc::newDocument();
}


K3bView* K3bDataDoc::newView( QWidget* parent )
{
  return new K3bDataView( this, parent );
}


void K3bDataDoc::addView(K3bView* view)
{
  K3bDataView* v = (K3bDataView*)view;
  connect( v, SIGNAL(dropped(const QStringList&, K3bDirItem*)), this, SLOT(slotAddURLs(const QStringList&, K3bDirItem*)) );
	
  K3bDoc::addView( view );
}

void K3bDataDoc::slotAddURLs( const QStringList& urls, K3bDirItem* dirItem )
{
  if( !dirItem )
    dirItem = m_root;

  for( QStringList::ConstIterator it = urls.begin(); it != urls.end(); ++it ) 
    {
      KURL kurl( *it );
      m_queuedToAddItems.enqueue( new PrivateItemToAdd(kurl.path(), dirItem ) );
    }

  m_queuedToAddItemsTimer->start(0);
  k3bMain()->statusBar()->changeItem( i18n( "Adding files to Project %1..." ).arg( volumeID() ), 1 );
}


void K3bDataDoc::slotAddQueuedItems()
{
  PrivateItemToAdd* item = m_queuedToAddItems.dequeue();
  if( item )
    {
      bool add = true;
      QList<K3bDataItem>* itemsInDir = item->parent->children();
      for( K3bDataItem* it = itemsInDir->first(); it; it = itemsInDir->next() ) {
	if( it->k3bName() == item->fileInfo.fileName() ) {
	  qDebug( "(K3bDataItem) already a file with that name in directory: " + it->k3bName() );
	  add = false;
	  break;
	  //	      emit infoMessage( "There was already a file with that name in directory: " + it->k3bName() );
	}
      }

      if( add )
	{
	  if( item->fileInfo.isDir() )
	    {
	      K3bDirItem* newDirItem = new K3bDirItem( item->fileInfo.fileName(), this, item->parent );
	      //	      emit newDir( newDirItem );
	      
	      QStringList dlist = QDir( item->fileInfo.absFilePath() ).entryList();
	      dlist.remove(".");
	      dlist.remove("..");
	      
	      for( QStringList::Iterator it = dlist.begin(); it != dlist.end(); ++it ) {
		m_queuedToAddItems.enqueue( new PrivateItemToAdd( item->fileInfo.absFilePath() + "/" + *it, 
								 newDirItem ) );
	      }
	    }
	  else
	    {
	      K3bFileItem* newK3bItem = new K3bFileItem( item->fileInfo.absFilePath(), this, item->parent );
	      m_size += newK3bItem->k3bSize();

	      if( k3bMain()->useID3TagForMp3Renaming() && newK3bItem->mimetype() == "audio/x-mp3" ) {
		ID3_Tag tag( item->fileInfo.absFilePath().latin1() );
		
		ID3_Frame* frame = tag.Find( ID3FID_TITLE );
		QString title(ID3_GetString(frame, ID3FN_TEXT ));
		
		frame = tag.Find( ID3FID_LEADARTIST );
		QString artist(ID3_GetString(frame, ID3FN_TEXT ));
		
		if( !title.isEmpty() && !artist.isEmpty() ) {
		  //	      qDebug( "(K3bDataDoc) setting k3bname to: \"" + artist + " - " + title + ".mp3\"" );
		  newK3bItem->setK3bName( artist + " - " + title + ".mp3" );
		}
	      }
	    }
	}

      delete item;
    }
  else
    {
      m_queuedToAddItemsTimer->stop();
      emit newFileItems();
      k3bMain()->statusBar()->changeItem( i18n("Ready"), 1 );
    }
}


K3bDirItem* K3bDataDoc::addEmptyDir( const QString& name, K3bDirItem* parent )
{
  K3bDirItem* item = new K3bDirItem( name, this, parent );

  emit newFileItems();

  return item;
}


long K3bDataDoc::size() const
{
//   K3bDataItem* item = root();
//   long _size = 0;
	
//   while( item ) {
//     _size+=item->k3bSize();
		
//     item = item->nextSibling();
//   }

  return m_size;	
  //  return root()->k3bSize();
}


int K3bDataDoc::length() const
{
  // 1 block consists of 2048 bytes real data
  // and 1 block are needed to store 1 audio frame
  // so this is the way to calculate:

  return size() / 2048;
}


QString K3bDataDoc::documentType() const
{
  return "k3b_data_project";
}


bool K3bDataDoc::loadDocumentData( QDomDocument* )
{
  // TODO: so what? load the shit! ;-)
  return true;
}


bool K3bDataDoc::saveDocumentData( QDomDocument* )
{
  // TODO: some saving work...
  return true;
}


void K3bDataDoc::removeItem( K3bDataItem* item )
{
  qDebug( "(K3bDataDoc) remove item " + item->k3bName() );
	
  if( item == root() )
    qDebug( "(K3bDataDoc) tried to remove root-entry!");
  else {
    emit itemRemoved( item );
	
    m_size -= item->k3bSize();
    if( m_size < 0 ) {
      qDebug( "(K3bDataDoc) Size of project is: %i, that CANNOT be! Will exit", m_size );
      exit(0);
    }

    // the item takes care about it's parent!
    qDebug( "(K3bDataDoc) now it should be save to delete the item!");
    delete item;
    qDebug( "(K3bDataDoc) now the item has been deleted!");
  }
}


QString K3bDataDoc::writePathSpec( const QString& filename )
{
  QFile file( filename );
  if( !file.open( IO_WriteOnly ) ) {
    qDebug( "(K3bDataDoc) Could not open path-spec-file %s", filename.latin1() );
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
		
    qDebug( "(K3bDataDoc) converted " + path + " to " + _result );
    return _result;
  }
  else
    return path;
}
