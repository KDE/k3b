/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudionormalizejob.h"
#include "k3bexternalbinmanager.h"
#include "k3bprocess.h"
#include "k3bcore.h"
#include "k3b_i18n.h"

#include <QDebug>


K3b::AudioNormalizeJob::AudioNormalizeJob( K3b::JobHandler* hdl, QObject* parent )
    : K3b::Job( hdl, parent ),
      m_process(0)
{
}


K3b::AudioNormalizeJob::~AudioNormalizeJob()
{
    delete m_process;
}


void K3b::AudioNormalizeJob::start()
{
    m_canceled = false;
    m_currentAction = COMPUTING_LEVELS;
    m_currentTrack = 1;

    jobStarted();

    if( m_process )
        delete m_process;

    m_process = new K3b::Process();
    connect( m_process, SIGNAL(stderrLine(QString)), this, SLOT(slotStdLine(QString)) );
    connect( m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotProcessExited(int,QProcess::ExitStatus)) );

    const K3b::ExternalBin* bin = k3bcore->externalBinManager()->binObject( "normalize" );

    if( !bin ) {
        emit infoMessage( i18n("Could not find normalize executable."), MessageError );
        jobFinished(false);
        return;
    }

    if( !bin->copyright().isEmpty() )
        emit infoMessage( i18n("Using %1 %2 – Copyright © %3",bin->name(),bin->version(),bin->copyright()), MessageInfo );

    // create the commandline
    *m_process << bin;

    // additional user parameters from config
    const QStringList& params = bin->userParameters();
    for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
        *m_process << *it;

    // end the options
    *m_process << "--";

    // add the files
    for( int i = 0; i < m_files.count(); ++i )
        *m_process << m_files[i];

    // now start the process
    if( !m_process->start( KProcess::OnlyStderrChannel ) ) {
        // something went wrong when starting the program
        // it "should" be the executable
        qDebug() << "(K3b::AudioNormalizeJob) could not start normalize";
        emit infoMessage( i18n("Could not start normalize."), K3b::Job::MessageError );
        jobFinished(false);
    }
}


void K3b::AudioNormalizeJob::cancel()
{
    m_canceled = true;

    if( m_process )
        if( m_process->isRunning() ) {
            m_process->kill();
        }
}


void K3b::AudioNormalizeJob::slotStdLine( const QString& line )
{
    // percent, subPercent, newTask (compute level and adjust)

    //  emit newSubTask( i18n("Normalizing track %1 of %2 (%3)",t,tt,m_files.at(t-1)) );

    emit debuggingOutput( "normalize", line );

    // wenn "% done" drin:
    //    wenn ein --% drin ist, so beginnt ein neuer track
    //    sonst prozent parsen "batch xxx" ist der fortschritt der action
    //                         also ev. den batch fortschritt * 1/2

    if( line.startsWith( "Applying adjustment" ) ) {
        if( m_currentAction == COMPUTING_LEVELS ) {
            // starting the adjustment with track 1
            m_currentTrack = 1;
            m_currentAction = ADJUSTING_LEVELS;
        }
    }

    else if( line.contains( "already normalized" ) ) {
        // no normalization necessary for the current track
        emit infoMessage( i18n("Track %1 is already normalized.",m_currentTrack), MessageInfo );
        m_currentTrack++;
    }

    else if( line.contains( "--% done") ) {
        if( m_currentAction == ADJUSTING_LEVELS ) {
            emit newTask( i18n("Adjusting volume level for track %1 of %2",m_currentTrack,m_files.count()) );
            qDebug() << "(K3b::AudioNormalizeJob) adjusting level for track "
                     << m_currentTrack
                     << " "
                     << m_files.at(m_currentTrack-1)
                     << Qt::endl;
        }
        else {
            emit newTask( i18n("Computing level for track %1 of %2",m_currentTrack,m_files.count()) );
            qDebug() << "(K3b::AudioNormalizeJob) computing level for track "
                     << m_currentTrack
                     << " "
                     << m_files.at(m_currentTrack-1)
                     << Qt::endl;
        }

        m_currentTrack++;
    }

    else if( int pos = line.indexOf( "% done" ) > 0 ) {
        // parse progress: "XXX% done" and "batch XXX% done"
        pos -= 3;
        bool ok;
        // TODO: do not use fixed values
        // track progress starts at position 19 in version 0.7.6
        int p = line.mid( 19, 3 ).toInt(&ok);
        if( ok )
            emit subPercent( p );
        else
            qDebug() << "(K3b::AudioNormalizeJob) subPercent parsing error at pos "
                     << 19 << " in line '" << line.mid( 19, 3 ) << "'" << Qt::endl;

        // batch progress starts at position 50 in version 0.7.6
        p = line.mid( 50, 3 ).toInt(&ok);
        if( ok && m_currentAction == COMPUTING_LEVELS )
            emit percent( int(double(p)/2.0) );
        else if( ok && m_currentAction == ADJUSTING_LEVELS )
            emit percent( 50 + int(double(p)/2.0) );
        else
            qDebug() << "(K3b::AudioNormalizeJob) percent parsing error at pos "
                     << 50 << " in line '" << line.mid( 50, 3 ) << "'" << Qt::endl;

    }
}


void K3b::AudioNormalizeJob::slotProcessExited( int exitCode, QProcess::ExitStatus exitStatus )
{
    if( exitStatus == QProcess::NormalExit ) {
        switch( exitCode ) {
        case 0:
            emit infoMessage( i18n("Successfully normalized all tracks."), MessageSuccess );
            jobFinished(true);
            break;
        default:
            if( !m_canceled ) {
                emit infoMessage( i18n("%1 returned an unknown error (code %2).",QString("normalize"), exitCode),
                                  K3b::Job::MessageError );
                emit infoMessage( i18n("Please send me an email with the last output."), K3b::Job::MessageError );
                emit infoMessage( i18n("Error while normalizing tracks."), MessageError );
            }
            else
                emit canceled();
            jobFinished(false);
            break;
        }
    }
    else {
        emit infoMessage( i18n("%1 did not exit cleanly.",QString("Normalize")), K3b::Job::MessageError );
        jobFinished( false );
    }
}

#include "moc_k3baudionormalizejob.cpp"
