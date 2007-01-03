/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_SESSION_IMPORT_DIALOG_H_
#define _K3B_SESSION_IMPORT_DIALOG_H_

#include <kdialogbase.h>

#include <qptrlist.h>
#include <qmap.h>

#include <k3bdevice.h>


class QLabel;
class KListBox;
class K3bDataDoc;
class K3bMediaSelectionComboBox;


class K3bDataSessionImportDialog : public KDialogBase
{
  Q_OBJECT

 public:
  /**
   * Import a session into the project.
   * If the project is a DVD data project only DVD media are
   * presented for selection.
   *
   * \param doc if 0 a new project will be created.
   *
   * \return the project
   */
  static K3bDataDoc* importSession( K3bDataDoc* doc, QWidget* parent );

 private slots:
  void slotOk();
  void slotCancel();

  void importSession( K3bDataDoc* doc );
  void slotSelectionChanged( K3bDevice::Device* );

 private:
  K3bDataSessionImportDialog( QWidget* parent = 0 );
  ~K3bDataSessionImportDialog();

  K3bDataDoc* m_doc;
  K3bMediaSelectionComboBox* m_comboMedia;
};

#endif
