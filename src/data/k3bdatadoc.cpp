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

#include <kstddirs.h>
#include <kurl.h>

#include <id3/tag.h>
#include <id3/misc_support.h>


K3bDataDoc::K3bDataDoc( QObject* parent )
  : K3bDoc( parent )
{
  m_docType = DATA;
  m_root = 0;
	
			
  //	connect( this, SIGNAL(signalAddDirectory(const QString&, K3bDirItem*)),
  //					this, SLOT(slotAddDirectory( const QString&, K3bDirItem*)) );
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
  connect( this, SIGNAL(newDir(K3bDirItem*)), v, SLOT(slotAddDir(K3bDirItem*)) );
  connect( this, SIGNAL(newFile(K3bFileItem*)), v, SLOT(slotAddFile(K3bFileItem*)) );
  connect( this, SIGNAL(itemRemoved(K3bDataItem*)), v, SLOT(slotItemRemoved(K3bDataItem*)) );
	
  K3bDoc::addView( view );
}

void K3bDataDoc::slotAddURLs( const QStringList& urls, K3bDirItem* dirItem )
{
  if( !dirItem )
    dirItem = m_root;

  qDebug( "(K3bDataDoc) adding urls to %s", dirItem->k3bName().latin1() );
				
  for( QStringList::ConstIterator _it = urls.begin(); _it != urls.end(); ++_it ) {
    // test if url directory or file
    KURL _kurl( *_it );
    if( QFileInfo( _kurl.path() ).isDir() ) {
      qDebug("       -dir-");
      slotAddDirectory( _kurl.path(), dirItem );
    }
    else {
      addNewFile( _kurl.path(), dirItem );
    }
  }
}


void K3bDataDoc::slotAddDirectory( const QString& url, K3bDirItem* parent )
{
  qDebug( "(K3bDataDoc) slotAddDirectory: %s", url.latin1() );

  QFileInfo _info( url );
  if( !_info.isDir() ) {
    qDebug( "(K3bDataDoc) tried to add url %s as directory which is no directory!", url.latin1() );
    return;
  }
	
	
  QList<K3bDataItem>* _itemsInDir = parent->children();
  for( K3bDataItem* _it = _itemsInDir->first(); _it; _it = _itemsInDir->next() ) {
    if( _it->k3bName() == QDir(url).dirName() ) {
      qDebug( "(K3bDataItem) already a file with that name in directory: " + _it->k3bName() );
      emit infoMessage( "There was already a file with that name in directory: " + _it->k3bName() );
      return;
    }
  }

  K3bDirItem* _newDirItem = new K3bDirItem( QDir(url).dirName(), this, parent );
  emit newDir( _newDirItem );
	
  QDir _d( url );
  QStringList _dlist = _d.entryList();
  _dlist.remove(".");
  _dlist.remove("..");
	
  for( QStringList::Iterator _it = _dlist.begin(); _it != _dlist.end(); ++_it ) {
    if( QFileInfo( _d.absPath() + "/" + *_it ).isDir() )
      slotAddDirectory(  _d.absPath() + "/" + *_it, _newDirItem );
    else
      addNewFile( _d.absPath() + "/" + *_it, _newDirItem );
  }
}



void K3bDataDoc::addNewFile( const QString& path, K3bDirItem* dir )
{
  // TODO: erst filename erstellen (id3tag), dann mithilfe von K3bDirItem::alreadyInDir testen, dann eventuelles umbenennen anbieten
  //             und dann ein neues item anlegen

  if( !dir)
    dir = m_root;

  K3bFileItem* _item =	new K3bFileItem( path, this, dir );
  qDebug( "---- adding file " + _item->url().path() );
	
  if( k3bMain()->useID3TagForMp3Renaming() && _item->mimetype() == "audio/x-mp3" ) {
    ID3_Tag _tag( _item->url().path() );
		
    ID3_Frame* _frame = _tag.Find( ID3FID_TITLE );
    QString _title(ID3_GetString(_frame, ID3FN_TEXT ));
		
    _frame = _tag.Find( ID3FID_LEADARTIST );
    QString _artist(ID3_GetString(_frame, ID3FN_TEXT ));
		
    if( !_title.isEmpty() && !_artist.isEmpty() ) {
      qDebug( "(K3bDataDoc) setting k3bname to: \"" + _artist + " - " + _title + ".mp3\"" );
      _item->setK3bName( _artist + " - " + _title + ".mp3" );
    }
  }

  // test if already in directory
  QList<K3bDataItem>* _itemsInDir = dir->children();
  for( K3bDataItem* _it = _itemsInDir->first(); _it; _it = _itemsInDir->next() ) {
    if( _it != _item && _it->k3bName() == KURL( path ).fileName() ) {
      qDebug( "(K3bDataDoc) already a file with that name in directory: " + _it->k3bName() );
      delete _item;
      return;
    }
  }
	
  emit newFile( _item );
}


int K3bDataDoc::size() const
{
  K3bDataItem* item = root();
  long _size = 0;
	
  while( item ) {
    _size+=item->k3bSize();
		
    item = item->nextSibling();
  }
	
  return (int)(_size);
}


int K3bDataDoc::length() const
{
  // 1 block consists of 2048 bytes real data
  // and 1 block are needed to store 1 audio frame
  // so this is the way to calculate:

  return size() / 2048;
}


bool K3bDataDoc::loadDocumentData( QFile& )
{
  // TODO: so what? load the shit! ;-)
  return true;
}


bool K3bDataDoc::saveDocumentData( QFile& )
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
