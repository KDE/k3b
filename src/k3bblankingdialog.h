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


#ifndef K3B_BLANKING_DIALOG_H
#define K3B_BLANKING_DIALOG_H

#include "k3binteractiondialog.h"
#include <k3bjobhandler.h>

class QGroupBox;
class QComboBox;
class QCloseEvent;
class KListView;
class K3bWriterSelectionWidget;
namespace K3bDevice {
  class Device;
}


class K3bBlankingDialog : public K3bInteractionDialog, public K3bJobHandler
{
Q_OBJECT

 public:
  K3bBlankingDialog( QWidget*, const char* );
  ~K3bBlankingDialog();

  /**
   * @reimplemented from K3bJobHandler
   */
  int waitForMedia( K3bDevice::Device*,
		    int mediaState = K3bDevice::STATE_EMPTY,
		    int mediaType = K3bDevice::MEDIA_WRITABLE_CD,
		    const QString& message = QString::null );
  
  /**
   * @reimplemented from K3bJobHandler
   */
  bool questionYesNo( const QString& text,
		      const QString& caption = QString::null );

 protected slots:
  void slotStartClicked();
  void slotInfoMessage( const QString& msg, int type );
  void slotWriterChanged();
  void slotWritingAppChanged( int );
  void slotJobFinished( bool );

 private:
  void setupGui();
  void loadK3bDefaults();
  void loadUserDefaults( KConfigBase* );
  void saveUserDefaults( KConfigBase* );

  K3bWriterSelectionWidget* m_writerSelectionWidget;

  QComboBox* m_comboEraseMode;
  QGroupBox* m_groupOutput;
  KListView* m_viewOutput;

  class Private;
  Private* d;
};

#endif
