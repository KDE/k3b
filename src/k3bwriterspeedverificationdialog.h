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

#ifndef _K3B_WRITER_SPEED_VERIFICATION_DIALOG_H_
#define _K3B_WRITER_SPEED_VERIFICATION_DIALOG_H_

#include <kdialogbase.h>

#include <qptrlist.h>
#include <qmap.h>

#include <k3bdevice.h>


class QSpinBox;


class K3bWriterSpeedVerificationDialog : public KDialogBase
{
  Q_OBJECT

 public:
  ~K3bWriterSpeedVerificationDialog();

  static void verify( QPtrList<K3bCdDevice::CdDevice>& wlist, QWidget* parent = 0, const char* name = 0 );

 private slots:
  void slotSpeedChanged( int );

 private:
  K3bWriterSpeedVerificationDialog( QPtrList<K3bCdDevice::CdDevice>& wlist, QWidget* parent = 0, const char* name = 0 );

  QMap<const QSpinBox*, K3bCdDevice::CdDevice*> m_spinMap;
};

#endif
