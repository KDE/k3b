/* 
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_EXTERNALBIN_OPTIONTAB_H
#define K3B_EXTERNALBIN_OPTIONTAB_H

#include <QWidget>


namespace K3b {
    
class ExternalBinManager;
class ExternalBinWidget;
    
class ExternalBinOptionTab : public QWidget
{
Q_OBJECT

 public:
  explicit ExternalBinOptionTab( ExternalBinManager* manager, QWidget* = 0 );
  ~ExternalBinOptionTab() override;

  void readSettings();
  void saveSettings();

 private:
  ExternalBinManager* m_manager;

  ExternalBinWidget* m_externalBinWidget;
};
}



#endif
