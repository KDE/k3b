/***************************************************************************
                          k3bcddbresult.h  -  description
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


#ifndef K3B_CDDB_RESULT_H
#define K3B_CDDB_RESULT_H


#include <qstringlist.h>


class K3bCddbResultEntry
{
 public:
  QStringList titles;
  QStringList artists;
  QStringList extInfos;

  QString cdTitle;
  QString cdArtist;
  QString cdExtInfo;

  QString genre;
  QString category;
  QString discid;

  QString rawData;
};


class K3bCddbResult
{
 public:
  K3bCddbResult();
  //  K3bCddbQuery( const K3bCddbQuery& );

  void clear();
  void addEntry( const K3bCddbResultEntry& = K3bCddbResultEntry() );
  const K3bCddbResultEntry& entry( unsigned int number = 0 ) const;
  int foundEntries() const;

 private:
  QValueList<K3bCddbResultEntry> m_entries;

  K3bCddbResultEntry m_emptyEntry;
};

#endif
