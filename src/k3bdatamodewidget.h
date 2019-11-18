/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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
