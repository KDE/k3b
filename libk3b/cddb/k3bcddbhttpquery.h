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


#ifndef K3BCDDB_HTTP_QUERY_H
#define K3BCDDB_HTTP_QUERY_H

#include "k3bcddbquery.h"
#include "k3bcddbresult.h"

#include <qvaluelist.h>

namespace KIO {
  class Job;
}

class K3bCddbHttpQuery : public K3bCddbQuery
{
  Q_OBJECT

 public:
  K3bCddbHttpQuery( QObject* parent = 0, const char* name = 0 );
  ~K3bCddbHttpQuery();

 public slots:
  void setServer( const QString& s, int port = 80 ) { m_server = s; m_port = port; }
  void setCgiPath( const QString& p ) { m_cgiPath = p; }

 protected slots:
  void doQuery();
  void doMatchQuery();
  void slotResult( KIO::Job* );
  void slotData( KIO::Job*, const QByteArray& data );

 private:
  void performCommand( const QString& );

  enum State { QUERY, QUERY_DATA, READ, READ_DATA, FINISHED };

  int m_state;
  QString m_server;
  int m_port;
  QString m_cgiPath;

  QString m_currentlyConnectingServer;

  QString m_data;
  QString m_parsingBuffer;
};

#endif

