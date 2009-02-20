/* 
 *
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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


#ifndef _K3B_VIDEOCD_RIPPING_DIALOG_H_
#define _K3B_VIDEOCD_RIPPING_DIALOG_H_

#include <qstringlist.h>
//Added by qt3to4:
#include <QLabel>

#include <k3binteractiondialog.h>
#include <k3bdiskinfo.h>
#include "k3bvideocdrippingoptions.h"

class QCheckBox;
class QLabel;
class KUrlRequester;
class K3bTempDirSelectionWidget;

class K3bVideoCdRippingDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public: 
  K3bVideoCdRippingDialog( K3bVideoCdRippingOptions* options, QWidget* parent = 0 );
  ~K3bVideoCdRippingDialog();

 private:
  void setupGui();
  void setupContextHelp();

  void loadK3bDefaults();
  void loadUserDefaults( const KConfigGroup& );
  void saveUserDefaults( KConfigGroup );
  
  K3bTempDirSelectionWidget* m_tempDirSelectionWidget;

  KUrlRequester* m_editDirectory;

  QLabel* m_labelFreeSpace;
  QLabel* m_labelNecessarySize;
  QCheckBox* m_ignoreExt;
  QCheckBox* m_sector2336;
  QCheckBox* m_extractXML;

  K3bVideoCdRippingOptions* m_videooptions;

  unsigned long m_freeSpace;

 private Q_SLOTS:
  void slotStartClicked();

  void slotUpdateFreeSpace();
  void slotFreeSpace(const QString&, unsigned long, unsigned long, unsigned long);

};

#endif
