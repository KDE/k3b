/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdatamultisessionimportdialog.h"
#include "k3bcore.h"
#include "k3bdatadoc.h"
#include "k3btoc.h"
#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bdiskinfo.h"
#include "k3biso9660.h"
#include "k3bmedium.h"
#include "k3bmediacache.h"

#include "../k3bapplication.h"
#include "../k3b.h"

#include <KIconLoader>
#include <KLocalizedString>
#include <KMessageBox>

#include <QMap>
#include <QCursor>
#include <QFont>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {
    class SessionInfo
    {
    public:
        SessionInfo()
            : sessionNumber( 0 ),
              device( 0 ) {}

        SessionInfo( int num, K3b::Device::Device* dev )
            : sessionNumber( num ),
              device( dev ) {}

        int sessionNumber;
        K3b::Device::Device* device;
    };

    typedef QMap<QTreeWidgetItem*, SessionInfo> Sessions;
}


class K3b::DataMultisessionImportDialog::Private
{
public:
    K3b::DataDoc* doc;
    QTreeWidget* sessionView;
    QPushButton* okButton;

    Sessions sessions;
};


K3b::DataDoc* K3b::DataMultisessionImportDialog::importSession( K3b::DataDoc* doc, QWidget* parent )
{
    K3b::DataMultisessionImportDialog dlg( parent );
    dlg.importSession( doc );
    dlg.exec();
    return dlg.d->doc;
}


void K3b::DataMultisessionImportDialog::slotOk()
{
    Sessions::const_iterator session = d->sessions.constFind( d->sessionView->currentItem() );
    if ( session != d->sessions.constEnd() ) {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

        K3b::Device::Device* dev = session->device;

        //
        // Mkisofs does not properly import joliet filenames from an old session
        //
        // See bug 79215 for details
        //
        K3b::Iso9660 iso( dev );
        if( iso.open() ) {
            if( iso.firstRRDirEntry() == 0 && iso.jolietLevel() > 0 )
                KMessageBox::sorry( this,
                                    i18n("<p>K3b found session containing Joliet information for long filenames "
                                         "but no Rock Ridge extensions."
                                         "<p>The filenames in the imported session will be converted to a restricted "
                                         "character set in the new session. This character set is based on the ISO 9660 "
                                         "settings in the K3b project. K3b is not able to display these converted filenames yet."),
                                    i18n("Session Import Warning") );
            iso.close();
        }

        if( !d->doc ) {
            d->doc = static_cast<K3b::DataDoc*>( k3bappcore->k3bMainWindow()->slotNewDataDoc() );
        }

        d->doc->setBurner( dev );
        d->doc->importSession( dev, session->sessionNumber );

        QApplication::restoreOverrideCursor();

        done( 0 );
    }
}


void K3b::DataMultisessionImportDialog::slotCancel()
{
    reject();
}


void K3b::DataMultisessionImportDialog::importSession( K3b::DataDoc* doc )
{
    d->doc = doc;
    updateMedia();
    slotSelectionChanged();
}


void K3b::DataMultisessionImportDialog::updateMedia()
{
    d->sessionView->clear();
    d->sessions.clear();

    QList<K3b::Device::Device*> devices = k3bcore->deviceManager()->allDevices();

    bool haveMedium = false;
    for( QList<K3b::Device::Device *>::const_iterator it = devices.constBegin();
         it != devices.constEnd(); ++it ) {
        K3b::Medium medium = k3bappcore->mediaCache()->medium( *it );

        if ( medium.diskInfo().mediaType() & K3b::Device::MEDIA_WRITABLE &&
             medium.diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE ) {
            addMedium( medium );
            haveMedium = true;
        }
        else if ( !medium.diskInfo().empty() &&
                  medium.diskInfo().mediaType() & ( K3b::Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_RW_OVWR ) ) {
            addMedium( medium );
            haveMedium = true;
        }
    }

    if ( !haveMedium ) {
        QTreeWidgetItem* noMediaItem = new QTreeWidgetItem( d->sessionView );
        QFont fnt( noMediaItem->font(0) );
        fnt.setItalic( true );
        noMediaItem->setText( 0, i18n( "Please insert an appendable medium" ) );
        noMediaItem->setFont( 0, fnt );
    }
    else if( QTreeWidgetItem* firstMedium = d->sessionView->topLevelItem(0) ) {
        if( firstMedium->childCount() > 0 )
            d->sessionView->setCurrentItem( firstMedium->child( firstMedium->childCount()-1 ) );
        else
            d->sessionView->setCurrentItem( firstMedium );
    }

    d->sessionView->setEnabled( haveMedium );
}


void K3b::DataMultisessionImportDialog::addMedium( const K3b::Medium& medium )
{
    QTreeWidgetItem* mediumItem = new QTreeWidgetItem( d->sessionView );
    QFont fnt( mediumItem->font(0) );
    fnt.setBold( true );
    mediumItem->setText( 0, medium.shortString() );
    mediumItem->setFont( 0, fnt );
    mediumItem->setIcon( 0, QIcon::fromTheme("media-optical-recordable") );

    const K3b::Device::Toc& toc = medium.toc();
    QTreeWidgetItem* sessionItem = 0;
    int lastSession = 0;
    for ( K3b::Device::Toc::const_iterator it = toc.begin(); it != toc.end(); ++it ) {
        const K3b::Device::Track& track = *it;

        if( track.session() != lastSession ) {
            lastSession = track.session();
            QString sessionInfo;
            if ( track.type() == K3b::Device::Track::TYPE_DATA ) {
                K3b::Iso9660 iso( medium.device(), track.firstSector().lba() );
                if ( iso.open() ) {
                    sessionInfo = iso.primaryDescriptor().volumeId;
                }
            }
            else {
                int numAudioTracks = 1;
                while ( it != toc.end()
                        && ( *it ).type() == K3b::Device::Track::TYPE_AUDIO
                        && ( *it ).session() == lastSession ) {
                    ++it;
                    ++numAudioTracks;
                }
                --it;
                sessionInfo = i18np("1 audio track", "%1 audio tracks", numAudioTracks );
            }

            sessionItem = new QTreeWidgetItem( mediumItem, sessionItem );
            sessionItem->setText( 0, i18n( "Session %1", lastSession )
                                     + ( sessionInfo.isEmpty() ? QString() : " (" + sessionInfo + ')' ) );
            if ( track.type() == K3b::Device::Track::TYPE_AUDIO )
                sessionItem->setIcon( 0, QIcon::fromTheme( "audio-x-generic" ) );
            else
                sessionItem->setIcon( 0, QIcon::fromTheme( "application-x-tar" ) );

            d->sessions.insert( sessionItem, SessionInfo( lastSession, medium.device() ) );
        }
    }

    if( 0 == lastSession ) {
        // the medium item in case we have no session info (will always use the last session)
        d->sessions.insert( mediumItem, SessionInfo( 0, medium.device() ) );
    }
    else {
        // we have a session item, there is no need to select the medium as a whole
        mediumItem->setFlags( mediumItem->flags() ^ Qt::ItemIsSelectable );
    }

    mediumItem->setExpanded( true );
}


void K3b::DataMultisessionImportDialog::slotSelectionChanged()
{
    Sessions::const_iterator session = d->sessions.constFind( d->sessionView->currentItem() );
    if ( session != d->sessions.constEnd() ) {
        d->okButton->setEnabled( true );
    }
    else {
        d->okButton->setEnabled( false );
    }
}


K3b::DataMultisessionImportDialog::DataMultisessionImportDialog( QWidget* parent )
    : QDialog( parent),
      d( new Private() )
{
    setModal(true);
    setWindowTitle(i18n("Session Import"));
    QVBoxLayout* layout = new QVBoxLayout( this );

    QLabel* label = new QLabel( i18n( "Please select a session to import." ), this );
    d->sessionView = new QTreeWidget( this );
    d->sessionView->setHeaderHidden( true );
    d->sessionView->setItemsExpandable( false );
    d->sessionView->setRootIsDecorated( false );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
    d->okButton = buttonBox->button( QDialogButtonBox::Ok );
    connect( buttonBox, SIGNAL(accepted()), SLOT(accept()) );
    connect( buttonBox, SIGNAL(rejected()), SLOT(reject()) );

    layout->addWidget( label );
    layout->addWidget( d->sessionView );
    layout->addWidget( buttonBox );

    connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3b::Device::Device*)),
             this, SLOT(updateMedia()) );
    connect( d->sessionView, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
             this, SLOT(slotSelectionChanged()) );
    connect(d->sessionView, SIGNAL(itemActivated(QTreeWidgetItem*,int)), SLOT(slotOk()) );
    connect(d->okButton, SIGNAL(clicked()), this, SLOT(slotOk()));
    connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(slotCancel()));
}


K3b::DataMultisessionImportDialog::~DataMultisessionImportDialog()
{
    delete d;
}


