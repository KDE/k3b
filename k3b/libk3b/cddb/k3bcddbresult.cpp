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



#include "k3bcddbresult.h"


K3bCddbResult::K3bCddbResult()
{
}


void K3bCddbResult::clear()
{
  m_entries.clear();
}


int K3bCddbResult::foundEntries() const
{
  return m_entries.count();
}

const K3bCddbResultEntry& K3bCddbResult::entry( unsigned int number ) const
{
  if( number >= m_entries.count() )
    return m_emptyEntry;

  return m_entries[number];
}


void K3bCddbResult::addEntry( const K3bCddbResultEntry& entry )
{
  m_entries.append( entry );
}
