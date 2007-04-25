/*
 *
 * $Id: k3bdatajob.cpp 657635 2007-04-24 16:55:42Z trueg $
 * Copyright (C) 2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdatamultisessionparameterjob.h"

#include <k3bthread.h>
#include <k3biso9660.h>
#include <k3bdevice.h>
#include <k3bdiskinfo.h>
#include <k3bdevicetypes.h>
#include <k3bglobals.h>
#include <k3btoc.h>
#include <k3btrack.h>

#include "k3bdatadoc.h"

#include <klocale.h>


class K3bDataMultiSessionParameterJob::WorkThread : public K3bThread
{
public:
    WorkThread( K3bDataDoc* doc, K3bDataMultiSessionParameterJob* parent );

    K3bDataDoc::MultiSessionMode usedMultiSessionMode() const;

protected:
    void run();

private:
    K3bDataDoc::MultiSessionMode determineMultiSessionModeFromMedium();
    bool setupMultiSessionParameters();

    K3bDataDoc* m_doc;

public:
    K3bDataDoc::MultiSessionMode m_usedMultiSessionMode;
    unsigned int m_previousSessionStart;
    unsigned int m_nextSessionStart;
    // true if the last session should be imported into the filesystem or not
    bool m_importSession;
};



K3bDataMultiSessionParameterJob::WorkThread::WorkThread( K3bDataDoc* doc, K3bDataMultiSessionParameterJob* job )
    : K3bThread( job ),
      m_doc( doc )
{
}


K3bDataDoc::MultiSessionMode K3bDataMultiSessionParameterJob::WorkThread::usedMultiSessionMode() const
{
    return m_usedMultiSessionMode;
}


void K3bDataMultiSessionParameterJob::WorkThread::run()
{
    emitStarted();

    m_usedMultiSessionMode = m_doc->multiSessionMode();

    if ( m_usedMultiSessionMode == K3bDataDoc::AUTO ) {
        if( m_doc->writingMode() == K3b::WRITING_MODE_AUTO ||
            m_doc->writingMode() == K3b::TAO ) {
            emitNewSubTask( i18n("Searching for old session") );

            //
            // Wait for the medium.
            // In case an old session was imported we always want to continue or finish a multisession CD/DVD.
            // Otherwise we wait for everything we could handle and decide what to do in
            // determineMultiSessionMode( K3bDevice::DeviceHandler* ) below.
            //

            int wantedMediaState = K3bDevice::STATE_INCOMPLETE|K3bDevice::STATE_EMPTY;
            if( m_doc->importedSession() >= 0 )
                wantedMediaState = K3bDevice::STATE_INCOMPLETE;

            int m = waitForMedia( m_doc->burner(),
                                  wantedMediaState,
                                  K3bDevice::MEDIA_WRITABLE );

            if( m < 0 ) {
                emitCanceled();
                emitFinished( false );
                return;
            }
            else {
                m_usedMultiSessionMode = determineMultiSessionModeFromMedium();
            }
        }
        else {
            // we need TAO for multisession
            m_usedMultiSessionMode = K3bDataDoc::NONE;
        }
    }

    if( !m_doc->onlyCreateImages() &&
        ( m_usedMultiSessionMode == K3bDataDoc::CONTINUE ||
          m_usedMultiSessionMode == K3bDataDoc::FINISH ) ) {
        if ( !setupMultiSessionParameters() ) {
            emitFinished( false );
            return;
        }
    }

    emitFinished( true );
}


K3bDataDoc::MultiSessionMode K3bDataMultiSessionParameterJob::WorkThread::determineMultiSessionModeFromMedium()
{
    K3bDevice::DiskInfo info = m_doc->burner()->diskInfo();

    if( info.mediaType() & (K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_RW_OVWR) ) {
        //
        // we need to handle DVD+RW and DVD-RW overwrite media differently since remainingSize() is not valid
        // in both cases
        // Since one never closes a DVD+RW we only differ between CONTINUE and START
        //

        // try to check the filesystem size
        K3bIso9660 iso( m_doc->burner() );
        if( iso.open() && info.capacity() - iso.primaryDescriptor().volumeSpaceSize >= m_doc->burningLength() ) {
            return K3bDataDoc::CONTINUE;
        }
        else {
            return K3bDataDoc::START;
        }
    }

    else if( info.appendable() ) {
        //
        //  1. the project does not fit -> no multisession (resulting in asking for another media above)
        //     Exception: a session was imported.
        //  2. the project does fit and fills up the medium to some arbitrary percentage -> finish multisession
        //  3. Special case for the 4GB boundary which seems to be enforced by a linux kernel issue
        //  4. the project does fit and does not fill up the CD -> continue multisession
        //
        if( m_doc->size() > info.remainingSize().mode1Bytes() &&
            m_doc->importedSession() < 0 ) {
            return K3bDataDoc::NONE;
        }
        else if( m_doc->size() >= info.remainingSize().mode1Bytes()*9/10 ) {
            return K3bDataDoc::FINISH;
        }
        else if( info.capacity() < 2621440 /* ~ 5 GB */ &&
                 info.size() + m_doc->burningLength() + 11400 /* used size + project size + session gap */ > 2097152 /* 4 GB */ ) {
            return K3bDataDoc::FINISH;
        }
        else {
            return K3bDataDoc::CONTINUE;
        }
    }

    else { // empty and complete rewritable media
        //
        // 1. the project does fit and fills up the medium to some arbitrary percentage -> finish multisession
        // 2. Special case for the 4GB boundary which seems to be enforced by a linux kernel issue
        //
        if( m_doc->size() >= info.capacity().mode1Bytes()*9/10 ||
            m_doc->writingMode() == K3b::DAO ) {
            return K3bDataDoc::NONE;
        }
        else if( ( info.capacity() < 2621440 /* ~ 5 GB */ &&
                   m_doc->size() + 11400 /* used size + project size + session gap */ > 2097152 /* 4 GB */ ) ||
                 m_doc->writingMode() == K3b::DAO ) {
            return K3bDataDoc::NONE;
        }
        else {
            return K3bDataDoc::START;
        }
    }
}


bool K3bDataMultiSessionParameterJob::WorkThread::setupMultiSessionParameters()
{
    K3bDevice::DiskInfo info = m_doc->burner()->diskInfo();
    K3bDevice::Toc toc = m_doc->burner()->readToc();

    //
    // determine the multisession import info
    //
    unsigned long lastSessionStart = 0;
    unsigned long nextSessionStart = 0;
    m_importSession = true;
    if( info.mediaType() & (K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_RW_OVWR) ) {
        lastSessionStart = 0;

        // get info from iso filesystem
        K3bIso9660 iso( m_doc->burner(), toc.last().firstSector().lba() );
        if( iso.open() ) {
            nextSessionStart = iso.primaryDescriptor().volumeSpaceSize;
        }
        else {
            emitInfoMessage( i18n("Could not open Iso9660 filesystem in %1.")
                             .arg( m_doc->burner()->vendor() + " " + m_doc->burner()->description() ), K3bJob::ERROR );
            return false;
        }
    }
    else {
        nextSessionStart = m_doc->burner()->nextWritableAddress();
        lastSessionStart = toc.last().firstSector().lba();
        if ( m_doc->importedSession() > 0 ) {
            for ( K3bDevice::Toc::const_iterator it = toc.begin(); it != toc.end(); ++it ) {
                if ( ( *it ).session() == m_doc->importedSession() ) {
                    lastSessionStart = ( *it ).firstSector().lba();
                    if ( ( *it ).type() == K3bDevice::Track::AUDIO )
                        m_importSession = false;
                    break;
                }
            }
        }
    }

    if ( info.mediaType() & K3bDevice::MEDIA_DVD_ALL ) {
        // pad to closest 32K boundary
        nextSessionStart += 15;
        nextSessionStart /= 16;
        nextSessionStart *= 16;

        // growisofs does it. I actually do not know yet why.... :(
        lastSessionStart += 16;
    }

    m_previousSessionStart = lastSessionStart;
    m_nextSessionStart = nextSessionStart;

    return true;
}



K3bDataMultiSessionParameterJob::K3bDataMultiSessionParameterJob( K3bDataDoc* doc, K3bJobHandler* hdl, QObject* parent )
    : K3bThreadJob( hdl, parent ),
      m_thread( new WorkThread( doc, this ) )
{
    setThread( m_thread );
}


K3bDataMultiSessionParameterJob::~K3bDataMultiSessionParameterJob()
{
    delete m_thread;
}


K3bDataDoc::MultiSessionMode K3bDataMultiSessionParameterJob::usedMultiSessionMode() const
{
    return m_thread->m_usedMultiSessionMode;
}


unsigned int K3bDataMultiSessionParameterJob::previousSessionStart() const
{
    return m_thread->m_previousSessionStart;
}


unsigned int K3bDataMultiSessionParameterJob::nextSessionStart() const
{
    return m_thread->m_nextSessionStart;
}


bool K3bDataMultiSessionParameterJob::importPreviousSession() const
{
    return m_thread->m_importSession;
}

#include "k3bdatamultisessionparameterjob.moc"
