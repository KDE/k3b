/***************************************************************************
                          k3bcddblocalquery.h  -  description
                             -------------------
    begin                : Mon Nov 4 2002
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


#ifndef K3BCDDB_LOCAL_QUERY_H
#define K3BCDDB_LOCAL_QUERY_H

#include "k3bcddbquery.h"
#include "k3bcddbresult.h"

#include <qstring.h>
#include <qvaluelist.h>


class K3bCddbLocalQuery : public K3bCddbQuery
{
  Q_OBJECT

 public:
  K3bCddbLocalQuery( QObject* parent = 0, const char* name = 0 );
  ~K3bCddbLocalQuery();

 public slots:
  void setCddbDir( const QString& dir ) { m_cddbDir = dir; }

 protected:
  void doQuery();

 private:
  QString m_cddbDir;
  QValueList<K3bCddbResultEntry> m_matches;
};

#endif
