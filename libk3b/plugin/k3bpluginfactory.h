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


#ifndef _K3B_PLUGIN_FACTORY_H_
#define _K3B_PLUGIN_FACTORY_H_

#include <klibloader.h>
#include <kinstance.h>
#include <kglobal.h>
#include <klocale.h>
//Added by qt3to4:
#include <Q3CString>

/**
 * Template based on KGenericFactory. This is just here to avoid using the QStringList args parameter
 * in every plugin's constructor.
 *
 * Use this as follows:
 * K_EXPORT_COMPONENT_FACTORY( libk3bartsaudioserver, K3bPluginFactory<K3bArtsAudioServer>( "k3bartsaudioserver" ) )
 *
 * See KGenericFactory for more information.
 */
template <class T>
class K3bPluginFactory : public KLibFactory
{
 public:
  K3bPluginFactory( const char* instanceName )
    : m_instanceName(instanceName) {
    s_self = this;
    m_catalogueInitialized = false;
  }

  ~K3bPluginFactory() {
    if ( s_instance )
      KGlobal::locale()->removeCatalog( s_instance->instanceName() );
    delete s_instance;
    s_instance = 0;
    s_self = 0;
  }

  static KInstance* instance();

 protected:
  virtual void setupTranslations( void ) {
    if( instance() )
      KGlobal::locale()->insertCatalog( instance()->instanceName() );
  }

  void initializeMessageCatalogue() {
    if( !m_catalogueInitialized ) {
      m_catalogueInitialized = true;
      setupTranslations();
    }
  }

  virtual QObject* createObject( QObject *parent, const char *name,
				 const char*, const QStringList& ) {
    initializeMessageCatalogue();
    return new T( parent, name );
  }

 private:
  Q3CString m_instanceName;
  bool m_catalogueInitialized;

  static KInstance* s_instance;
  static K3bPluginFactory<T> *s_self;
};


template <class T>
KInstance* K3bPluginFactory<T>::s_instance = 0;


template <class T>
K3bPluginFactory<T>* K3bPluginFactory<T>::s_self = 0;


template <class T>
KInstance* K3bPluginFactory<T>::instance()
{
  if( !s_instance && s_self )
    s_instance = new KInstance( s_self->m_instanceName );
  return s_instance;
}

#endif
