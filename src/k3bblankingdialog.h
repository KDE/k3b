/* 
 *
 * $Id: $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3B_BLANKING_DIALOG_H
#define K3B_BLANKING_DIALOG_H

#include <kdialogbase.h>

class QString;
class QGroupBox;
class QComboBox;
class QButtonGroup;
class QRadioButton;
class QCheckBox;
class QComboBox;
class QCloseEvent;
class KListView;
class K3bDevice;
class K3bBlankingJob;
class KProgress;
class K3bWriterSelectionWidget;


class K3bBlankingDialog : public KDialogBase
{
Q_OBJECT

 public:
  K3bBlankingDialog( QWidget*, const char* );
  ~K3bBlankingDialog();

 protected slots:
  void slotUser1();
  void slotUser2();
  void slotInfoMessage( const QString& msg, int type );
  void slotJobFinished( bool );
  void slotWriterChanged();

 protected:
  void closeEvent( QCloseEvent* );

 private:
  void setupGui();
  
  K3bWriterSelectionWidget* m_writerSelectionWidget;

  QButtonGroup* m_groupBlankType;
  QRadioButton* m_radioCompleteBlank;
  QRadioButton* m_radioFastBlank;
  QRadioButton* m_radioBlankTrack;
  QRadioButton* m_radioUncloseSession;
  QRadioButton* m_radioBlankSession;

  QGroupBox* m_groupOptions;
  QCheckBox* m_checkForce;

  QGroupBox* m_groupOutput;
  KListView* m_viewOutput;

  K3bBlankingJob* m_job;
};

#endif
