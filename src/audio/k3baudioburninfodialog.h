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

#ifndef K3BAUDIOBURNINFODIALOG_H
#define K3BAUDIOBURNINFODIALOG_H

#include <kdialog.h>

#include <qvariant.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QGroupBox;
class QLabel;
class KProgress;
class QPushButton;
class QTextView;
class K3bAudioDoc;
class QTime;


/**
  *@author Sebastian Trueg
  */

class K3bAudioBurnInfoDialog : public KDialog  {

   Q_OBJECT

public:
	K3bAudioBurnInfoDialog( K3bAudioDoc*, QWidget *parent=0, const char *name=0);
	~K3bAudioBurnInfoDialog();

protected:
	enum action { DECODING = 1, WRITING = 2 };
	int currentAction;
	
	void setupGUI();
	void setupConnections();
	
    QGroupBox* m_groupInfo;
    QTextView* m_viewInfo;
    QPushButton* m_buttonCancel;
    QPushButton* m_buttonOk;
    QGroupBox* m_groupBuffer;
    KProgress* m_progressBuffer;
    QGroupBox* m_groupProgress;
    KProgress* m_progressTrack;
    KProgress* m_progressCd;
    QLabel* m_labelFileName;
    QLabel* m_labelTrackProgress;
    QLabel* m_labelCdTime;
    QLabel* m_labelCdProgress;
	QLabel* m_labelWriter;
		
    QGridLayout* mainLayout;
    QHBoxLayout* m_groupInfoLayout;
    QHBoxLayout* m_groupBufferLayout;
    QGridLayout* m_groupProgressLayout;

private:
	K3bAudioDoc* doc;

signals:
  void cancelPressed();

protected slots:
  void updateCdTimeProgress( const QTime& processedTime );
  void updateCdSizeProgress( unsigned long processed, unsigned long size );
//  void updateTrackTimeProgress( const QTime& processedTrackTime );
  void updateTrackSizeProgress( int processed, int size );
  void displayInfo( const QString& infoString );

  void finished();
  void nextTrack();

public slots:
  void startBurning();
  void startDecoding();
};

#endif
