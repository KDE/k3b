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

class K3bAudioCdView;
class K3bDiskInfoView;
class K3bFileTreeView;
class K3bFileView;
class K3bVideoCdView;
class K3bVideoDVDRippingView;
class KComboBox;
class KConfigGroup;
class KUrl;
class QSplitter;
class QStackedWidget;
namespace K3bDevice {
    class Device;
}

/**
 *@author Sebastian Trueg
 */
class K3bDirView : public KVBox
{
    Q_OBJECT

public:
    K3bDirView(K3bFileTreeView* tree, QWidget *parent=0);
    ~K3bDirView();

public Q_SLOTS:
    void saveConfig( KConfigGroup grp );
    void readConfig( const KConfigGroup & grp );
    void showUrl( const KUrl& );
    void showDevice( K3bDevice::Device* );
    void showDiskInfo( K3bDevice::Device* );

protected Q_SLOTS:
    void slotDirActivated( const KUrl& );
    void slotDirActivated( const QString& );
    void slotMountFinished( const QString& );
    void slotUnmountFinished( bool );
    void showMediumInfo( const K3bMedium& );
    void home();

Q_SIGNALS:
    void urlEntered( const KUrl& );
    void deviceSelected( K3bDevice::Device* );

private:
    QStackedWidget* m_viewStack;

    K3bAudioCdView*   m_cdView;
    K3bVideoCdView*   m_videoView;
    K3bVideoDVDRippingView* m_movieView;
    K3bFileView* m_fileView;
    K3bDiskInfoView* m_infoView;

    KComboBox* m_urlCombo;
    QSplitter* m_mainSplitter;
    K3bFileTreeView* m_fileTreeView;

    bool m_bViewDiskInfo;

    class Private;
    Private* d;
};

#endif
