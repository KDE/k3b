/* 
 *
 * $Id: sourceheader,v 1.3 2005/01/19 13:03:46 trueg Exp $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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


#include <config.h>

#include "kfile_k3bprojectfileplugin.h"
#include "kostore/koStore.h"
#include "kostore/koStoreDevice.h"

#include <k3bdoc.h>

#include <kgenericfactory.h>

#include <qtextstream.h>
#include <qdom.h>
#include <qfile.h>



K_EXPORT_COMPONENT_FACTORY(kfile_k3b, KGenericFactory<K3bProjectFilePlugin>("kfile_k3b"))


K3bProjectFilePlugin::K3bProjectFilePlugin( QObject *parent, const char *name,
					    const QStringList &args)
  : KFilePlugin(parent, name, args)
{
  KFileMimeTypeInfo* info = addMimeTypeInfo( "application/x-k3b" );

  KFileMimeTypeInfo::GroupInfo* group = addGroupInfo( info, "General", i18n("General") );

  addItemInfo( group, "documenttype", i18n("Document Type"), QVariant::String );
}


bool K3bProjectFilePlugin::readInfo( KFileMetaInfo& info, uint /*what*/)
{
  if( !info.url().isLocalFile() ) {
    kdDebug() << "(K3bProjectFilePluginInfo) no local file." << endl;
    return false;
  }

  // open the file
  bool success = false;
  QDomDocument xmlDoc;

  // try opening a store
  KoStore* store = KoStore::createStore( info.url().path(), KoStore::Read );
  if( store && !store->bad() && store->open( "maindata.xml" ) ) {
    QIODevice* dev = store->device();
    dev->open( IO_ReadOnly );
    if( xmlDoc.setContent( dev ) )
      success = true;
    dev->close();
    store->close();
  }
  else
    kdDebug() << "(K3bProjectFilePluginInfo) failed to open the store." << endl;

  if( success ) {
    // check the documents DOCTYPE
    K3bDoc::DocType type = K3bDoc::AUDIO;
    if( xmlDoc.doctype().name() == "k3b_audio_project" )
      type = K3bDoc::AUDIO;
    else if( xmlDoc.doctype().name() == "k3b_data_project" )
      type = K3bDoc::DATA;
    else if( xmlDoc.doctype().name() == "k3b_vcd_project" )
      type = K3bDoc::VCD;
    else if( xmlDoc.doctype().name() == "k3b_mixed_project" )
      type = K3bDoc::MIXED;
    else if( xmlDoc.doctype().name() == "k3b_movix_project" )
      type = K3bDoc::MOVIX;
    else if( xmlDoc.doctype().name() == "k3b_movixdvd_project" )
      type = K3bDoc::MOVIX_DVD;
    else if( xmlDoc.doctype().name() == "k3b_dvd_project" )
      type = K3bDoc::DVD;
    else if( xmlDoc.doctype().name() == "k3b_video_dvd_project" )
      type = K3bDoc::VIDEODVD;
    else {
      kdDebug() << "(K3bDoc) unknown doc type: " << xmlDoc.doctype().name() << endl;
      success = false;
    }

    QString stringType;
    switch( type ) {
    case K3bDoc::AUDIO:
      stringType = i18n("Audio CD");
      break;
    case K3bDoc::DATA:
      stringType = i18n("Data CD");
      break;
    case K3bDoc::MIXED:
      stringType = i18n("Mixed Mode CD");
      break;
    case K3bDoc::VCD:
      stringType = i18n("Video CD");
      break;
    case K3bDoc::MOVIX:
      stringType = i18n("eMovix CD");
      break;
    case K3bDoc::MOVIX_DVD:
      stringType = i18n("eMovix DVD");
      break;
    case K3bDoc::DVD:
      stringType = i18n("Data DVD");
      break;
    case K3bDoc::VIDEODVD:
      stringType = i18n("Video DVD");
      break;
    }

    // and finally display it!
    KFileMetaInfoGroup group = appendGroup(info, "General");
    appendItem( group, "documenttype", stringType );
  }

  delete store;

  return success;
}

#include "kfile_k3bprojectfileplugin.moc"

