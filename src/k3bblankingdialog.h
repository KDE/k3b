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


#ifndef K3B_BLANKING_DIALOG_H
#define K3B_BLANKING_DIALOG_H

#include "k3binteractiondialog.h"
#include "device/k3bdevice.h"

class QGroupBox;
class QComboBox;
class QCloseEvent;
class KListView;
class K3bWriterSelectionWidget;


class K3bBlankingDialog : public K3bInteractionDialog
{
Q_OBJECT

 public:
  K3bBlankingDialog( QWidget*, const char* );
  ~K3bBlankingDialog();

 protected slots:
  void slotStartClicked();
  void slotInfoMessage( const QString& msg, int type );
  void slotWriterChanged();
  void slotWritingAppChanged( int );
  void slotJobFinished( bool );

  void slotLoadK3bDefaults();
  void slotLoadUserDefaults();
  void slotSaveUserDefaults();

 private:
  void setupGui();

  K3bWriterSelectionWidget* m_writerSelectionWidget;

  QComboBox* m_comboEraseMode;
  QGroupBox* m_groupOutput;
  KListView* m_viewOutput;

  class Private;
  Private* d;
};

#endif
