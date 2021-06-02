/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef K3BDIRVIEW_H
#define K3BDIRVIEW_H

#include "k3bmedium.h"
#include <QWidget>

class KConfigGroup;
class QUrl;

namespace K3b {
    class FileTreeView;
    namespace Device {
        class Device;
    }
    
    /**
     * @author Sebastian Trueg
     */
    class DirView : public QWidget
    {
        Q_OBJECT

    public:
        explicit DirView(FileTreeView* tree, QWidget *parent=0);
        ~DirView() override;

    public Q_SLOTS:
        void saveConfig( KConfigGroup grp );
        void readConfig( const KConfigGroup & grp );
        void showUrl( const QUrl& );
        void showDevice( K3b::Device::Device* );
        void showDiskInfo( K3b::Device::Device* );

    protected Q_SLOTS:
        void slotDirActivated( const QUrl& url );
        void slotMountFinished( const QString& );
        void slotUnmountFinished( bool );
        void showMediumInfo( const Medium& );
        void home();

    Q_SIGNALS:
        void urlEntered( const QUrl& );
        void deviceSelected( K3b::Device::Device* );

    private:
        class Private;
        Private* d;
    };
}

#endif
