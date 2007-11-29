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


#ifndef _K3B_AUDIO_RIPPING_DIALOG_H_
#define _K3B_AUDIO_RIPPING_DIALOG_H_

#include <k3binteractiondialog.h>

#include <qstringlist.h>
//Added by qt3to4:
#include <Q3ValueList>

#include <k3bcddbquery.h>

namespace K3bDevice {
  class Device;
  class Toc;
}


class K3ListView;
class QCheckBox;
class QSpinBox;
class QComboBox;
class K3bCddbPatternWidget;
class QToolButton;
class K3bAudioConvertingOptionWidget;


/**
  *@author Sebastian Trueg
  */
class K3bAudioRippingDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public: 
  K3bAudioRippingDialog( const K3bDevice::Toc&, 
			 K3bDevice::Device*,
			 const K3bCddbResultEntry&, 
			 const QList<int>&, 
			 QWidget *parent = 0, const char *name = 0 );
  ~K3bAudioRippingDialog();

  void setStaticDir( const QString& path );

 public slots:  
  void refresh();
  void init();

 private:
  K3bDevice::Toc m_toc;
  K3bDevice::Device* m_device;
  K3bCddbResultEntry m_cddbEntry;
  QList<int> m_trackNumbers;

  K3ListView*    m_viewTracks;

  QComboBox* m_comboParanoiaMode;
  QSpinBox* m_spinRetries;
  QCheckBox* m_checkIgnoreReadErrors;
  QCheckBox* m_checkUseIndex0;

  K3bCddbPatternWidget* m_patternWidget;
  K3bAudioConvertingOptionWidget* m_optionWidget;

  void setupGui();
  void setupContextHelp();

  void loadK3bDefaults();
  void loadUserDefaults( const KConfigGroup& );
  void saveUserDefaults( KConfigGroup& );

  class Private;
  Private* d;
  
 private slots:
  void slotStartClicked();
};

#endif
