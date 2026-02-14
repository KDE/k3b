/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3blsofwrapper.h"

#include "k3bdevice.h"
#include "k3bglobals.h"

#include <KProcess>

#include <QFile>
#include <QFileInfo>
#include <QList>

#include <sys/types.h>
#include <unistd.h>

static K3b::LsofWrapper::Process createProcess( const QString& name, int pid )
{
    K3b::LsofWrapper::Process p;
    p.name = name;
    p.pid = pid;
    return p;
}


class K3b::LsofWrapper::Private
{
public:
    QList<Process> apps;
    QString lsofBin;
};


K3b::LsofWrapper::LsofWrapper()
{
    d = new Private;
}


K3b::LsofWrapper::~LsofWrapper()
{
    delete d;
}


bool K3b::LsofWrapper::checkDevice( K3b::Device::Device* dev )
{
    d->apps.clear();

    if( !findLsofExecutable() )
        return false;

    // run lsof
    KProcess p;
    p.setOutputChannelMode( KProcess::OnlyStdoutChannel );

    //
    // We use the following output form:
    // p<PID>
    // c<COMMAND_NAME>
    //
    p << d->lsofBin << "-Fpc" << dev->blockDeviceName();
    p.start();

    if( !p.waitForFinished( -1 ) )
        return false;

    //
    // now process its output
    const QStringList l = QString::fromLocal8Bit( p.readAllStandardOutput() ).split( '\n', Qt::SkipEmptyParts );
    QStringList::ConstIterator it = l.constBegin();
    while ( it != l.constEnd() ) {
        int pid = it->mid(1).toInt();

        if ( ++it != l.constEnd() ) {
            QString app = it->mid( 1 );

            qDebug() << "(K3b::LsofWrapper) matched: app: " << app << " pid: " << pid;

            // we don't care about ourselves using the device ;)
            if( pid != int(::getpid()) )
                d->apps.append( createProcess( app, pid ) );

            ++it;
        }
    }

    return true;
}


const QList<K3b::LsofWrapper::Process>& K3b::LsofWrapper::usingApplications() const
{
    return d->apps;
}


bool K3b::LsofWrapper::findLsofExecutable()
{
    if( d->lsofBin.isEmpty() )
        d->lsofBin = K3b::findExe( "lsof" );

    return !d->lsofBin.isEmpty();
}
