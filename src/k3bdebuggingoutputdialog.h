/*
 *
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

#ifndef _K3B_DEBUGGING_OUTPUT_DIALOG_H_
#define _K3B_DEBUGGING_OUTPUT_DIALOG_H_

#include <QMap>
#include <QDialog>

class QTextEdit;

namespace K3b {
class DebuggingOutputDialog : public QDialog
{
  Q_OBJECT

 public:
  explicit DebuggingOutputDialog( QWidget* parent );

 public Q_SLOTS:
  void setOutput( const QString& );

  void slotSaveAsClicked();
  void slotCopyClicked();
private:

  QTextEdit* debugView;
};
}



#endif
