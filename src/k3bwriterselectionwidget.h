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


#ifndef K3BWRITERSELECTIONWIDGET_H
#define K3BWRITERSELECTIONWIDGET_H

#include <qwidget.h>

class KComboBox;
class KConfig;
class QLabel;
class K3bDeviceComboBox;
namespace K3bCdDevice {
  class CdDevice;
}

/**
  *@author Sebastian Trueg
  */
class K3bWriterSelectionWidget : public QWidget
{
   Q_OBJECT

 public: 
  K3bWriterSelectionWidget(QWidget *parent=0, const char *name=0);
  ~K3bWriterSelectionWidget();

  int writerSpeed() const;
  K3bCdDevice::CdDevice* writerDevice() const;

  /**
   * returns K3b::WritingApp
   * DEFAULT, CDRECORD, CDRDAO
   */
  int writingApp() const;

  void loadConfig( KConfig* );
  void saveConfig( KConfig* );

 public slots:
  void setWriterDevice( K3bCdDevice::CdDevice* );
  void setSpeed( int );

  /**
   * K3b::CDRDAO and K3b::CDRECORD or'ed together
   */
  void setSupportedWritingApps( int );

 signals:
  void writerChanged();
  void writingAppChanged( int app );

 private slots:
  void slotRefreshWriterSpeeds();
  void slotWritingAppSelected( int id );
  void slotConfigChanged( KConfig* c );
  void slotSpeedChanged( int index );
  void slotWriterChanged();

 private:
  void init();

  int selectedWritingApp() const;

  KComboBox* m_comboSpeed;
  K3bDeviceComboBox* m_comboWriter;
  KComboBox* m_comboWritingApp;
  QLabel* m_writingAppLabel;

  class Private;
  Private* d;
};

#endif
