/***************************************************************************
                          k3bcddbpquery.h  -  description
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


#ifndef K3BCDDBP_QUERY_H
#define K3BCDDBP_QUERY_H

#include "k3bcddbquery.h"
#include "k3bcddbresult.h"

#include <qstring.h>
#include <qvaluelist.h>

class QSocket;

class K3bCddbpQuery : public K3bCddbQuery
{
  Q_OBJECT

 public:
  K3bCddbpQuery( QObject* parent = 0, const char* name = 0 );
  ~K3bCddbpQuery();

 public slots:
  void setServer( const QString& s, int port = 8080 ) { m_server = s; m_port = port; }

 protected slots:
  void slotHostFound();
  void slotConnected();
  void slotConnectionClosed();
  void slotReadyRead();
  void slotError( int e );
  void doQuery();

 private:
  void cddbpQuit();
  bool readFirstEntry();
  enum State { GREETING, HANDSHAKE, QUERY, QUERY_DATA, READ, READ_DATA, QUIT };

  int m_state;
  QString m_server;
  int m_port;

  QSocket* m_socket;

  QValueList<K3bCddbResultEntry> m_matches;
  QString m_parsingBuffer;
};

#endif
