/***************************************************************************
                          k3baudioburninfodialog.h  -  description
                             -------------------
    begin                : Thu Apr 5 2001
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

#ifndef K3BBURNPROGRESSDIALOG_H
#define K3BBURNPROGRESSDIALOG_H

#include <kdialog.h>

#include <qvariant.h>
#include <qmap.h>
#include <qstringlist.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QGroupBox;
class QLabel;
class KProgress;
class QPushButton;
class KListView;
class K3bDoc;
class QTime;
class QTimer;
class K3bJob;
class K3bBurnJob;
class KCutLabel;
class QCloseEvent;

/**
  *@author Sebastian Trueg
  */

class K3bBurnProgressDialog : public KDialog  {

  Q_OBJECT

 public:
  K3bBurnProgressDialog( QWidget *parent=0, const char *name=0);
  ~K3bBurnProgressDialog();

  void setJob( K3bBurnJob* job );
  /** reimplemented from QDialog since we need this to return imideately! */
  void show();

 protected:
  /**
   * reimplemented from QWidget since the user should really not
   * close this widget another way than the close button!
   */
  void closeEvent( QCloseEvent* ) {}

  void setupGUI();
  void setupConnections();
	
  QGroupBox* m_groupInfo;
  KListView* m_viewInfo;
  QPushButton* m_buttonCancel;
  QPushButton* m_buttonClose;
  QPushButton* m_buttonShowDebug;
  QGroupBox* m_groupBuffer;
  KProgress* m_progressBuffer;
  QGroupBox* m_groupProgress;
  KProgress* m_progressTrack;
  KProgress* m_progressCd;
  KCutLabel* m_labelFileName;
  QLabel* m_labelTrackProgress;
  QLabel* m_labelCdTime;
  QLabel* m_labelCdProgress;
  QLabel* m_labelWriter;
		
  QGridLayout* mainLayout;
  QHBoxLayout* m_groupInfoLayout;
  QHBoxLayout* m_groupBufferLayout;
  QGridLayout* m_groupProgressLayout;

  // debugging output display
  class PrivateDebugWidget : public KDialog {
  public:
    PrivateDebugWidget( QMap<QString, QStringList>&, QWidget* parent );
  };

 private:
  K3bBurnJob* m_job;
  QTimer* m_timer;
  int m_time;

  QMap<QString, QStringList> m_debugOutputMap;

 protected slots:
//  void updateCdTimeProgress( const QTime& processedTime );
  void updateCdSizeProgress( int processed, int size );
  //  void updateTrackTimeProgress( const QTime& processedTrackTime );
  void updateTrackSizeProgress( int processed, int size );
  void displayInfo( const QString& infoString, int type );

  void mapDebuggingOutput( const QString&, const QString& );

  void finished();
  void slotCancelPressed();
  void slotNewSubTask(const QString& name);
  void slotNewTask(const QString& name);
  void started();
  void slotUpdateTime();

  void slotShowDebuggingOutput();
};

#endif
