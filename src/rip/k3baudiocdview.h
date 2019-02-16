/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010-2011 Michal Malek <michalm@jabster.pl>
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

#include "k3bmediacontentsview.h"

class QPoint;
class KActionCollection;


namespace K3b {
class AudioCdView : public MediaContentsView
{
    Q_OBJECT

public:
    explicit AudioCdView( QWidget* parent = 0 );
    ~AudioCdView() override;

    KActionCollection* actionCollection() const;

public Q_SLOTS:
    void loadCdInfo();
    void queryCddb();
    void readCdText();

protected:
    bool eventFilter( QObject* obj, QEvent* event ) override;

private Q_SLOTS:
    void slotContextMenu( const QPoint& );
    void slotContextMenuAboutToShow();
    void slotTrackSelectionChanged();
    void slotSaveCddbLocally();

    void slotEditTrackCddb();
    void slotEditAlbumCddb();
    void startRip();
    void slotCheck();
    void slotUncheck();
    void slotToggle();
    void slotShowDataPart();
    void slotCddbChanged( K3b::Device::Device* dev );

private:
    void reloadMedium() override;

    void initActions();
    void enableInteraction( bool ) override;
    void showBusyLabel( bool );
    void updateTitle();

    class Private;
    Private* d;
};
}


#endif
