/* 
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_DATAMODE_WIDGET_H_
#define _K3B_DATAMODE_WIDGET_H_

#include <KConfigGroup>

#include <QComboBox>


namespace K3b {
class DataModeWidget : public QComboBox
{
  Q_OBJECT

 public:
  explicit DataModeWidget( QWidget* parent = 0 );
  ~DataModeWidget() override;

  /**
   * returns K3b::DataMode
   */
  int dataMode() const;

  void saveConfig( KConfigGroup );
  void loadConfig( const KConfigGroup& );

 public Q_SLOTS:
  void setDataMode( int );
};
}

#endif
