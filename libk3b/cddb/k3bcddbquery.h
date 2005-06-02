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


#ifndef K3BCDDB_QUERY_H
#define K3BCDDB_QUERY_H

#include <qobject.h>
#include <qstring.h>
#include <qtextstream.h>

#include "k3bcddbresult.h"

#include <k3btoc.h>



class K3bCddbQuery : public QObject
{
  Q_OBJECT

 public:
  K3bCddbQuery( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bCddbQuery();

  void query( const K3bToc& );

  /**
   * Use this if the query returned multible matches
   */
  void queryMatch( const K3bCddbResultHeader& );

  const K3bCddbResultEntry& result() const { return m_result; }

  /**
   * After emitting the signal inexactMatches one has to choose one
   * of these entries and query it with queryInexactMatch
   */
  const QValueList<K3bCddbResultHeader>& getInexactMatches() const { return m_inexactMatches; }

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
  /**
   * This gets emitted if a single entry has been found or 
   * no entry has been found.
   */
  void queryFinished( K3bCddbQuery* );

  /**
   * This gets emitted if multible entries have been found.
   * Call queryInexactMatch() after receiving it.
   */
  void inexactMatches( K3bCddbQuery* );

  void infoMessage( const QString& );

 protected slots:
  virtual void doQuery() = 0;
  virtual void doMatchQuery() = 0;

 protected:
  const K3bToc& toc() const { return m_toc; }
  K3bCddbResultHeader& header() { return m_header; }
  K3bCddbResultEntry& result() { return m_result; }
  void setError( int e ) { m_error = e; }

  bool parseEntry( QTextStream&, K3bCddbResultEntry& );
  int getCode( const QString& );
  QString handshakeString() const;
  QString queryString() const;
  bool parseMatchHeader( const QString& line, K3bCddbResultHeader& header );

  /**
   * since I'm not quite sure when the socket will emit connectionClosed
   * this method makes sure the queryFinished signal
   * gets emited only once.
   */
  void emitQueryFinished();

  QValueList<K3bCddbResultHeader> m_inexactMatches;

 private:
  K3bToc m_toc;
  K3bCddbResultEntry m_result;
  K3bCddbResultHeader m_header;
  int m_error;

  bool m_bQueryFinishedEmited;
};

#endif
