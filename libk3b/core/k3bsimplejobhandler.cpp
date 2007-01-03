/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bsimplejobhandler.h"


K3bSimpleJobHandler::K3bSimpleJobHandler( QObject* parent )
  : QObject( parent ),
    K3bJobHandler()
{
}

K3bSimpleJobHandler::~K3bSimpleJobHandler()
{
}

int K3bSimpleJobHandler::waitForMedia( K3bDevice::Device* dev,
				       int mediaState,
				       int mediaType,
				       const QString& message )
{
  Q_UNUSED( dev );
  Q_UNUSED( mediaState );
  Q_UNUSED( mediaType );
  Q_UNUSED( message );

  return 0;
}

bool K3bSimpleJobHandler::questionYesNo( const QString& text,
					 const QString& caption,
					 const QString& yesText,
					 const QString& noText )
{
  Q_UNUSED( text );
  Q_UNUSED( caption );
  Q_UNUSED( yesText );
  Q_UNUSED( noText );

  return true;
}

void K3bSimpleJobHandler::blockingInformation( const QString& text,
					       const QString& caption )
{
  Q_UNUSED( text );
  Q_UNUSED( caption );
}

#include "k3bsimplejobhandler.moc"
