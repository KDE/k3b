/***************************************************************************
                          k3bcddblocalsubmit.h  -  description
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
