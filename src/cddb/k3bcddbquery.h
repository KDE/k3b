/***************************************************************************
                          k3bcddblookup.h  -  description
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

#ifndef K3BCDDB_QUERY_H
#define K3BCDDB_QUERY_H

#include <qobject.h>
#include <qstring.h>
#include <qtextstream.h>

#include "k3bcddbresult.h"

#include <device/k3btoc.h>



class K3bCddbQuery : public QObject
{
  Q_OBJECT

 public:
  K3bCddbQuery( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bCddbQuery();

  void query( const K3bToc& );
  const K3bCddbResult& queryResult() const { return m_queryResult; }

  static const QStringList& categories();

  enum Error { SUCCESS = 0, 
	       NO_ENTRY_FOUND, 
	       CONNECTION_ERROR,
	       QUERY_ERROR,
	       READ_ERROR,
	       FAILURE, 
	       WORKING };

  int error() const { return m_error; }

 signals:
  void queryFinished( K3bCddbQuery* );
  void infoMessage( const QString& );

 protected slots:
  virtual void doQuery() = 0;

 protected:
  const K3bToc& toc() const { return m_toc; }
  K3bCddbResult& queryResult() { return m_queryResult; }
  void setError( int e ) { m_error = e; }

  bool parseEntry( QTextStream&, K3bCddbResultEntry& );
  int getCode( const QString& );
  QString handshakeString() const;
  QString queryString() const;
  bool parseExactMatch( const QString &line, K3bCddbResultEntry& entry );

  /**
   * since I'm not quite sure when the socket will emit connectionClosed
   * this method makes sure the queryFinished signal
   * gets emited only once.
   */
  void emitQueryFinished();

 private:
  K3bToc m_toc;
  K3bCddbResult m_queryResult;
  int m_error;

  bool m_bQueryFinishedEmited;
};

#endif
