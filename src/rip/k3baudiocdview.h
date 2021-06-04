/*
    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010-2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
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
