/***************************************************************************
                          k3bfilmview.h  -  description
                             -------------------
    begin                : Fri Feb 22 2002
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

#ifndef K3BFILMVIEW_H
#define K3BFILMVIEW_H

#include "../k3bcdcontentsview.h"

#include <qvaluelist.h>

class QString;
class QListViewItem;
class QLabel;
class KProcess;
class KListView;
class K3bTcWrapper;
class K3bDvdContent;
class K3bDevice;
class K3bDvdRipperWidget;

/**
  *@author Sebastian Trueg
  */
class K3bFilmView : public K3bCdContentsView
{
  Q_OBJECT

 public: 
  K3bFilmView(QWidget *parent=0, const char *name=0);
  ~K3bFilmView();
  void setDevice( K3bDevice* device );
  void reload();

 private:
  K3bDevice* m_device;
  bool m_initialized;
  K3bTcWrapper *m_tcWrapper;
  KListView *m_chapterView;
  KListView *m_titleView;
  KListView *m_audioView;
  QLabel *m_input, *m_mode, *m_res, *m_aspect, *m_time;
  QLabel *m_video, *m_audio, *m_frames, *m_framerate;
  typedef QValueList<K3bDvdContent> DvdTitle;
  DvdTitle m_dvdTitles;
  void setupGui();
  void setCheckBoxes( KListView *m_audioView, bool status );

  //K3bDvdRipperWidget *m_ripWidget;

 signals:
  void notSupportedDisc( const QString& device );


 private slots:
 //void ripperWidgetClosed();
 void slotDvdChecked( bool successful );
  void slotNotSupportedDisc();
  void slotTitleSelected(QListViewItem*item);
  void slotAudioButtonAll();
  void slotAudioButtonNone();
  void slotChapterButtonAll();
  void slotChapterButtonNone();
  void slotRip();
  //void slotReload();
};

#endif
