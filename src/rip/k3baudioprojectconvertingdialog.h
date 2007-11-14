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


#ifndef _K3B_AUDIO_PROJECT_CONVERTING_DIALOG_H_
#define _K3B_AUDIO_PROJECT_CONVERTING_DIALOG_H_

#include <k3binteractiondialog.h>
#include <k3bmsf.h>

#include <qstringlist.h>


class K3bListView;
class QCheckBox;
class QSpinBox;
class QComboBox;
class K3bCddbPatternWidget;
class QToolButton;
class K3bAudioConvertingOptionWidget;
class K3bCddbResultEntry;
class K3bAudioConvertingJob;
class K3bJobHandler;
class K3bAudioDoc;


/**
  *@author Sebastian Trueg
  */
class K3bAudioProjectConvertingDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public: 
  K3bAudioProjectConvertingDialog( K3bAudioDoc*, QWidget *parent = 0, const char *name = 0 );
  ~K3bAudioProjectConvertingDialog();

  void setBaseDir( const QString& path );

 public slots:  
  void refresh();

 protected:
  void loadK3bDefaults();
  void loadUserDefaults( const KConfigGroup& );
  void saveUserDefaults( KConfigGroup& );

 private:
  K3bCddbPatternWidget* m_patternWidget;
  K3bAudioConvertingOptionWidget* m_optionWidget;

  K3bListView* m_viewTracks;
  K3bAudioDoc* m_doc;

  void setupGui();

  static K3bCddbResultEntry createCddbEntryFromDoc( K3bAudioDoc* );

  class Private;
  Private* d;
  
 private slots:
  void slotStartClicked();
};

#endif
