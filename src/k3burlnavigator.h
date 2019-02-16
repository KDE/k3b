/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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
        explicit UrlNavigator( KFilePlacesModel* model, QWidget* parent = 0 );
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
