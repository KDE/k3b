/*

    SPDX-FileCopyrightText: 2005 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
