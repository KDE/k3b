/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3b<name>encoder.h"

#include <k3bcore.h>

#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kinstance.h>

#include <qlayout.h>
#include <qcstring.h>





K3b<name>Encoder::K3b<name>Encoder( QObject* parent, const char* name )
  : K3bAudioEncoder( parent, name )
{
}


K3b<name>Encoder::~K3b<name>Encoder()
{
}


bool K3b<name>Encoder::initEncoderInternal( const QString& )
{
  // PUT YOUR CODE HERE
  return false;
}


long K3b<name>Encoder::encodeInternal( const char* data, Q_ULONG len )
{
  // PUT YOUR CODE HERE
  return false;
}


void K3b<name>Encoder::finishEncoderInternal()
{
  // PUT YOUR CODE HERE
}


void K3b<name>Encoder::setMetaDataInternal( K3bAudioEncoder::MetaDataField f, const QString& value )
{
  // PUT YOUR CODE HERE
}





K3b<name>EncoderSettingsWidget::K3b<name>EncoderSettingsWidget( QWidget* parent, const char* name )
  : K3bPluginConfigWidget( parent, name )
{
}


K3b<name>EncoderSettingsWidget::~K3b<name>EncoderSettingsWidget()
{
}


void K3b<name>EncoderSettingsWidget::loadConfig()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "K3b<name>EncoderPlugin" );

  // PUT YOUR CODE HERE
}


void K3b<name>EncoderSettingsWidget::saveConfig()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "K3b<name>EncoderPlugin" );

  // PUT YOUR CODE HERE
}



K3b<name>EncoderFactory::K3b<name>EncoderFactory( QObject* parent, const char* name )
  : K3bAudioEncoderFactory( parent, name )
{
  s_instance = new KInstance( "k3b<name>sencoder" );
}


K3b<name>EncoderFactory::~K3b<name>EncoderFactory()
{
}


QStringList K3b<name>EncoderFactory::extensions() const
{
  // PUT YOUR CODE HERE
  return QStringList( "" );
}


QString K3b<name>EncoderFactory::fileTypeComment( const QString& ) const
{
  // PUT YOUR CODE HERE
  return "";
}


long long K3b<name>EncoderFactory::fileSize( const QString&, const K3b::Msf& msf ) const
{
  // PUT YOUR CODE HERE
  return -1;
}


K3bPlugin* K3b<name>EncoderFactory::createPluginObject( QObject* parent, 
							   const char* name,
							   const QStringList& )
{
  return new K3b<name>Encoder( parent, name );
}


K3bPluginConfigWidget* K3b<name>EncoderFactory::createConfigWidgetObject( QWidget* parent, 
									     const char* name,
									     const QStringList& )
{
  return new K3b<name>EncoderSettingsWidget( parent, name );
}


#include "k3b<name>encoder.moc"
