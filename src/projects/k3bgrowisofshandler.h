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

#ifndef _K3B_GROWISOFS_HANDLER_H_
#define _K3B_GROWISOFS_HANDLER_H_

#include <qobject.h>


/**
 * This class handles the output parsing for growisofs
 * We put it in an extra class since we have two classes
 * using growisofs: the writer and the imager.
 */
class K3bGrowisofsHandler : public QObject
{
  Q_OBJECT

 public:
  K3bGrowisofsHandler( QObject* parent = 0, const char* name = 0 );
  ~K3bGrowisofsHandler();

  enum ErrorType {
    ERROR_UNKNOWN,
    ERROR_MEDIA,
    ERROR_OVERSIZE,
    ERROR_SPEED_SET_FAILED
  };

  int error() const { return m_error; }

 public slots:
  /**
   * This will basicly reset the error type
   * @param dao was growisofs called with DAO?
   */
  void reset( bool dao = false );

  void handleLine( const QString& );
  void handleExit( int exitCode );

 signals:
  void infoMessage( const QString&, int );
  void newSubTask( const QString& );
  void deviceBuffer( int );

 private:
  int m_error;
  bool m_dao;
};

#endif
