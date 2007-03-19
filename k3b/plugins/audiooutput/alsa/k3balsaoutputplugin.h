/* 
 *
 * $Id: k3bartsoutputplugin.h 369057 2004-12-07 14:05:11Z trueg $
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_ALSA_AUDIO_OUTPUT_H_
#define _K3B_ALSA_AUDIO_OUTPUT_H_

#include <k3baudiooutputplugin.h>
#include <k3bpluginconfigwidget.h>

class KComboBox;


class K3bAlsaOutputPlugin : public K3bAudioOutputPlugin
{
 public:
  K3bAlsaOutputPlugin( QObject* parent = 0, const char* name = 0 );
  ~K3bAlsaOutputPlugin();

  int pluginSystemVersion() const { return 3; }
  QCString soundSystem() const { return "alsa"; }

  bool init();
  void cleanup();

  QString lastErrorMessage() const;

  int write( char* data, int len );

  K3bPluginConfigWidget* createConfigWidget( QWidget* parent = 0, 
					     const char* name = 0 ) const;

 private:
  bool setupHwParams();
  bool recoverFromError( int err );

  class Private;
  Private* d;
};


class K3bAlsaOutputPluginConfigWidget : public K3bPluginConfigWidget
{
  Q_OBJECT

 public:
  K3bAlsaOutputPluginConfigWidget( QWidget* parent = 0, const char* name = 0 );
  ~K3bAlsaOutputPluginConfigWidget();

 public slots:
  void loadConfig();
  void saveConfig();

 private:
  KComboBox* m_comboDevice;
};

#endif
