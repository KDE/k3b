/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
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

#ifndef _K3B_INTERFERING_SYSTEMS_HANDLER_H_
#define _K3B_INTERFERING_SYSTEMS_HANDLER_H_

#include <k3bjob.h>


/**
 * This class can disable and enable local systems that
 * interfere with CD/DVD burning.
 *
 * Currently supported systems are:
 * \li KDED module mediamanager (disabled through dcop)
 * \li SuSEPlugger (killed by pid and fired up again by desktop service)
 * \li automounting systems subfs, supermount, and the kernel based autofs (handled through 
 *     a script running suid root)
 *
 * Also the K3bInterferingSystemsHandler is a K3bJob it is not intended to be used as such.
 * That means the normal K3bJob slots start() and cancel() do nothing. It has to be used
 * through disable() and enable().
 * There will be no started() or finished() signals.
 */
class K3bInterferingSystemsHandler : public K3bJob
{
  Q_OBJECT

 public:
  K3bInterferingSystemsHandler( QObject* parent = 0, const char* name = 0 );
  ~K3bInterferingSystemsHandler();

 public slots:
  void setDevice( K3bDevice::Device* );
  void disable( K3bDevice::Device* );
  void disable();
  void enable();

  void start() {}
  void cancel() {}

 private:
  int startStopMediaManager( bool start );
  int startStopSuSEPlugger( bool start );

  class Private;
  Private* d;
};

#endif
