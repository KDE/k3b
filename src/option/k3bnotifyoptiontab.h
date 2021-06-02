/* 

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/



#ifndef _K3B_NOTIFY_OPTIONTAB_H_
#define _K3B_NOTIFY_OPTIONTAB_H_

#include <QWidget>

class KNotifyConfigWidget;

namespace K3b {
class NotifyOptionTab : public QWidget
{
  Q_OBJECT

 public:
  explicit NotifyOptionTab( QWidget* parent = 0 );
  ~NotifyOptionTab() override;

  void readSettings();
  bool saveSettings();

 private:
   KNotifyConfigWidget *m_notifyWidget;
};
}

#endif
