/***************************************************************************
                          k3bcddbsubmit.h  -  description
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

#ifndef K3BCDDB_SUBMIT_H
#define K3BCDDB_SUBMIT_H

#include <qobject.h>
#include <qstring.h>

#include "k3bcddbresult.h"



class K3bCddbSubmit : public QObject
{
  Q_OBJECT

 public:
  K3bCddbSubmit( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bCddbSubmit();

  int error() const { return m_error; }

  enum State { SUCCESS, WORKING, IO_ERROR, CONNECTION_ERROR };

 public slots:
  void submit( const K3bCddbResultEntry& );

 signals:
  void infoMessage( const QString& );
  void submitFinished( K3bCddbSubmit* );

 protected slots:
  virtual void doSubmit() = 0;
  void setError( int e ) { m_error = e; }

 protected:
  K3bCddbResultEntry& resultEntry() { return m_resultEntry; }

 private:
  void createDataStream( K3bCddbResultEntry& entry );

  int m_error;
  K3bCddbResultEntry m_resultEntry;
};

#endif
