/*
 *
 * Copyright (C) 2008-2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bcddb.h"
#include "k3bmedium.h"

#include "k3btoc.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QLabel>
#include <QtGui/QApplication>

#include <KLocale>

#include <libkcddb/client.h>


KCDDB::TrackOffsetList K3b::CDDB::createTrackOffsetList( const K3b::Device::Toc& toc )
{
    KCDDB::TrackOffsetList trackOffsets;
    foreach( const K3b::Device::Track& track, toc ) {
        trackOffsets.append( track.firstSector().lba() + 150 );
    }
    trackOffsets.append( toc.length().lba() + 150 );
    return trackOffsets;
}


K3b::CDDB::MultiEntriesDialog::MultiEntriesDialog( QWidget* parent )
    : KDialog( parent )
{
    setCaption( i18n("Multiple CDDB Entries Found") );
    setButtons( Ok|Cancel );

    QWidget* frame = mainWidget();

    QLabel* infoLabel = new QLabel( i18n("K3b found multiple or inexact CDDB entries. Please select one."), frame );
    infoLabel->setWordWrap( true );

    m_listBox = new QListWidget( frame );
    m_listBox->setSelectionMode( QAbstractItemView::SingleSelection );

    QVBoxLayout* layout = new QVBoxLayout( frame );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( infoLabel );
    layout->addWidget( m_listBox );

    setMinimumSize( 280, 200 );
}


int K3b::CDDB::MultiEntriesDialog::selectCddbEntry( const KCDDB::CDInfoList& entries, QWidget* parent )
{
    MultiEntriesDialog d( parent );

    int i = 1;
    foreach( const KCDDB::CDInfo& info, entries ) {
        d.m_listBox->addItem( QString::number(i++) + ' ' +
                              info.get( KCDDB::Artist ).toString() + " - " +
                              info.get( KCDDB::Title ).toString() + " (" +
                              info.get( KCDDB::Category ).toString() + ')' );
    }

    d.m_listBox->setCurrentRow( 0 );

    if( d.exec() == QDialog::Accepted )
        return d.m_listBox->currentRow();
    else
        return -1;
}


K3b::CDDB::MultiEntriesDialog::~MultiEntriesDialog()
{
}



class K3b::CDDB::CDDBJob::Private
{
public:
    KCDDB::Client cddbClient;
    K3b::Medium medium;
    K3b::Device::Toc toc;

    KCDDB::CDInfo cddbInfo;

    void _k_cddbQueryFinished( KCDDB::Result );

    CDDBJob* q;
};


void K3b::CDDB::CDDBJob::Private::_k_cddbQueryFinished( KCDDB::Result result )
{
    if( result == KCDDB::Success ) {
        cddbInfo = cddbClient.lookupResponse().first();
    }
    else if ( result == KCDDB::MultipleRecordFound ) {
        KCDDB::CDInfoList results = cddbClient.lookupResponse();
        int i = K3b::CDDB::MultiEntriesDialog::selectCddbEntry( results, qApp->activeWindow() );
        if ( i >= 0 ) {
            cddbInfo = results[i];
        }
    }
    else {
        q->setError( KJob::UserDefinedError );
        q->setErrorText( KCDDB::resultToString( result ) );
    }

    // save the entry locally
    if ( cddbInfo.isValid() ) {
        cddbClient.store( cddbInfo, K3b::CDDB::createTrackOffsetList( toc ) );
    }

    q->emitResult();
}


K3b::CDDB::CDDBJob::CDDBJob( QObject* parent )
    : KJob( parent ),
      d( new Private() )
{
    d->q = this;
    d->cddbClient.setBlockingMode( false );
    connect( &d->cddbClient, SIGNAL(finished(KCDDB::Result)),
             this, SLOT(_k_cddbQueryFinished(KCDDB::Result)) );
}


K3b::CDDB::CDDBJob::~CDDBJob()
{
    delete d;
}


K3b::Medium K3b::CDDB::CDDBJob::medium() const
{
    return d->medium;
}


void K3b::CDDB::CDDBJob::start()
{
    kDebug();
    d->cddbInfo.clear();
    d->cddbClient.lookup( createTrackOffsetList( d->toc ) );
}


KCDDB::CDInfo K3b::CDDB::CDDBJob::cddbResult() const
{
    return d->cddbInfo;
}


K3b::CDDB::CDDBJob* K3b::CDDB::CDDBJob::queryCddb( const K3b::Medium& medium )
{
    CDDBJob* job = new CDDBJob();
    job->d->medium = medium;
    job->d->toc = medium.toc();
    // start async so callers can connect to signals
    QMetaObject::invokeMethod( job, "start", Qt::QueuedConnection );
    return job;
}


K3b::CDDB::CDDBJob* K3b::CDDB::CDDBJob::queryCddb( const K3b::Device::Toc& toc )
{
    CDDBJob* job = new CDDBJob();
    job->d->toc = toc;
    // start async so callers can connect to signals
    QMetaObject::invokeMethod( job, "start", Qt::QueuedConnection );
    return job;
}

#include "k3bcddb.moc"
