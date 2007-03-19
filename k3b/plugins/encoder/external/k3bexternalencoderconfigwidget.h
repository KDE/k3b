/* 
 *
 * $Id: k3bexternalencoder.cpp 567280 2006-07-28 13:26:27Z trueg $
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_EXTERNAL_ENCODER_CONFIG_WIDGET_H_
#define _K3B_EXTERNAL_ENCODER_CONFIG_WIDGET_H_

#include "base_k3bexternalencodereditwidget.h"
#include "base_k3bexternalencoderconfigwidget.h"
#include "k3bexternalencodercommand.h"

#include <k3bpluginconfigwidget.h>
#include <kdialogbase.h>


class K3bExternalEncoderEditDialog : public KDialogBase
{
  Q_OBJECT
  
 public:
  K3bExternalEncoderEditDialog( QWidget* parent );
  ~K3bExternalEncoderEditDialog();

  K3bExternalEncoderCommand currentCommand() const;
  void setCommand( const K3bExternalEncoderCommand& cmd );

 private slots:
  void slotOk();

 private:
  base_K3bExternalEncoderEditWidget* m_editW;
};


class K3bExternalEncoderSettingsWidget : public K3bPluginConfigWidget
{
  Q_OBJECT

 public:
  K3bExternalEncoderSettingsWidget( QWidget* parent = 0, const char* name = 0 );
  ~K3bExternalEncoderSettingsWidget();

 public slots:
  void loadConfig();
  void saveConfig();

 private slots:
  void slotSelectionChanged();
  void slotNewCommand();
  void slotEditCommand();
  void slotRemoveCommand();

 private:
  base_K3bExternalEncoderConfigWidget* w;
  K3bExternalEncoderEditDialog* m_editDlg;

  class Private;
  Private* d;
};

#endif
