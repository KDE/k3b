/***************************************************************************
                          k3bmovieview.h  -  description
                             -------------------
    begin                : Tue May 14 2002
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

#ifndef K3BMOVIEVIEW_H
#define K3BMOVIEVIEW_H

#include "../k3bcdcontentsview.h"
#include <qvaluelist.h>
#include "../device/k3bdevice.h"

//class QString;
class KCutLabel;
class QListViewItem;
class QLabel;
class QPoint;
//class KProcess;
class KActionCollection;
class KListView;
class KPopupMenu;
class K3bTcWrapper;
class K3bDvdContent;
class K3bDvdRipListViewItem;
class K3bToolBox;
//class K3bDvdRipperWidget;

/**
  *@author Sebastian Trueg
  */

class K3bMovieView : public K3bCdContentsView  {
   Q_OBJECT
public: 
    K3bMovieView(QWidget *parent=0, const char *name=0);
    ~K3bMovieView();
    void setDevice( K3bDevice* device );
    void reload();
signals:
    void notSupportedDisc( const QString& device );

private:
    K3bDevice* m_device;
    bool m_initialized;
    K3bTcWrapper *m_tcWrapper;
    KListView *m_listView;
    QLabel *m_input, *m_mode, *m_res, *m_aspect, *m_time;
    QLabel *m_video, *m_audio, *m_frames, *m_framerate;
    QLabel* m_fetchingInfoLabel;
    typedef QValueList<K3bDvdContent> DvdTitle;
    DvdTitle m_dvdTitles;
    KActionCollection *m_actionCollection;
    KPopupMenu *m_popupMenu;
    K3bDvdRipListViewItem *m_ripTitle;
    KCutLabel *m_labelDvdInfo;
    QLabel* m_labelTitle;
    K3bToolBox* m_toolBox;
    void setupGUI();
    void setupActions();
    QString filterAudioList( QStringList* );

private slots:
    void slotDvdChecked( bool successful );
    void slotNotSupportedDisc();
    void slotTitleSelected(QListViewItem*item);
    void slotRip();
    void slotUpdateInfoDialog( int i);
    void slotContextMenu( KListView*, QListViewItem*, const QPoint& p);

};

#endif
