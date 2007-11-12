/*
 *
 * $Id$
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


#ifndef K3BWRITERSELECTIONWIDGET_H
#define K3BWRITERSELECTIONWIDGET_H

#include <qwidget.h>
//Added by qt3to4:
#include <QLabel>
#include <Q3ValueList>

class KComboBox;
class KConfigBase;
class QLabel;
class K3bMediaSelectionComboBox;
namespace K3bDevice {
  class Device;
  class DeviceManager;
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
   */
  K3bWriterSelectionWidget( QWidget* parent = 0, const char* name = 0 );
  ~K3bWriterSelectionWidget();

  int writerSpeed() const;
  K3bDevice::Device* writerDevice() const;

  Q3ValueList<K3bDevice::Device*> allDevices() const;

  /**
   * returns K3b::WritingApp
   */
  int writingApp() const;

  int wantedMediumType() const;
  int wantedMediumState() const;

  void loadDefaults();
  void loadConfig( KConfigBase* );
  void saveConfig( KConfigBase* );

 public slots:
  void setWriterDevice( K3bDevice::Device* );
  void setSpeed( int );
  void setWritingApp( int );

  /**
   * K3b::WritingApp or'ed together
   *
   * Defaults to cdrecord and cdrdao for CD and growisofs for DVD
   */
  void setSupportedWritingApps( int );

  /**
   * A simple hack to disable the speed selection for DVD formatting
   */
  void setForceAutoSpeed( bool );

  /**
   * Set the wanted medium type. Defaults to writable CD.
   * 
   * \param type a bitwise combination of the K3bDevice::MediaType enum
   */
  void setWantedMediumType( int type );

  /**
   * Set the wanted medium state. Defaults to empty media.
   *
   * \param state a bitwise combination of the K3bDevice::State enum
   */
  void setWantedMediumState( int state );

  /**
   * This is a hack to allow the copy dialogs to use the same device for reading
   * and writing without having the user to choose the same medium.
   *
   * \param overrideString A string which will be shown in place of the medium string.
   *                       For example: "Burn to the same device". Set it to 0 in order
   *                       to disable the feature.
   */
  void setOverrideDevice( K3bDevice::Device* dev, const QString& overrideString = QString::null, const QString& tooltip = QString::null );

  /**
   * Compare K3bMediaSelectionComboBox::setIgnoreDevice
   */
  void setIgnoreDevice( K3bDevice::Device* dev );

 signals:
  void writerChanged();
  void writerChanged( K3bDevice::Device* );
  void writingAppChanged( int app );

  /**
   * \see K3bMediaSelectionComboBox
   */
  void newMedia();
  void newMedium( K3bDevice::Device* dev );

 private slots:
  void slotRefreshWriterSpeeds();
  void slotRefreshWritingApps();
  void slotWritingAppSelected( int id );
  void slotConfigChanged( KConfigBase* c );
  void slotSpeedChanged( int index );
  void slotWriterChanged();
  void slotNewBurnMedium( K3bDevice::Device* dev );
  void slotManualSpeed();

 private:
  void clearSpeedCombo();
  void insertSpeedItem( int );
  int selectedWritingApp() const;

  class MediaSelectionComboBox;

  KComboBox* m_comboSpeed;
  MediaSelectionComboBox* m_comboMedium;
  KComboBox* m_comboWritingApp;
  QLabel* m_writingAppLabel;

  class Private;
  Private* d;
};

#endif
