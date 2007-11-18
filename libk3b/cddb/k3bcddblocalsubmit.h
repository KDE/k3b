/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BCDDB_LOCAL_SUBMIT_H
#define K3BCDDB_LOCAL_SUBMIT_H

#include "k3bcddbsubmit.h"

#include <qstring.h>


class K3bCddbLocalSubmit : public K3bCddbSubmit
{
  Q_OBJECT

 public:
  K3bCddbLocalSubmit( QObject* parent = 0, const char* name = 0 );
  ~K3bCddbLocalSubmit();

 public slots:
  void setCddbDir( const QString& dir ) { m_cddbDir = dir; }

 protected slots:
  void doSubmit();

 private:
  QString m_cddbDir;
};

#endif
