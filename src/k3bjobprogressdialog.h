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


#ifndef _K3B_JOB_PROGRESSDIALOG_H_
#define _K3B_JOB_PROGRESSDIALOG_H_

#include <kdialog.h>

#include <k3bjobhandler.h>

#include <qdatetime.h>
#include <qfile.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class KListView;
class QFrame;
class QGroupBox;
class QLabel;
class QListViewItem;
class KProgress;
class QPushButton;
class QTimer;
class K3bJob;
class KCutLabel;
class QCloseEvent;
class KSystemTray;
class QGridLayout;
class QKeyEvent;
class K3bJobProgressSystemTray;



class K3bJobProgressDialog : public KDialog, public K3bJobHandler
{
  Q_OBJECT

 public:
  K3bJobProgressDialog( QWidget* parent = 0, 
			const char* name = 0, 
			bool showSubProgress = true, 
			bool modal = FALSE, 
			WFlags fl = 0 );
  virtual ~K3bJobProgressDialog();

  virtual void setJob( K3bJob* job );
  void setExtraInfo( QWidget *extra );

  /**
   * reimplemented for internal reasons
   */
  void show();

  /**
   * reimplemented for internal reasons
   */
  void hide();

  /**
   * This will show the dialog and then start the given job or
   * if job == 0 the job set with setJob
   * Use instead of exec()
   */
  int startJob( K3bJob* job = 0 );

  QSize sizeHint() const;

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
  virtual void slotProcessedSize( int processed, int size );
  virtual void slotProcessedSubSize( int processed, int size );
  virtual void slotInfoMessage( const QString& infoString, int type );
  virtual void slotDebuggingOutput( const QString&, const QString& );
  virtual void slotNewSubTask(const QString& name);
  virtual void slotNewTask(const QString& name);
  virtual void slotFinished(bool);
  virtual void slotCanceled();
  virtual void slotStarted();


  void slotCancelButtonPressed();
  void slotUpdateTime();
  void slotShowDebuggingOutput();

  void slotUpdateCaption( int );

 protected:
  void closeEvent( QCloseEvent* );
  void keyPressEvent( QKeyEvent* e );

  void setupGUI();
  void setupConnections();
	
  KCutLabel* m_labelJob;
  KCutLabel* m_labelJobDetails;
  KListView* m_viewInfo;
  KCutLabel* m_labelTask;
  QLabel* m_labelElapsedTime;
  KCutLabel* m_labelSubTask;
  QLabel* m_labelSubProcessedSize;
  KProgress* m_progressSubPercent;
  QLabel* m_labelProcessedSize;
  KProgress* m_progressPercent;
  QFrame* m_frameExtraInfo;
  QPushButton* m_buttonCancel;
  QPushButton* m_buttonClose;
  QPushButton* m_buttonShowDebug;
  QLabel* m_pixLabel;

  QGridLayout* m_frameExtraInfoLayout;

  // debugging output display
  class PrivateDebugWidget;

 private:
  class Private;
  Private* d;

  K3bJob* m_job;
  QTimer* m_timer;
  QTime m_startTime;
  QFile m_logFile;

  QMap<QString, QStringList> m_debugOutputMap;

  bool m_bCanceled;
  bool m_bShowSystemTrayProgress;

  QString m_plainCaption;

  bool in_loop;

  K3bJobProgressSystemTray* m_systemTray;
};


#endif
