/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_URL_NAVIGATOR_H_
#define _K3B_URL_NAVIGATOR_H_

#include <KUrlNavigator>
#include <QUrl>

namespace K3b {
    namespace Device {
        class Device;
    }

    class UrlNavigator : public KUrlNavigator
    {
        Q_OBJECT

    public:
        explicit UrlNavigator( KFilePlacesModel* model, QWidget* parent = nullptr );
        ~UrlNavigator() override;

    public Q_SLOTS:
        void setDevice( K3b::Device::Device* );

    Q_SIGNALS:
        void activated( const QUrl& url );
        void activated( K3b::Device::Device* dev );

    private Q_SLOTS:
        void urlActivated( const QUrl &url );

    };
}

#endif
