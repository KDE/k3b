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

#ifndef _K3B_SESSION_IMPORT_DIALOG_H_
#define _K3B_SESSION_IMPORT_DIALOG_H_

#include <kdialogbase.h>

#include <qptrlist.h>
#include <qmap.h>

#include <k3bdevice.h>

namespace K3bDevice {
  class DiskInfoDetector;
}

class QLabel;
class K3bBusyWidget;
class KListBox;
class K3bDataDoc;


class K3bDataSessionImportDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bDataSessionImportDialog( QWidget* parent = 0 );
  ~K3bDataSessionImportDialog();

  /**
   * Import a session into the project.
   * If the project is a DVD data project only DVD media are
   * presented for selection.
   */
  void importSession( K3bDataDoc* doc );

  /**
   * Convinience method.
   */
  static bool importSession( K3bDataDoc* doc, QWidget* parent );

 private slots:
  void slotOk();
  void slotCancel();

  void checkNextDevice();
  void slotDiskInfoReady( K3bDevice::DiskInfoDetector* );

  void slotSelectionChanged();

 private:
  K3bDevice::DiskInfoDetector* m_diskInfoDetector;

  K3bDataDoc* m_doc;

  QLabel* m_processLabel;
  K3bBusyWidget* m_busyWidget;
  KListBox* m_selectionBox;

  bool m_alreadySelected;

  QPtrList<K3bDevice::Device> m_devices;
  QMap<int, K3bDevice::Device*> m_deviceMap;
};

#endif
