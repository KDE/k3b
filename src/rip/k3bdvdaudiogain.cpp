/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdvdaudiogain.h"
#include <k3bcore.h>
#include <k3bexternalbinmanager.h>

#include <qstring.h>
#include <qdir.h>

#include <kdebug.h>
#include <kprocess.h>

K3bDvdAudioGain::K3bDvdAudioGain( const QString &dir) 
  : QObject(),
    m_audioProcess(0)
{
  m_dirname = dir;
}

K3bDvdAudioGain::~K3bDvdAudioGain()
{
  if( m_audioProcess ) delete m_audioProcess;
}

bool K3bDvdAudioGain::start(){
    const K3bExternalBin *tcextract = k3bcore->externalBinManager()->binObject("tcextract");
    if( tcextract == 0 ){
        kdDebug() << "(K3bDvdAudioGain) Fatal Error, couldn't find tcextract tools." << endl;
        return false;
    }
    const K3bExternalBin *tcdecode = k3bcore->externalBinManager()->binObject("tcdecode");
    if( tcdecode == 0 ){
        kdDebug() << "(K3bDvdAudioGain) Fatal Error, couldn't find tcdecode tools." << endl;
        return false;
    }
    const K3bExternalBin *tcscan = k3bcore->externalBinManager()->binObject("tcscan");
    if( tcscan == 0 ){
        kdDebug() << "(K3bDvdAudioGain) Fatal Error, couldn't find tcscan tools." << endl;
        return false;
    }
    if( m_audioProcess ) delete m_audioProcess;
    m_audioProcess = new KShellProcess();
    QDir testDir( m_dirname );
    if( !testDir.exists() ){
        bool result = testDir.mkdir( m_dirname );
        if( result ){
            kdDebug() << "(K3bDvdAudioGain) Fatal Error, couldn't create directory <" << m_dirname << ">." << endl;
            return false;
        }
    }
    QString outputfile = m_dirname + "/audioGain.log";
    kdDebug() << "K3bDvdAudioGain) Audio processing " << tcextract->path << " -x ac3 -t vob | " << tcdecode->path << " -x ac3 | " << tcscan->path << " -x pcm ";
    *m_audioProcess << tcextract->path << " -x ac3 -t vob |" << tcdecode->path << " -x ac3 |" << tcscan->path << " -x pcm " << "&>" << outputfile;
    //connect( m_audioProcess, SIGNAL( receivedStdout( KProcess*, char*, int)), this, SLOT(slotParseOutput(KProcess*, char*, int)) );
    //connect( m_audioProcess, SIGNAL( receivedStderr( KProcess*, char*, int)), this, SLOT(slotParseError(KProcess*, char*, int)) );
    //connect( m_audioProcess, SIGNAL( wroteStdin( KProcess* )), this, SLOT(slotWroteStdin( KProcess* )) );
    connect( m_audioProcess, SIGNAL( processExited(KProcess*)), this, SLOT(slotExited( KProcess* )) );
    if( !m_audioProcess->start( KProcess::NotifyOnExit, KProcess::Stdin ) ){
        kdDebug() << "(K3bDvdAudioGain) Error starting audio processing." << endl;
        return false;
    }
    return true;
}

void K3bDvdAudioGain::writeStdin( const char *buffer, int len ){
    //kdDebug() << "(K3bDvdAudioGain) Write stdin." << endl;
    m_audioProcess->writeStdin( buffer, len );
}

void K3bDvdAudioGain::kill( ){
    kdDebug() << "(K3bDvdAudioGain) Kill AudioProcessing." << endl;
    m_audioProcess->kill();
}

void K3bDvdAudioGain::closeStdin( ){
    kdDebug() << "(K3bDvdAudioGain) Close Stdin." << endl;
    if( !m_audioProcess->closeStdin()){
        kdDebug() << "(K3bDvdAudioGain) Close failed." << endl;
    }
}

void K3bDvdAudioGain::slotParseOutput( KProcess *, char *buffer, int len ){
    QString tmp = QString::fromLocal8Bit( buffer, len );
    kdDebug() << "(K3bDvdAudioGain) AudioProcessing output: " << tmp << endl;
}

void K3bDvdAudioGain::slotParseError( KProcess *, char *buffer, int len ){
    QString tmp = QString::fromLocal8Bit( buffer, len );
    kdDebug() << "(K3bDvdAudioGain) AudioProcessing error: " << tmp << endl;
}

void K3bDvdAudioGain::slotExited( KProcess *){
    kdDebug() << "(K3bDvdAudioGain) AudioProcessing finished." << endl;
    emit finished();
}

void K3bDvdAudioGain::slotWroteStdin( KProcess*){
    //kdDebug() << "(K3bDvdAudioGain) Wrote Stdin." << endl;
}

#include "k3bdvdaudiogain.moc"
