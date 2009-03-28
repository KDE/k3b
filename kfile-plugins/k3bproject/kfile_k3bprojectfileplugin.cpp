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


#include <config-k3b.h>

#include "kfile_k3bprojectfileplugin.h"
#include "kostore/koStore.h"
#include "kostore/koStoreDevice.h"

#include <k3bdoc.h>

#include <kgenericfactory.h>

#include <q3textstream.h>
#include <qdom.h>
#include <qfile.h>



K_EXPORT_COMPONENT_FACTORY(kfile_k3b, KGenericFactory<K3b::ProjectFilePlugin>("kfile_k3b"))


K3b::ProjectFilePlugin::K3b::ProjectFilePlugin( QObject *parent, const char *name,
					    const QStringList &args)
  : KFilePlugin(parent, name, args)
{
  KFileMimeTypeInfo* info = addMimeTypeInfo( "application/x-k3b" );

  KFileMimeTypeInfo::GroupInfo* group = addGroupInfo( info, "General", i18n("General") );

  addItemInfo( group, "documenttype", i18n("Document Type"), QVariant::String );
}


bool K3b::ProjectFilePlugin::readInfo( KFileMetaInfo& info, uint /*what*/)
{
  if( !info.url().isLocalFile() ) {
    kDebug() << "(K3b::ProjectFilePluginInfo) no local file.";
    return false;
  }

  // open the file
  bool success = false;
  QDomDocument xmlDoc;

  // try opening a store
  KoStore* store = KoStore::createStore( info.url().toLocalFile(), KoStore::Read );
  if( store && !store->bad() && store->open( "maindata.xml" ) ) {
    QIODevice* dev = store->device();
    dev->open( QIODevice::ReadOnly );
    if( xmlDoc.setContent( dev ) )
      success = true;
    dev->close();
    store->close();
  }
  else
    kDebug() << "(K3b::ProjectFilePluginInfo) failed to open the store.";

  if( success ) {
    // check the documents DOCTYPE
    K3b::Doc::DocType type = K3b::Doc::AUDIO;
    if( xmlDoc.doctype().name() == "k3b_audio_project" )
      type = K3b::Doc::AUDIO;
    else if( xmlDoc.doctype().name() == "k3b_data_project" )
      type = K3b::Doc::DATA;
    else if( xmlDoc.doctype().name() == "k3b_vcd_project" )
      type = K3b::Doc::VCD;
    else if( xmlDoc.doctype().name() == "k3b_mixed_project" )
      type = K3b::Doc::MIXED;
    else if( xmlDoc.doctype().name() == "k3b_movix_project" )
      type = K3b::Doc::MOVIX;
    else if( xmlDoc.doctype().name() == "k3b_movixdvd_project" )
        type = K3b::Doc::MOVIX;
    else if( xmlDoc.doctype().name() == "k3b_dvd_project" )
      type = K3b::Doc::DATA;
    else if( xmlDoc.doctype().name() == "k3b_video_dvd_project" )
      type = K3b::Doc::VIDEODVD;
    else {
      kDebug() << "(K3b::Doc) unknown doc type: " << xmlDoc.doctype().name();
      success = false;
    }

    QString stringType;
    switch( type ) {
    case K3b::Doc::AUDIO:
      stringType = i18n("Audio CD project");
      break;
    case K3b::Doc::DATA:
      stringType = i18n("Data project");
      break;
    case K3b::Doc::MIXED:
      stringType = i18n("Mixed Mode CD project");
      break;
    case K3b::Doc::VCD:
      stringType = i18n("Video CD project");
      break;
    case K3b::Doc::MOVIX:
      stringType = i18n("eMovix project");
      break;
    case K3b::Doc::VIDEODVD:
      stringType = i18n("Video DVD project");
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

