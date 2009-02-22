/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3BDIRVIEW_H
#define K3BDIRVIEW_H

#include <k3bmedium.h>
#include <KVBox>

namespace K3b {
    class AudioCdView;
}
namespace K3b {
    class DiskInfoView;
}
namespace K3b {
    class FileTreeView;
}
namespace K3b {
    class FileView;
}
namespace K3b {
    class VideoCdView;
}
namespace K3b {
    class VideoDVDRippingView;
}
class KComboBox;
class KConfigGroup;
class KUrl;
class QSplitter;
class QStackedWidget;
namespace Device {
    class Device;
}

/**
 *@author Sebastian Trueg
 */
namespace K3b {
class DirView : public KVBox
{
    Q_OBJECT

public:
    DirView(FileTreeView* tree, QWidget *parent=0);
    ~DirView();

public Q_SLOTS:
    void saveConfig( KConfigGroup grp );
    void readConfig( const KConfigGroup & grp );
    void showUrl( const KUrl& );
    void showDevice( Device::Device* );
    void showDiskInfo( Device::Device* );

protected Q_SLOTS:
    void slotDirActivated( const KUrl& );
    void slotDirActivated( const QString& );
    void slotMountFinished( const QString& );
    void slotUnmountFinished( bool );
    void showMediumInfo( const Medium& );
    void home();

Q_SIGNALS:
    void urlEntered( const KUrl& );
    void deviceSelected( Device::Device* );

private:
    QStackedWidget* m_viewStack;

    AudioCdView*   m_cdView;
    VideoCdView*   m_videoView;
    VideoDVDRippingView* m_movieView;
    FileView* m_fileView;
    DiskInfoView* m_infoView;

    KComboBox* m_urlCombo;
    QSplitter* m_mainSplitter;
    FileTreeView* m_fileTreeView;

    bool m_bViewDiskInfo;

    class Private;
    Private* d;
};
}

#endif
