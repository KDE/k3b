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
class QMultiLineEdit;


class K3bAudioDoc;
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
   void setupBurnTab( QFrame* frame );
   void setupCdTextTab( QFrame* frame );
   void saveSettings();
   void readSettings();

   // the burn tab
   // ---------------------------------------------------------
   QCheckBox* m_checkCdText;
   QCheckBox* m_checkDao;
   QCheckBox* m_checkOnTheFly;
   QCheckBox* m_checkPadding;
   QCheckBox* m_checkSimulate;
   // -----------------------------------------------------------
	
   // the cd-text-tab
   // -----------------------------------------------------------
   QLineEdit* m_editDisc_id;
   QLineEdit* m_editUpc_ean;
   QMultiLineEdit* m_editMessage;
   QLineEdit* m_editPerformer;
   QLineEdit* m_editArranger;
   QLineEdit* m_editTitle;
   QLineEdit* m_editSongwriter;
   // -----------------------------------------------------------
};

#endif
