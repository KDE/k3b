/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3BSETUP2_H_
#define _K3BSETUP2_H_

#include <kcmodule.h>
#include <kaboutdata.h>


class base_K3bSetup2;


class K3bSetup2: public KCModule
{
  Q_OBJECT

 public:
  K3bSetup2( QWidget* parent = 0, const char* name = 0, const QStringList& args = QStringList() );
  ~K3bSetup2();

  int buttons();
  QString quickHelp() const;
  const KAboutData* aboutData() { return m_aboutData; };

  void load();
  void save();
  void defaults();

 public slots:
  void updateViews();

 private slots:
  void slotSearchPrograms();
  void slotAddDevice();

 private:
  void updatePrograms();
  void updateDevices();
  QString burningGroup() const;

  class Private;
  Private* d;

  base_K3bSetup2* w;
  
  KAboutData* m_aboutData;
};

#endif
