/***************************************************************************
                          k3bvcdburndialog.h  -  description
                             -------------------
    begin                : Son Nov 10 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#ifndef K3BVCDBURNDIALOG_H
#define K3BVCDBURNDIALOG_H


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
class K3bVcdDoc;


class K3bVcdBurnDialog : public K3bProjectBurnDialog
{
   Q_OBJECT

 public:
   K3bVcdBurnDialog(K3bVcdDoc* doc, QWidget *parent=0, const char *name=0, bool modal = true );
   ~K3bVcdBurnDialog();

 protected:
   void setupBurnTab( QFrame* frame );
   void saveSettings();
   void readSettings();

   // the burn tab
   // ---------------------------------------------------------
   QCheckBox* m_checkDao;
   QCheckBox* m_checkOnTheFly;
   QCheckBox* m_checkSimulate;
   QCheckBox* m_checkRemoveBufferFiles;
   K3bWriterSelectionWidget* m_writerSelectionWidget;
   K3bTempDirSelectionWidget* m_tempDirSelectionWidget;
   // -----------------------------------------------------------

 protected slots:
   void slotOk();

   void loadDefaults();
   void loadUserDefaults();
   void saveUserDefaults();
};

#endif
