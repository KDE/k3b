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


#include "k3bmovixjob.h"
#include "k3bmovixdoc.h"
#include "k3bmovixfileitem.h"
#include "k3bmovixdocpreparer.h"

#include "k3bcore.h"
#include "k3bdatajob.h"
#include "k3bdevice.h"
#include "k3bisooptions.h"

#include <klocale.h>
#include <kdebug.h>


K3b::MovixJob::MovixJob( K3b::MovixDoc* doc, K3b::JobHandler* jh, QObject* parent )
    : K3b::BurnJob( jh, parent ),
      m_doc(doc)
{
    m_dataJob = new K3b::DataJob( doc, this, this );
    m_movixDocPreparer = new K3b::MovixDocPreparer( doc, this, this );

    // pipe signals
    connect( m_dataJob, SIGNAL(percent(int)), this, SIGNAL(percent(int)) );
    connect( m_dataJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
    connect( m_dataJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
    connect( m_dataJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
    connect( m_dataJob, SIGNAL(bufferStatus(int)), this, SIGNAL(bufferStatus(int)) );
    connect( m_dataJob, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
    connect( m_dataJob, SIGNAL(writeSpeed(int, K3b::Device::SpeedMultiplicator)), this, SIGNAL(writeSpeed(int, K3b::Device::SpeedMultiplicator)) );
    connect( m_dataJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
    connect( m_dataJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_dataJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
             this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
    connect( m_dataJob, SIGNAL(infoMessage(const QString&, int)),
             this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_dataJob, SIGNAL(burning(bool)), this, SIGNAL(burning(bool)) );

    // we need to clean up here
    connect( m_dataJob, SIGNAL(finished(bool)), this, SLOT(slotDataJobFinished(bool)) );

    connect( m_movixDocPreparer, SIGNAL(infoMessage(const QString&, int)),
             this, SIGNAL(infoMessage(const QString&, int)) );
}


K3b::MovixJob::~MovixJob()
{
}


K3b::Device::Device* K3b::MovixJob::writer() const
{
    return m_dataJob->writer();
}


K3b::Doc* K3b::MovixJob::doc() const
{
    return m_doc;
}


void K3b::MovixJob::start()
{
    jobStarted();

    m_canceled = false;
    m_dataJob->setWritingApp( writingApp() );

    if( m_movixDocPreparer->createMovixStructures() ) {
        m_dataJob->start();
    }
    else {
        m_movixDocPreparer->removeMovixStructures();
        jobFinished(false);
    }
}


void K3b::MovixJob::cancel()
{
    m_canceled = true;
    m_dataJob->cancel();
}


void K3b::MovixJob::slotDataJobFinished( bool success )
{
    m_movixDocPreparer->removeMovixStructures();

    if( m_canceled || m_dataJob->hasBeenCanceled() )
        emit canceled();

    jobFinished( success );
}


QString K3b::MovixJob::jobDescription() const
{
    if( m_doc->isoOptions().volumeID().isEmpty() )
        return i18n("Writing eMovix Project");
    else
        return i18n("Writing eMovix Project (%1)",m_doc->isoOptions().volumeID());
}


QString K3b::MovixJob::jobDetails() const
{
    return ( i18np("One file (%2) and about 8 MB eMovix data",
                   "%1 files (%2) and about 8 MB eMovix data",
                   m_doc->movixFileItems().count(), KIO::convertSize(m_doc->size()))
             + ( m_doc->copies() > 1
                 ? i18np(" – One copy", " – %1 copies", m_doc->copies())
                 : QString() ) );
}

#include "k3bmovixjob.moc"
