/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
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
class KConfigBase;
class QLabel;
class K3bDeviceComboBox;
class QToolButton;
namespace K3bDevice {
  class Device;
}

/**
  *@author Sebastian Trueg
  */
class K3bWriterSelectionWidget : public QWidget
{
   Q_OBJECT

 public: 
  /**
   * Creates a writerselectionwidget
   * @param dvd if true only dvd writers are presented for selection
   */
  K3bWriterSelectionWidget( bool dvd, QWidget* parent = 0, const char* name = 0 );
  ~K3bWriterSelectionWidget();

  int writerSpeed() const;
  K3bDevice::Device* writerDevice() const;

  /**
   * returns K3b::WritingApp
   */
  int writingApp() const;

  void loadDefaults();
  void loadConfig( KConfigBase* );
  void saveConfig( KConfigBase* );

 public slots:
  void setWriterDevice( K3bDevice::Device* );
  void setSpeed( int );
  void setWritingApp( int );

  /**
   * K3b::WritingApp or'ed together
   */
  void setSupportedWritingApps( int );

  /**
   * if dvd is true only DVD writers are presented for selection
   */
  void setDvd( bool );

  /**
   * A simple hack to disable the speed selection for DVD formatting
   */
  void setForceAutoSpeed( bool );

 signals:
  void writerChanged();
  void writingAppChanged( int app );

 private slots:
  void slotRefreshWriterSpeeds();
  void slotWritingAppSelected( int id );
  void slotConfigChanged( KConfigBase* c );
  void slotSpeedChanged( int index );
  void slotWriterChanged();
  void slotDetermineSupportedWriteSpeeds();

 private:
  void init();
  void clearSpeedCombo();
  void insertSpeedItem( int );
  int selectedWritingApp() const;
  void insertWritingSpeedsUpTo( int max );

  KComboBox* m_comboSpeed;
  K3bDeviceComboBox* m_comboWriter;
  KComboBox* m_comboWritingApp;
  QLabel* m_writingAppLabel;
  QToolButton* m_buttonDetermineSpeed;

  class Private;
  Private* d;
};

#endif
