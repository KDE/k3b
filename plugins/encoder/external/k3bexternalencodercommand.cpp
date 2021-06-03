/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "k3bexternalencodercommand.h"

#include <config-k3b.h>

#include "k3bcore.h"

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QSet>
#include <QStandardPaths>


QList<K3bExternalEncoderCommand> K3bExternalEncoderCommand::defaultCommands()
{
    QList<K3bExternalEncoderCommand> commands;
    
    // check if the lame encoding plugin has been compiled
#ifndef HAVE_LAME
    if( !QStandardPaths::findExecutable( "lame" ).isEmpty() ) {
        K3bExternalEncoderCommand lameCmd;
        lameCmd.name = "Mp3 (Lame)";
        lameCmd.extension = "mp3";
        lameCmd.command = "lame "
                            "-r "
                            "--bitwidth 16 "
                            "--little-endian "
                            "-s 44.1 "
                            "-h "
                            "--tt %t "
                            "--ta %a "
                            "--tl %m "
                            "--ty %y "
                            "--tc %c "
                            "--tn %n "
                            "- %f";

        commands.append( lameCmd );
    }
#endif

    if( !QStandardPaths::findExecutable( "flac" ).isEmpty() ) {
        K3bExternalEncoderCommand flacCmd;
        flacCmd.name = "Flac";
        flacCmd.extension = "flac";
        flacCmd.command = "flac "
                            "-V "
                            "-o %f "
                            "--force-raw-format "
                            "--endian=little "
                            "--channels=2 "
                            "--sample-rate=44100 "
                            "--sign=signed "
                            "--bps=16 "
                            "-T ARTIST=%a "
                            "-T TITLE=%t "
                            "-T TRACKNUMBER=%n "
                            "-T DATE=%y "
                            "-T ALBUM=%m "
                            "-";

        commands.append( flacCmd );
    }

    if( !QStandardPaths::findExecutable( "mppenc" ).isEmpty() ) {
        K3bExternalEncoderCommand mppCmd;
        mppCmd.name = "Musepack";
        mppCmd.extension = "mpc";
        mppCmd.command = "mppenc "
                            "--standard "
                            "--overwrite "
                            "--silent "
                            "--artist %a "
                            "--title %t "
                            "--track %n "
                            "--album %m "
                            "--comment %c "
                            "--year %y "
                            "- "
                            "%f";
        mppCmd.writeWaveHeader = true;

        commands.append( mppCmd );
    }
    
    return commands;
}

QList<K3bExternalEncoderCommand> K3bExternalEncoderCommand::readCommands()
{
    KSharedConfig::Ptr c = KSharedConfig::openConfig();
    KConfigGroup grp(c,"K3bExternalEncoderPlugin" );

    QList<K3bExternalEncoderCommand> commands;
    QSet<QString> commandNames;

    QStringList cmds = grp.readEntry( "commands",QStringList() );
    for( QStringList::iterator it = cmds.begin(); it != cmds.end(); ++it ) {
        QStringList cmdString = grp.readEntry( "command_" + *it,QStringList() );
        K3bExternalEncoderCommand cmd;
        cmd.name = cmdString[0];
        cmd.extension = cmdString[1];
        cmd.command = cmdString[2];
        for( int i = 3; i < cmdString.count(); ++i ) {
            if( cmdString[i] == "swap" )
                cmd.swapByteOrder = true;
            else if( cmdString[i] == "wave" )
                cmd.writeWaveHeader = true;
        }
        
        commands.append( cmd );
        commandNames.insert( cmd.name );
    }

    // some defaults
    QList<K3bExternalEncoderCommand> defaults = defaultCommands();
    Q_FOREACH( const K3bExternalEncoderCommand& command, defaults )
    {
        if ( !commandNames.contains( command.name ) ) {
            commands.append( command );
            commandNames.insert( command.name );
        }
    }

    return commands;
}


void K3bExternalEncoderCommand::saveCommands( const QList<K3bExternalEncoderCommand>& cmds )
{
    KSharedConfig::Ptr c = KSharedConfig::openConfig();
    c->deleteGroup( "K3bExternalEncoderPlugin" );
    KConfigGroup grp(c,"K3bExternalEncoderPlugin" );

    QStringList cmdNames;

    foreach( const K3bExternalEncoderCommand& cmd, cmds ) {
        cmdNames << cmd.name;

        QStringList cmdArgs;
        cmdArgs << cmd.name << cmd.extension << cmd.command;
        if( cmd.swapByteOrder )
            cmdArgs << "swap";
        if( cmd.writeWaveHeader )
            cmdArgs << "wave";
        grp.writeEntry( "command_" + cmd.name, cmdArgs );
    }
    grp.writeEntry( "commands", cmdNames );
}
