/***************************************************************************
                          k3baudioburndialog.h  -  description
                             -------------------
    begin                : Sun Apr 1 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BAUDIOBURNDIALOG_H
#define K3BAUDIOBURNDIALOG_H


#include "../k3bprojectburndialog.h"

#include <qvariant.h>
#include <qwidget.h>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QToolButton;
class K3bWriterSelectionWidget;
class K3bTempDirSelectionWidget;
class K3bAudioDoc;
class K3bAudioCdTextWidget;



/**
  *@author Sebastian Trueg
  */
class K3bAudioBurnDialog : public K3bProjectBurnDialog  
{
   Q_OBJECT

 public:
   K3bAudioBurnDialog(K3bAudioDoc* doc, QWidget *parent=0, const char *name=0, bool modal = true );
   ~K3bAudioBurnDialog();

 protected:
   void saveSettings();
   void readSettings();

   QCheckBox* m_checkHideFirstTrack;
   K3bAudioCdTextWidget* m_cdtextWidget;
   K3bAudioDoc* m_doc;

 protected slots:
   void loadDefaults();
   void loadUserDefaults();
   void saveUserDefaults();
   void slotToggleEverything();
};

#endif
