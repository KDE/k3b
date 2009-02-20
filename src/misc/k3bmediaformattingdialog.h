/*
 *
 * Copyright (C) 2007 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_MEDIA_FORMATTING_DIALOG_H_
#define _K3B_MEDIA_FORMATTING_DIALOG_H_

#include <k3binteractiondialog.h>


class QCheckBox;
class K3bWritingModeWidget;
class K3bWriterSelectionWidget;
namespace K3bDevice {
  class Device;
}

class K3bMediaFormattingDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public:
  K3bMediaFormattingDialog( QWidget* = 0 );
  ~K3bMediaFormattingDialog();

 public Q_SLOTS:
  void setDevice( K3bDevice::Device* );

 protected Q_SLOTS:
  void slotStartClicked();

 protected:
  void toggleAll();

 private:
  void loadUserDefaults( const KConfigGroup& );
  void saveUserDefaults( KConfigGroup );
  void loadK3bDefaults();

  K3bWriterSelectionWidget* m_writerSelectionWidget;
  K3bWritingModeWidget* m_writingModeWidget;
  QCheckBox* m_checkForce;
  QCheckBox* m_checkQuickFormat;
};

#endif
