/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_TRM_H_
#define _K3B_TRM_H_

#include <qobject.h>
#include "ktrm.h"

class K3bTRMLookup : public QObject, public KTRMLookup
{
  Q_OBJECT

 public:
  /**
   * This will start the lookup immediately.
   *
   * \code
   * K3bTRMLookup* l = new K3bTRMLookup( "file.mp3" );
   * connect( l, SIGNAL(lookupFinished(K3bTRMLookup*)), this, SLOT(slotHandleLookup(K3bTRMLookup*)) );
   * \endcode
   *
   * And in slotHandleLookup do womthing like:
   *
   * \code
   * if( lookup->resultState() == K3bTRMLookup::RECOGNIZED )
   *   ...
   * else if( lookup->resultState() == K3bTRMLookup::UNRECOGNIZED )
   * \endcode
   */
  K3bTRMLookup( const QString& file, QObject* parent = 0, const char* name = 0 );
  ~K3bTRMLookup();

  void recognized();
  void unrecognized();
  void collision();
  void error();
  void finished();

  enum ResultState {
    ERROR,
    RECOGNIZED,
    UNRECOGNIZED,
    COLLISION     /**< multiple results */
  };

  ResultState resultState() const { return m_resultState; }

 signals:
  /**
   * Connect to this signal and ask the lookup object
   * for the state and the results.
   */
  void lookupFinished( K3bTRMLookup* );

 private:
  ResultState m_resultState;
};

#endif
