/*
 *
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config-k3b.h>

#include "k3b<name>encoder.h"

#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>

K3B_EXPORT_PLUGIN_CONFIG_WIDGET( kcm_<name>, K3b<name>EncoderConfigWidget )

K3b<name>Encoder::K3b<name>Encoder( QObject* parent, const QVariantList& )
    : K3b::AudioEncoder( parent )
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


void K3b<name>Encoder::setMetaDataInternal( K3b::AudioEncoder::MetaDataField f, const QString& value )
{
    // PUT YOUR CODE HERE
}


QStringList K3b<name>Encoder::extensions() const
{
    // PUT YOUR CODE HERE
    return QStringList( "" );
}


QString K3b<name>Encoder::fileTypeComment( const QString& ) const
{
    // PUT YOUR CODE HERE
    return "";
}


long long K3b<name>Encoder::fileSize( const QString&, const K3b::Msf& msf ) const
{
    // PUT YOUR CODE HERE
    return -1;
}



K3b<name>EncoderConfigWidget::K3b<name>EncoderConfigWidget( QWidget* parent )
    : K3b::PluginConfigWidget( parent )
{
}


K3b<name>EncoderConfigWidget::~K3b<name>EncoderConfigWidget()
{
}


void K3b<name>EncoderConfigWidget::load()
{
    KSharedConfig::Ptr c = KGlobal::config();
    c->setGroup( "K3b<name>EncoderPlugin" );

    // PUT YOUR CODE HERE
}


void K3b<name>EncoderConfigWidget::save()
{
    KSharedConfig::Ptr c = KGlobal::config();
    c->setGroup( "K3b<name>EncoderPlugin" );

    // PUT YOUR CODE HERE
}


void K3b<name>EncoderConfigWidget::defaults()
{
    // PUT YOUR CODE HERE
}


#include "k3b<name>encoder.moc"
