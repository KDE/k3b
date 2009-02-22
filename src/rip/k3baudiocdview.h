/* 
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_AUDIOCD_VIEW_H_
#define _K3B_AUDIOCD_VIEW_H_

#include <k3bmediacontentsview.h>
#include <k3bmedium.h>

#include <QLabel>

class QTreeView;
class QPoint;
class KActionCollection;
class KMenu;
class QLabel;
class KToolBar;


namespace K3b {
    class AudioTrackModel;
}

namespace K3b {
class AudioCdView : public MediaContentsView
{
    Q_OBJECT

public:
    AudioCdView( QWidget* parent = 0 );
    ~AudioCdView();

    KActionCollection* actionCollection() const { return m_actionCollection; }

public Q_SLOTS:
    void queryCddb();

private Q_SLOTS:
    void slotContextMenu( const QPoint& );
    void slotTrackSelectionChanged();
    void slotSaveCddbLocally();

    void slotEditTrackCddb();
    void slotEditAlbumCddb();
    void startRip();
    void slotSelect();
    void slotDeselect();

private:
    void reloadMedium();

    void initActions();
    void enableInteraction( bool );
    void showBusyLabel( bool );
    void updateTitle();

    KActionCollection* m_actionCollection;
    KMenu* m_popupMenu;

    K3b::AudioTrackModel* m_trackModel;
    QTreeView* m_trackView;
    KToolBar* m_toolBox;
    QLabel* m_labelLength;

    
    QLabel* m_busyInfoLabel;
};
}


#endif
