/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config.h>

#if HAVE_MUSICBRAINZ

#include "k3btrm.h"

#include <kdebug.h>


K3bTRMLookup::K3bTRMLookup( const QString& file, QObject* parent, const char* name )
  : QObject( parent, name ),
    KTRMLookup( file )
{
}


K3bTRMLookup::~K3bTRMLookup()
{
}


void K3bTRMLookup::recognized()
{
  m_resultState = RECOGNIZED;
  KTRMLookup::recognized();
}


void K3bTRMLookup::unrecognized()
{
  m_resultState = UNRECOGNIZED;
  KTRMLookup::unrecognized();
}


void K3bTRMLookup::collision()
{
  m_resultState = COLLISION;
  KTRMLookup::collision();
}


void K3bTRMLookup::error()
{
  m_resultState = ERROR;
  KTRMLookup::error();
}


void K3bTRMLookup::finished()
{
  emit lookupFinished( this );
  KTRMLookup::finished();
}

#include "k3btrm.moc"

#endif
