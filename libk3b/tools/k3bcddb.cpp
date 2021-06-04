/*
    SPDX-FileCopyrightText: 2008-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bcddb.h"
#include "k3bmedium.h"
#include "k3btoc.h"
#include "k3b_i18n.h"

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>

#include <KCddb/Client>


KCDDB::TrackOffsetList K3b::CDDB::createTrackOffsetList( const K3b::Device::Toc& toc )
{
    KCDDB::TrackOffsetList trackOffsets;
    foreach( const K3b::Device::Track& track, toc ) {
        trackOffsets.append( track.firstSector().lba() + 150 );
    }
    trackOffsets.append( toc.length().lba() + 150 );
    return trackOffsets;
}


int K3b::CDDB::MultiEntriesDialog::selectCddbEntry( const KCDDB::CDInfoList& entries, QWidget* parent )
{
    QDialog dialog( parent );
    dialog.setWindowTitle( i18n("Multiple CDDB Entries Found") );

    QLabel* infoLabel = new QLabel( i18n("K3b found multiple or inexact CDDB entries. Please select one."), &dialog );
    infoLabel->setWordWrap( true );

    QListWidget* listBox = new QListWidget( &dialog );
    listBox->setSelectionMode( QAbstractItemView::SingleSelection );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog );
    QObject::connect( buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()) );
    QObject::connect( buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()) );

    QVBoxLayout* layout = new QVBoxLayout( &dialog );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( infoLabel );
    layout->addWidget( listBox );
    layout->addWidget( buttonBox );

    dialog.setMinimumSize( 280, 200 );

    int i = 1;
    foreach( const KCDDB::CDInfo& info, entries ) {
        listBox->addItem( QString::number(i++) + ' ' +
                              info.get( KCDDB::Artist ).toString() + " - " +
                              info.get( KCDDB::Title ).toString() + " (" +
                              info.get( KCDDB::Category ).toString() + ')' );
    }

    listBox->setCurrentRow( 0 );

    if( dialog.exec() == QDialog::Accepted )
        return listBox->currentRow();
    else
        return -1;
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
    qDebug();
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

#include "moc_k3bcddb.cpp"
