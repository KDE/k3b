/* 
 *
 * $Id: $
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


#include "k3bmovixdoc.h"
#include "k3bmovixview.h"
#include "k3bmovixjob.h"
#include "k3bmovixfileitem.h"

#include <data/k3bdiritem.h>
#include <data/k3bfileitem.h>

#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <klineeditdlg.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kapplication.h>

#include <qdom.h>
#include <qfileinfo.h>


K3bMovixDoc::K3bMovixDoc( QObject* parent )
  : K3bDataDoc( parent )
{
  connect( this, SIGNAL(itemRemoved(K3bDataItem*)),
	   this, SLOT(slotDataItemRemoved(K3bDataItem*)) );
}


K3bMovixDoc::~K3bMovixDoc()
{
}


K3bView* K3bMovixDoc::newView( QWidget* parent )
{
  return new K3bMovixView( this, parent );
}


K3bBurnJob* K3bMovixDoc::newBurnJob()
{
  return new K3bMovixJob(this);
}


bool K3bMovixDoc::newDocument()
{
  return K3bDataDoc::newDocument();
}


void K3bMovixDoc::addUrls( const KURL::List& urls )
{
  for( KURL::List::ConstIterator it = urls.begin(); it != urls.end(); ++it ) {
    addMovixFile( *it );
  }

  emit newMovixFileItems();
  emit newFileItems();
}


void K3bMovixDoc::addMovixFile( const KURL& url, int pos )
{
  QFileInfo f( url.path() );
  if( !f.isFile() || !url.isLocalFile() )
    return;

  QString newName = f.fileName();
  if( nameAlreadyInDir( newName, root() ) ) {
    kapp->config()->setGroup("Data project settings");
    bool dropDoubles = kapp->config()->readBoolEntry( "Drop doubles", false );
    if( dropDoubles )
      return;
    
    bool ok = true;
    do {
      newName = KLineEditDlg::getText( i18n("A file with that name already exists. Please enter a new name."), 
				       newName, &ok, 0 );
    } while( ok && nameAlreadyInDir( newName, root() ) );
    
    if( !ok )
      return;
  }

  K3bMovixFileItem* newK3bItem = new K3bMovixFileItem( f.absFilePath(), this, root(), newName );
  if( pos < 0 || pos > (int)m_movixFiles.count() )
    pos = m_movixFiles.count();

  m_movixFiles.insert( pos, newK3bItem );

  emit newMovixFileItems();
  emit newFileItems();

  setModified(true);
}


bool K3bMovixDoc::loadDocumentData( QDomElement* root )
{
  // FIXME
  return false;
}


bool K3bMovixDoc::saveDocumentData( QDomElement* )
{
  // FIXME
  return false;
}


void K3bMovixDoc::slotDataItemRemoved( K3bDataItem* item )
{
  // check if it's a movix item
  if( K3bMovixFileItem* fi = dynamic_cast<K3bMovixFileItem*>(item) )
    if( m_movixFiles.containsRef( fi ) ) {
      emit movixItemRemoved( fi );
      m_movixFiles.removeRef( fi );
      setModified(true);
    }
}


int K3bMovixDoc::indexOf( K3bMovixFileItem* item )
{
  return m_movixFiles.findRef(item)+1;
}


void K3bMovixDoc::moveMovixItem( K3bMovixFileItem* item, K3bMovixFileItem* itemAfter )
{
  if( item == itemAfter )
    return;

  // set the current item to track
  m_movixFiles.findRef( item );
  // take the current item
  item = m_movixFiles.take();

  // if after == 0 findRef returnes -1
  int pos = m_movixFiles.findRef( itemAfter );
  m_movixFiles.insert( pos+1, item );

  emit newMovixFileItems();
}


void K3bMovixDoc::addSubTitleItem( K3bMovixFileItem* item, const KURL& url )
{
  if( item->subTitleItem() )
    removeSubTitleItem( item );

  QFileInfo f( url.path() );
  if( !f.isFile() || !url.isLocalFile() )
    return;

  // check if there already is a file named like we want to name the subTitle file
  QString name = K3bMovixFileItem::subTitleFileName( item->k3bName() );

  if( nameAlreadyInDir( name, root() ) ) {
    KMessageBox::error( 0, i18n("Could not rename subtitle file. File with requested name %1 already exists.").arg(name) );
    return;
  }

  K3bFileItem* subItem = new K3bFileItem( f.absFilePath(), this, root(), name );
  item->setSubTitleItem( subItem );

  emit newMovixFileItems();
  emit newFileItems();

  setModified(true);
}


void K3bMovixDoc::removeSubTitleItem( K3bMovixFileItem* item )
{
  if( item->subTitleItem() ) {
    emit subTitleItemRemoved( item );

    delete item->subTitleItem();
    item->setSubTitleItem(0);

    setModified(true);
  }
}

#include "k3bmovixdoc.moc"
