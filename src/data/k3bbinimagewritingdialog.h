/***************************************************************************
                          k3bbinimagewritingdialog.h  -  description
                             -------------------
    begin                : Mon Jan 13 2003
    copyright            : (C) 2003 by Klaus-Dieter Krannich
    email                : kd@math.tu-cottbus.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BBINIMAGEWRITINGDIALOG_H
#define K3BBINIMAGEWRITINGDIALOG_H

#include <kdialogbase.h>
#include <kurl.h>
#include <klineedit.h>
#include <qcheckbox.h>
#include <qtoolbutton.h>
#include <qspinbox.h>
#include "k3bwriterselectionwidget.h"
#include "k3bbinimagewritingjob.h"


/**
  *@author Klaus-Dieter Krannich
  */
class K3bBinImageWritingDialog : public KDialogBase
{
Q_OBJECT

 public: 
  K3bBinImageWritingDialog( QWidget* = 0, const char* = 0, bool = true );
  ~K3bBinImageWritingDialog();

  void setTocFile( const KURL& url );

 protected slots:
  void slotUser1();
  void slotUser2();
  void slotFindTocFile();
  void slotWriterChanged();
 private:
  void setupGui();

  K3bBinImageWritingJob* m_job;

  K3bWriterSelectionWidget* m_writerSelectionWidget;
  QCheckBox* m_checkSimulate;
  QCheckBox* m_checkMulti;
  QCheckBox* m_checkForce;
  QSpinBox*  m_spinCopies;
  KLineEdit* m_editTocPath;
  QToolButton* m_buttonFindTocFile;
  QString m_tocPath;
};

#endif
