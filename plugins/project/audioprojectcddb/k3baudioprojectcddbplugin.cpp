/*
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

#include "k3baudioprojectcddbplugin.h"

#include <config-k3b.h>

// the k3b stuff we need
#include <k3bcore.h>
#include <k3bglobals.h>
#include <k3baudiodoc.h>
#include <k3baudiotrack.h>
#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bmsf.h>
#include <k3bprogressdialog.h>
#include <k3bcddb.h>

#include <kdebug.h>
#include <kaction.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kconfig.h>

#include <qstring.h>

#include <libkcddb/cdinfo.h>


K3B_EXPORT_PLUGIN( k3baudioprojectcddbplugin, K3bAudioProjectCddbPlugin )


K3bAudioProjectCddbPlugin::K3bAudioProjectCddbPlugin( QObject* parent, const QVariantList& )
    : K3bProjectPlugin( AUDIO_CD, false, parent ),
      m_progress(0)
{
    setText( i18n("Query Cddb") );
    setToolTip( i18n("Query a cddb entry for the current audio project.") );
}


K3bAudioProjectCddbPlugin::~K3bAudioProjectCddbPlugin()
{
    delete m_progress;
}


void K3bAudioProjectCddbPlugin::activate( K3bDoc* doc, QWidget* parent )
{
    m_doc = dynamic_cast<K3bAudioDoc*>( doc );
    m_parentWidget = parent;
    m_canceled = false;

    if( !m_doc || m_doc->numOfTracks() == 0 ) {
        KMessageBox::sorry( parent, i18n("Please select a non-empty audio project for a cddb query.") );
    }
    else {
        if( !m_progress ) {
            m_progress = new K3bProgressDialog( i18n("Query Cddb"), parent, i18n("Audio Project") );
            connect( m_progress, SIGNAL(cancelClicked()),
                     this, SLOT(slotCancelClicked()) );
        }

        K3bCDDB::CDDBJob* job = K3bCDDB::CDDBJob::queryCddb( m_doc->toToc() );
        connect( job, SIGNAL( result( KJob* ) ),
                 this, SLOT( slotCddbQueryFinished( KJob* ) ) );

        m_progress->exec(false);
    }
}


void K3bAudioProjectCddbPlugin::slotCancelClicked()
{
    m_canceled = true;
    m_progress->close();
}


void K3bAudioProjectCddbPlugin::slotCddbQueryFinished( KJob* job )
{
    if( !m_canceled ) {
        m_progress->hide();

        if( !job->error() ) {
            K3bCDDB::CDDBJob* cddbJob = dynamic_cast<K3bCDDB::CDDBJob*>( job );
            KCDDB::CDInfo cddbInfo = cddbJob->cddbResult();

            // save the entry to the doc
            m_doc->setTitle( cddbInfo.get( KCDDB::Title ).toString() );
            m_doc->setPerformer( cddbInfo.get( KCDDB::Artist ).toString() );
            m_doc->setCdTextMessage( cddbInfo.get( KCDDB::Comment ).toString() );

            int i = 0;
            for( K3bAudioTrack* track = m_doc->firstTrack(); track; track = track->next() ) {
                KCDDB::TrackInfo info = cddbInfo.track( i );
                track->setTitle( info.get( KCDDB::Title ).toString() );
                track->setPerformer( info.get( KCDDB::Artist ).toString() );
                track->setCdTextMessage( info.get( KCDDB::Comment ).toString() );

                ++i;
            }

            // and enable cd-text
            m_doc->writeCdText( true );
        }
        else {
            KMessageBox::information( m_parentWidget, job->errorString(), i18n("Cddb error") );
        }
    }

    // make sure the progress dialog does not get deleted by it's parent
    delete m_progress;
    m_doc = 0;
    m_parentWidget = 0;
    m_progress = 0;
}

#include "k3baudioprojectcddbplugin.moc"
