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


#ifndef _K3B_APPLICATION_H_
#define _K3B_APPLICATION_H_

#include <kapplication.h>
#include <dcopobject.h>

#define k3bapp K3bApplication::k3bApplication()

class K3bMainWindow;
class K3bCore;
class K3bInterface;
class K3bSongManager;


class K3bApplication : public KApplication, public DCOPObject
{
  Q_OBJECT

 public:
  K3bApplication();
  ~K3bApplication();

  void init();

  K3bMainWindow* k3bMainWindow() const;
  K3bSongManager* songManager() const { return m_songManager; }

  /**
   * Dispatches any incoming DCOP message for a new instance.
   *
   * If it is not a request for a new instance, return false.
   * Overloaded from DCOPObject to hide the reuseInstance method
   * from the public DCOP api.
   * @param fun DCOP function signature
   * @param data the data for the arguments
   * @param replyType the type of the reply value
   * @param replyData the reply
   * @see DCOPObject
   */
  bool process( const QCString& fun, const QByteArray& data,
		QCString& replyType, QByteArray& replyData );

  static K3bApplication* k3bApplication() { return s_k3bApp; }

 signals:
  void initializationInfo( const QString& );
  void initializationDone();

 private slots:
  void slotShutDown();
  void reuseInstance();

 private:
  /**
   * Returns true if no dialog is opened by the args (and the tips may be shown on startup)
   */
  bool processCmdLineArgs();

  K3bCore* m_core;
  K3bInterface* m_interface;
  K3bMainWindow* m_mainWindow;
  K3bSongManager* m_songManager;

  QByteArray m_reuseData;

  static K3bApplication* s_k3bApp;
};

#endif
