/***************************************************************************
                          k3bcddbresult.cpp  -  description
                             -------------------
    begin                : Sun Oct 7 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


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
