/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
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

#ifndef K3BMOVIEVIEW_H
#define K3BMOVIEVIEW_H

#include "../k3bcdcontentsview.h"
#include <qvaluelist.h>
#include <qstring.h>


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
namespace K3bCdDevice {
  class CdDevice;
}

/**
  *@author Sebastian Trueg
  */

class K3bMovieView : public K3bCdContentsView  {
   Q_OBJECT
public: 
    K3bMovieView(QWidget *parent=0, const char *name=0);
    ~K3bMovieView();
    void setDevice( K3bCdDevice::CdDevice* device );
    void reload();
signals:
    void notSupportedDisc( const QString& device );

private:
    K3bCdDevice::CdDevice* m_device;
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

    QLabel* m_pixmapLabelLeft;
    QLabel* m_pixmapLabelRight;

private slots:
    void slotDvdChecked( bool successful );
    void slotNotSupportedDisc();
    void slotTitleSelected(QListViewItem*item);
    void slotRip();
    void slotUpdateInfoDialog( int i);
    void slotContextMenu( KListView*, QListViewItem*, const QPoint& p);

    void slotThemeChanged();
};

#endif
