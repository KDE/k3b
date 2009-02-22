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


#include "k3bfiletreecombobox.h"
#include "k3bfiletreeview.h"

#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bcore.h>
#include <k3bglobals.h>

#include <qrect.h>
#include <qapplication.h>
#include <qstyle.h>
#include <q3listbox.h>
#include <q3header.h>
#include <qevent.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qdrawutil.h>
#include <qdir.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QPixmap>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDesktopWidget>
#include <QLineEdit>

#include <kdebug.h>
#include <kiconloader.h>
#include <kurlcompletion.h>
#include <kuser.h>


class K3b::FileTreeComboBox::Private
{
public:
    Private() {
        poppedUp = false;
        ignoreNextMouseClick = false;
    }
    bool poppedUp;
    bool ignoreNextMouseClick; // used when the view was hidden with the arrow button

    KUrlCompletion* urlCompletion;
};


K3b::FileTreeComboBox::FileTreeComboBox( QWidget* parent )
    : KComboBox( true, parent )
{
    d = new Private;

    d->urlCompletion = new KUrlCompletion();
    setCompletionObject( d->urlCompletion );

    m_fileTreeView = new K3b::FileTreeView( this );
    setView( m_fileTreeView );

    connect( m_fileTreeView, SIGNAL(activated(K3b::Device::Device*)),
             this, SLOT(slotDeviceExecuted(K3b::Device::Device*)) );
    connect( m_fileTreeView, SIGNAL(activated(const KUrl&)),
             this, SLOT(slotUrlExecuted(const KUrl&)) );

    connect( lineEdit(), SIGNAL(returnPressed()),
             this, SLOT(slotGoUrl()) );

    // TODO: subclass KURLCompletition to support the dev:/ stuff and block any non-local urls
}


K3b::FileTreeComboBox::~FileTreeComboBox()
{
    delete d->urlCompletion;
    delete d;
}


void K3b::FileTreeComboBox::slotDeviceExecuted( K3b::Device::Device* dev )
{
    setDevice( dev );
    emit activated( dev );
}


void K3b::FileTreeComboBox::slotUrlExecuted( const KUrl& url )
{
    setUrl( url );
    emit activated( url );
}


void K3b::FileTreeComboBox::setUrl( const KUrl& url )
{
    setEditText( K3b::convertToLocalUrl(url).path() );
}


void K3b::FileTreeComboBox::setDevice( K3b::Device::Device* dev )
{
    setEditText( dev->vendor() + " " + dev->description() + " (" + dev->blockDeviceName() + ")" );
}


void K3b::FileTreeComboBox::slotGoUrl()
{
    QString p = currentText();

    // check for a media url or a device string
    if( K3b::Device::Device* dev = K3b::urlToDevice( p ) ) {
        emit activated( dev );
        return;
    }

    // check for our own internal format
    else if( p.contains("/dev/") ) {
        int pos1 = p.lastIndexOf('(');
        int pos2 = p.lastIndexOf(')');
        QString devStr = p.mid( pos1+1, pos2-pos1-1  );
        if( K3b::Device::Device* dev = k3bcore->deviceManager()->findDevice( devStr ) ) {
            emit activated( dev );
            return;
        }
    }

    // no device -> select url

    //
    // Properly replace home dirs.
    // a single ~ will be replaced with the current user's home dir
    // while for example "~ftp" will be replaced by the home dir of user
    // ftp
    //
    // TODO: move this to k3bglobals

    // to expand another user's home dir we need a tilde followed by a user name
    static QRegExp someUsersHomeDir( "\\~([^/]+)" );
    int pos = 0;
    while( ( pos = someUsersHomeDir.indexIn( p, pos ) ) != -1 ) {
        KUser user( someUsersHomeDir.cap(1) );
        if( user.isValid() )
            p.replace( pos, someUsersHomeDir.cap(1).length() + 1, user.homeDir() );
        else
            ++pos; // skip this ~
    }

    // now replace the unmatched tildes with our home dir
    p.replace( "~", K3b::prepareDir( QDir::homePath() ) );


    lineEdit()->setText( p );
    KUrl url;
    url.setPath( p );
    emit activated( url );
}

#include "k3bfiletreecombobox.moc"

