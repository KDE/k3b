/*
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2011 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bkjobbridge.h"
#include "k3bjob.h"
#include "k3b_i18n.h"


namespace K3b
{
    
class KJobBridge::Private
{
public:
    Private( Job& j )
    :
        job( j )
    {
    }
    
    Job& job;
};

KJobBridge::KJobBridge( Job& job )
    : d( new Private( job ) )
{
    connect( &d->job, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)) );
    connect( &d->job, SIGNAL(infoMessage(QString,int)), this, SLOT(slotInfoMessage(QString,int)) );
    connect( &d->job, SIGNAL(percent(int)), this, SLOT(slotPercent(int)) );
    connect( &d->job, SIGNAL(processedSize(int,int)), this, SLOT(slotProcessedSize(int,int)) );
    connect( &d->job, SIGNAL(newTask(QString)), this, SLOT(slotNewTask(QString)) );
    
    setCapabilities( Killable );
}


KJobBridge::~KJobBridge()
{
}


void KJobBridge::start()
{
    d->job.start();
}


bool KJobBridge::doKill()
{
    d->job.cancel();
    return true;
}


void KJobBridge::slotFinished( bool success )
{
    if( success )
        setError( NoError );
    else if( d->job.hasBeenCanceled() )
        setError( KilledJobError );
    else
        setError( UserDefinedError );
    
    emitResult();
}


void KJobBridge::slotInfoMessage( const QString& message, int type )
{
    if( type == Job::MessageError )
        setErrorText( message );
    else if( type == Job::MessageWarning )
        emit warning( this, message );
}


void KJobBridge::slotPercent( int progress )
{
    setPercent( progress );
}


void KJobBridge::slotProcessedSize( int processed, int size )
{
    setTotalAmount( Bytes, static_cast<qulonglong>( size ) * 1024ULL * 1024ULL );
    setProcessedAmount( Bytes, static_cast<qulonglong>( processed ) * 1024ULL * 1024ULL );
}


void KJobBridge::slotNewTask( const QString& task )
{
    if( !d->job.jobSource().isEmpty() && !d->job.jobTarget().isEmpty() ) {
        emit description( this, task,
                        qMakePair( i18n( "Source" ), d->job.jobSource() ),
                        qMakePair( i18n( "Target" ), d->job.jobTarget() ) );
    } else {
        emit description( this, task );
    }
    emit infoMessage( this, task );
}

} // namespace K3b
