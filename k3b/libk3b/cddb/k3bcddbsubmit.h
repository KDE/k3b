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
