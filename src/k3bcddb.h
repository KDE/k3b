/***************************************************************************
                          k3bcddb.h  -  description
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

#ifndef K3BCDDB_H
#define K3BCDDB_H

#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>

#include "device/k3btoc.h"


class QSocket;
class KConfig;

namespace KIO {
  class Job;
}


class K3bCddbEntry
{
 public:
  QStringList titles;
  QStringList artists;
  QStringList extInfos;

  QString cdTitle;
  QString cdArtist;
  QString cdExtInfo;

  QString category;
  QString discid;
};


class K3bCddbQuery
{
 public:
  K3bCddbQuery();
  //  K3bCddbQuery( const K3bCddbQuery& );

  void clear();
  void addEntry( const K3bCddbEntry& = K3bCddbEntry() );
  const K3bCddbEntry& entry( unsigned int number = 0 ) const;
  int foundEntries() const;

 private:
  QValueList<K3bCddbEntry> m_entries;

  K3bCddbEntry m_emptyEntry;
};


class K3bCddb : public QObject 
{
  Q_OBJECT

 public:
  K3bCddb( QObject* parent = 0, const char* name = 0 );
  ~K3bCddb();

  int error() const { return m_error; }
  int queryType() const { return m_queryType; }
  const K3bCddbQuery& queryResult() const { return m_query; }

  enum Error { SUCCESS = 0, 
	       NO_ENTRY_FOUND, 
	       QUERY_ERROR,
	       READ_ERROR,
	       FAILURE, 
	       WORKING };
  enum QueryType { CDDBP, HTTP, LOCAL };

 public slots:  
  /** query a cd and connect to the queryFinished signal */
  void query( const K3bToc& );
  void localQuery( const K3bToc& );
  void httpQuery( const K3bToc& );
  void cddbpQuery( const K3bToc& );
  void readConfig( KConfig* c );

 signals:
  void queryFinished( K3bCddb* );
  void infoMessage( const QString& );

 private slots:
  void searchLocalDir();
  void queryCdOnServer();

  void slotHostFound();
  void slotConnected();
  void slotConnectionClosed();
  void slotReadyRead();
  void slotError( int e );

  void statJobFinished( KIO::Job* );

 private:
  K3bCddbEntry parseEntry( QTextStream& );
  bool readFirstEntry();
  int  getCode( const QString& );
  bool splitServerPort( const QString&, QString& server, int& port );
  void cddbpQuit();
  QString handshakeString() const;
  QString queryString() const;
  QString createHttpUrl( unsigned int i );
  bool connectToHttpServer( unsigned int );

  enum state { GREETING, HANDSHAKE, QUERY, QUERY_DATA, READ, READ_DATA, QUIT };

  int m_state;
  int m_error;
  int m_queryType;

  K3bCddbQuery m_query;
  QValueList<K3bCddbEntry> m_matches;
  K3bToc m_toc;
  QSocket* m_socket;
  K3bCddbEntry m_currentEntry;
  QString m_parsingBuffer;

  unsigned int m_iCurrentQueriedServer;
  unsigned int m_iCurrentLocalDir;

  QString m_localCddbFile;
  QString m_currentlyConnectingServer;

  // config
  QStringList m_cddbpServer;
  QStringList m_httpServer;
  QString m_proxyServer;
  int m_proxyPort;
  QString m_cgiPath;
  bool m_bUseProxyServer;
  bool m_bUseKdeSettings;
  QStringList m_localCddbDirs;
  bool m_bSaveCddbEntriesLocally;
  bool m_bCddbpQuery;
  bool m_bSearchLocalDirs;
  bool m_bUseManualCgiPath;
};
  

#endif
