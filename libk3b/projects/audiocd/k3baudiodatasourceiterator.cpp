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

#include "k3baudiodatasourceiterator.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3baudiodatasource.h"


K3bAudioDataSourceIterator::K3bAudioDataSourceIterator( K3bAudioDoc* doc )
  : m_doc( doc )
{
  first();
}


K3bAudioDataSource* K3bAudioDataSourceIterator::current() const
{
  return m_currentSource;
}


K3bAudioDataSource* K3bAudioDataSourceIterator::next()
{
  m_currentSource = m_currentSource->next();
  if( !m_currentSource ) {
    m_currentTrack = m_currentTrack->next();
    if( m_currentTrack )
      m_currentSource = m_currentTrack->firstSource();
  }

  return m_currentSource;
}


bool K3bAudioDataSourceIterator::hasNext() const
{
  if( !m_currentSource )
    return false;
  if( m_currentSource->next() )
    return true;
  if( m_currentTrack->next() )
    return true;

  return false;
}


K3bAudioDataSource* K3bAudioDataSourceIterator::first()
{
  m_currentTrack = m_doc->firstTrack();

  if( m_currentTrack )
    m_currentSource = m_currentTrack->firstSource();
  else
    m_currentSource = 0;

  return m_currentSource;
}
