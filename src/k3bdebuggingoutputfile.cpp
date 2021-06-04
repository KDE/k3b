/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bdebuggingoutputfile.h"

#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bcore.h"
#include "k3bversion.h"
#include "k3bdeviceglobals.h"
#include "k3bglobals.h"

#include <kcoreaddons_version.h>

#include <QDir>
#include <QStandardPaths>
#include <QTextStream>


namespace
{
    QString debuggingOutputFilePath()
    {
        QString dirPath = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
        QDir().mkpath( dirPath );
        return dirPath + "/lastlog.log";
    }
} // namespace

K3b::DebuggingOutputFile::DebuggingOutputFile()
    : QFile( debuggingOutputFilePath() )
{
}


bool K3b::DebuggingOutputFile::open( OpenMode mode )
{
    if( !QFile::open( mode|WriteOnly|Unbuffered ) )
        return false;

    addOutput( QLatin1String( "System" ), QString::fromLatin1( "K3b Version: %1" ).arg(k3bcore->version()) );
    addOutput( QLatin1String( "System" ), QString::fromLatin1( "KDE Version: %1" ).arg(KCOREADDONS_VERSION_STRING) );
    addOutput( QLatin1String( "System" ), QString::fromLatin1( "Qt Version:  %1" ).arg(qVersion()) );
    addOutput( QLatin1String( "System" ), QString::fromLatin1( "Kernel:      %1" ).arg(K3b::kernelVersion()) );

    // devices in the logfile
    Q_FOREACH( K3b::Device::Device* dev, k3bcore->deviceManager()->allDevices() ) {
        addOutput( "Devices",
                   QString( "%1 (%2, %3) [%5] [%6] [%7]" )
                   .arg( dev->vendor() + ' ' + dev->description() + ' ' + dev->version() )
                   .arg( dev->blockDeviceName() )
                   .arg( K3b::Device::deviceTypeString( dev->type() ) )
                   .arg( K3b::Device::mediaTypeString( dev->supportedProfiles() ) )
                   .arg( K3b::Device::writingModeString( dev->writingModes() ) ) );
    }

    return true;
}


void K3b::DebuggingOutputFile::addOutput( const QString& app, const QString& msg )
{
    if( !isOpen() )
        open();

    QTextStream s( this );
    s << "[" << app << "] " << msg << endl;
    flush();
}


