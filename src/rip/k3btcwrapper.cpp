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


#include "k3btcwrapper.h"
#include "k3bdvdcontent.h"
#include <k3bprocess.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bexternalbinmanager.h>
#include <k3bcore.h>

#include <qstring.h>

//#include <kprocess.h>
#include <klocale.h>
#include <kdebug.h>


K3bTcWrapper::K3bTcWrapper( QObject* parent )
  : QObject( parent )
{
    m_runTcProbeCheckOnly = false;
}

K3bTcWrapper::~K3bTcWrapper(){
}

bool K3bTcWrapper::supportDvd(){
    return k3bcore->externalBinManager()->foundBin("tcprobe");
}

void K3bTcWrapper::isDvdInsert( K3bDevice::Device* device ) {
    m_runTcProbeCheckOnly = true;
    checkDvdContent( device );
}

void K3bTcWrapper::checkDvdContent( K3bDevice::Device* device ) {
    m_firstProbeDone = false;
    m_currentTitle = 1;
    m_device = device;
    runTcprobe();
    m_dvdTitles.clear();
}

void K3bTcWrapper::runTcprobe()
{
  m_outputBuffer = QString::null;
  m_errorBuffer = QString::null;

  const K3bExternalBin *bin = k3bcore->externalBinManager()->binObject("tcprobe");
  KShellProcess *p = new KShellProcess();
  //K3bProcess *p = new K3bProcess();
  emit tcprobeTitleParsed( m_currentTitle );

  bool usemountpoint = true;
#ifndef Q_OS_FREEBSD
  usemountpoint = false;
#endif

  *p << bin->path << "-i" <<  ( usemountpoint ? m_device->mountPoint() : m_device->blockDeviceName() ) << "-T" << QString::number(m_currentTitle);
  //p->setSplitStdout( true );
  connect( p, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(slotParseTcprobeError(KProcess*, char*, int)) );
  connect( p, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(slotParseTcprobeOutput(KProcess*, char*, int)) );
  //connect( p, SIGNAL(stderrLine(const QString&)), this, SLOT(slotParseTcprobeError(const QString&)) );
  //connect( p, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotParseTcprobeOutput(const QString&)) );
  
  connect( p, SIGNAL(processExited(KProcess*)), this, SLOT(slotTcprobeExited( KProcess* )) );

  if( !p->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bTcWrapper) Error during checking drive for DVD." << endl;
  }
}

void K3bTcWrapper::slotParseTcprobeOutput( KProcess *, char *text, int len){
//void K3bTcWrapper::slotParseTcprobeOutput( const QString &text){
    //kdDebug() << "Stdout: " << text << endl;
    //kdDebug() << "Stdout Latin: " << QString::fromLatin1( text, len) << endl;
    //kdDebug() << "Stdout Ascii: " << QString::fromAscii( text, len) << endl;
    //int z = printf("Blub: %s", text);
    //QString buffer = QString("");
    //kdDebug() << "Laenge: " << len << endl;
    // due to cutted output when non-printing signs occur, prepare the output here
    for ( int i=0; i < len; i++){
        QChar c( text[i] );
        if ( c.isPrint() || ( c == "\n" || c == "\0" )){
            m_outputBuffer += c;
        }
    }
}

void K3bTcWrapper::slotParseTcprobeError( KProcess *, char *text, int len){
    m_errorBuffer += QString::fromLocal8Bit( text, len );
}

void K3bTcWrapper::slotTcprobeExited( KProcess *){
    //kdDebug() << "==============================================================================" << endl;
    //kdDebug() << "(K3bTcWrapper) Tcprobe output\n" << m_outputBuffer << endl;
    //kdDebug() << "==============================================================================" << endl;    
    //kdDebug() << "(K3bTcWrapper) Tcprobe error\n" << m_errorBuffer << endl;
    kdDebug() << "(K3bTcWrapper) Tcprobe finished" << endl;
    // split to lines
    bool isDvd = false;
    QStringList errorLines = QStringList::split( "\n", m_errorBuffer );
    if( !m_firstProbeDone ) {
        // check dvd
        for( QStringList::Iterator str = errorLines.begin(); str != errorLines.end(); str++ ) {
            kdDebug() << (*str) << endl;
            if( !(*str).contains("tcprobe") || !(*str).contains("DVD image/device") ) {
                continue;
            } else{
                isDvd = true;
            }
        }
        if( !isDvd ){
            kdDebug() << "(K3bTcWrapper) no readable dvd." << endl;
            emit successfulDvdCheck( false );
            return;
        }
        if ( m_runTcProbeCheckOnly ){
            kdDebug() << "(K3bTcWrapper) DVD check, is DVD." << endl;
            m_runTcProbeCheckOnly = false;
            emit successfulDvdCheck( true );
            return;
        }
        // chekc
        QString titles; // errorLines[ m_titleLineIndex ];
        for( QStringList::Iterator str = errorLines.begin(); str != errorLines.end(); str++ ) {
            if( (*str).contains("DVD title") ) {
                titles = (*str);
                break;
            }
        }
        kdDebug() << titles << endl;
        int index = titles.find("DVD title");
        titles = titles.mid(index+10);
        index= titles.find("/");
        m_currentTitle = titles.left(index).toInt();
        titles= titles.mid(index+1);
        index = titles.find(":");
        m_allTitle = titles.left(index).toInt();
        index = titles.find(",");
        titles = titles.mid(index+1).stripWhiteSpace();
        index = titles.find("angle");
        kdDebug() << "string: " << titles << endl;
        m_allAngle = titles.mid(0, index).stripWhiteSpace().toInt();
        m_firstProbeDone = true;
        kdDebug() << "(K3bTcWrapper) Found titles " << m_currentTitle << "/" << m_allTitle << ", angles " << m_allAngle << endl;
    }
    if( m_currentTitle <= m_allTitle ){
        K3bDvdContent con( parseTcprobe() );
        QString titles; // = errorLines[ m_titleLineIndex ];
        for( QStringList::Iterator str = errorLines.begin(); str != errorLines.end(); str++ ) {
            if( (*str).contains("DVD title") ) {
                titles = (*str);
                break;
            }
        }
        kdDebug() << titles << endl;
        int index = titles.find(":");
        int end = titles.find("chap");
        kdDebug() << "(K3bTcWrapper) Title: " << titles.mid(index+1, end-index) << endl;
        kdDebug() << "(K3bTcWrapper) Chapters " << titles.mid(index+1, end-index-1).stripWhiteSpace().toInt() << endl;
        con.setMaxChapter( titles.mid(index+1, end-index-1).stripWhiteSpace().toInt() );
        con.setTitleNumber( m_currentTitle );

        index = titles.find(",");
        titles = titles.mid(index+1).stripWhiteSpace();
        index = titles.find("angle");
        m_allAngle = titles.mid(0, index).stripWhiteSpace().toInt();
        con.setMaxAngle( m_allAngle );
        for( int a=1; a <= m_allAngle; a++){
            con.addAngle( QString::number(a) );
        }
        kdDebug() << "(K3bTcWrapper) Angles " << m_allAngle << endl;
        index = titles.find( "title set");
        titles = titles.mid( index+10).stripWhiteSpace();
        con.setTitleSet( titles.toInt() );

        m_dvdTitles.append( con );
        m_currentTitle++;
        if( m_currentTitle <= m_allTitle) {
            runTcprobe();
        } else {
            emit successfulDvdCheck( true );
        }
    }
}

K3bDvdContent K3bTcWrapper::parseTcprobe(){
    QStringList outputLines = QStringList::split( "\n", m_outputBuffer );
    // check content
    int dvdreaderIndex = 0;
    K3bDvdContent title;
    for( QStringList::Iterator str = outputLines.begin(); str != outputLines.end(); str++ ) {
        kdDebug() << (*str) << endl;
        if( (*str).contains( "dvd_reader.c" ) ) {
            int index = (*str).find( ")" );
            QString tmp = (*str).mid( index+1 );
            if( dvdreaderIndex > 0 ){
                if( tmp.contains( "kHz" ) ) {
                    // audio channels
                    kdDebug() << "(K3bTcWrapper) audio channels: '" << tmp << "'" << endl;
                    title.getAudioList()->append( tmp );
                    //kdDebug() << "(K3bTcWrapper) audio channels 2" << endl;
                }
            } else {
               // input mode
              kdDebug() << "(K3bTcWrapper) input mode" << endl;
              QStringList mode = QStringList::split( " ", tmp );
              title.setInput( mode[0] );
              title.setMode( mode[1] );
              QStringList extension = mode.grep("letterboxed");
              if ( extension.count() > 0 ) {
                  title.setAspectExtension( extension[0] );
                  title.setAspectAnamorph( "anamorph" );
              } else {
                  title.setAspectAnamorph( "" );
                  extension = mode.grep("scan");
              if ( extension.count() > 0 ) {
                  title.setAspectExtension( extension[0] );
              } else {
                  title.setAspectExtension( "" );
              }
          }
      } // end input mode
      dvdreaderIndex++;
      } else if( (*str).contains("frame size") ){
          int index = (*str).find(": -g");
          int end = (*str).find( " ", index+5);
          QString str_res = (*str).mid( index+5, end-index-5 );
          kdDebug() << "(K3bTcWrapper) Framesize: <" << str_res << ">" << endl;
          int width = str_res.mid( 0, 3 ).toInt();
          if( width != 720 ){
            kdDebug() << "(K3bTcWrapper) WARNING: parsed resolution <"<< str_res << ">is not 720x576, using default resolution of 720x576." << endl;
          }
          title.setRes( str_res );
      } else if( (*str).contains("aspect") ){
          int index = (*str).find("ratio:");
          int end = (*str).find( " ", index+7);
          title.setAspect( (*str).mid( index+7, end-index-7 ) );
      } else if( (*str).contains("frame rate") ){
          int index = (*str).find(": -f");
          int end = (*str).find( " ", index+5);
          title.setFramerate( (*str).mid( index+5, end-index-5 ) );
      } else if( (*str).contains("[tcprobe] V:") ){
          int index = (*str).find("V:");
          int end = (*str).find( " ", index+3);
          title.setFrames( (*str).mid( index+3, end-index-3 ) );
          index = (*str).find("frames,");
          end = (*str).find( " ", index+8);
          title.setTime( (*str).mid( index+8, end-index-8 ) );
      } else if( (*str).contains("[tcprobe] A:") ){
          int index = (*str).find("A:");
          title.setAudio( (*str).mid( index+3 ) );
      } else if( (*str).contains("700 MB |") ){
          int index = (*str).find("700");
          title.setVideo( (*str).mid( index+13 ) );
      }
    }
    return title;
}

const QValueList<K3bDvdContent>& K3bTcWrapper::getDvdTitles( ) const {
    return m_dvdTitles;
}

#include "k3btcwrapper.moc"
