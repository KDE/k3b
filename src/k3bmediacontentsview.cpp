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

#include "k3bmediacontentsview.h"

#include "k3bmediacache.h"
#include "k3bapplication.h"

#include <QLabel>
#include <QLayout>


class K3b::MediaContentsView::Private
{
public:
    K3b::Medium medium;
    int supportedMediumContent;
    int supportedMediumTypes;
    int supportedMediumStates;

    bool autoReload;
};


K3b::MediaContentsView::MediaContentsView( bool withHeader,
                                           int mediumContent,
                                           int mediumTypes,
                                           int mediumState,
                                           QWidget* parent )
    : K3b::ContentsView( withHeader, parent )
{
    d = new Private;
    d->supportedMediumContent = mediumContent;
    d->supportedMediumTypes = mediumTypes;
    d->supportedMediumStates = mediumState;
    d->autoReload = true;

    connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3b::Device::Device*)),
             this, SLOT(slotMediumChanged(K3b::Device::Device*)) );
}


K3b::MediaContentsView::~MediaContentsView()
{
    delete d;
}


void K3b::MediaContentsView::setAutoReload( bool b )
{
    d->autoReload = b;
}


int K3b::MediaContentsView::supportedMediumContent() const
{
    return d->supportedMediumContent;
}


int K3b::MediaContentsView::supportedMediumTypes() const
{
    return d->supportedMediumTypes;
}


int K3b::MediaContentsView::supportedMediumStates() const
{
    return d->supportedMediumStates;
}


K3b::Medium K3b::MediaContentsView::medium() const
{
    return d->medium;
}


K3b::Device::Device* K3b::MediaContentsView::device() const
{
    return medium().device();
}


void K3b::MediaContentsView::setMedium( const K3b::Medium& m )
{
    d->medium = m;
}


void K3b::MediaContentsView::reload( K3b::Device::Device* dev )
{
    reload( k3bappcore->mediaCache()->medium( dev ) );
}


void K3b::MediaContentsView::reload( const K3b::Medium& m )
{
    setMedium( m );
    reload();
}


void K3b::MediaContentsView::reload()
{
    enableInteraction( true );
    reloadMedium();
}


void K3b::MediaContentsView::enableInteraction( bool enable )
{
    mainWidget()->setEnabled( enable );
}


void K3b::MediaContentsView::slotMediumChanged( K3b::Device::Device* dev )
{
    if( !d->autoReload || !isActive() )
        return;

    if( dev == device() ) {
        K3b::Medium m = k3bappcore->mediaCache()->medium( dev );

        // no need to reload if the medium did not change (this is even
        // important since K3b blocks the devices in action and after
        // the release they are signalled as changed)
        if( m == medium() ) {
            qDebug() << " medium did not change";
            enableInteraction( true );
        }
        else if( ( m.diskInfo().mediaType() == Device::MEDIA_NONE ||
                   m.content() & supportedMediumContent() ) &&
                 m.diskInfo().mediaType() & supportedMediumTypes() &&
                 m.diskInfo().diskState() & supportedMediumStates() ) {
            qDebug() << " new supported medium found";
            reload( m );
        }
        else {
            qDebug() << " unsupported medium found";
            enableInteraction( false );
        }
    }
}


