/***************************************************************************
                          k3baudioplayerwidget.h  -  description
                             -------------------
    begin                : Mon Feb 11 2002
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

#ifndef K3BAUDIOPLAYERWIDGET_H
#define K3BAUDIOPLAYERWIDGET_H

#include <qwidget.h>


class K3bAudioPlayer;
class QLabel;
class QToolButton;
class QTimer;
class QSlider;


/**Control Widget for the K3bAudioPlayer class
  *@author Sebastian Trueg
  */
class K3bAudioPlayerWidget : public QWidget  
{
   Q_OBJECT
     
 public: 
   K3bAudioPlayerWidget( K3bAudioPlayer* player, bool skipButtons = false, QWidget *parent=0, const char *name=0 );
   ~K3bAudioPlayerWidget();

   void init();

 signals:
   void forward();
   void back();

 private slots:
   void slotStarted();
   void slotStarted( const QString& filename );
   void slotPaused();
   void slotStopped();
   void slotEnded();
   void slotUpdateDisplay();

 private:
   K3bAudioPlayer* m_player;

   QLabel* m_labelFilename;
   QLabel* m_labelCurrentTime;
   QLabel* m_labelOverallTime;

   QToolButton* m_buttonPlay;
   QToolButton* m_buttonPause;
   QToolButton* m_buttonStop;
   QToolButton* m_buttonForward;
   QToolButton* m_buttonBack;

   QSlider* m_seekSlider;

   QTimer* m_displayTimer;
   bool m_bLengthReady;
};

#endif
