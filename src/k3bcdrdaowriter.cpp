/***************************************************************************
                          k3bcdrdaowriter.cpp  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg and
                                       Klaus-Dieter Krannich
    email                : trueg@informatik.uni-freiburg.de
                           kd@math.tu-cottbus.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bcdrdaowriter.h"

#include <k3b.h>
#include <k3bexternalbinmanager.h>
#include <k3bdevicemanager.h>
#include <k3bprocess.h>
#include <device/k3bdevice.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qregexp.h>
#include <qfile.h>
#include <qdir.h>
#include <qurl.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>

#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>


K3bCdrdaoWriter::K3bCdrdaoWriter( K3bDevice* dev, QObject* parent, const char* name )
  : K3bAbstractWriter( dev, parent, name ),
    m_command(WRITE),
    m_blankMode(MINIMAL),
    m_sourceDevice(0),
    m_dataFile(QString("")),
    m_tocFile(QString("")),
    m_readRaw(false),
    m_multi(false),
    m_force(false),
    m_onTheFly(false),
    m_fastToc(false),
    m_readSubchan(None),
    m_taoSource(false),
    m_taoSourceAdjust(-1),
    m_paranoiaMode(-1),
    m_session(-1),
    m_cdrdaoBinObject( K3bExternalBinManager::self()->binObject("cdrdao") ),
    m_process(0),
    m_comSock(0),
    m_parser(new K3bCdrdaoParser())
{
    QPtrList<K3bDevice> devices;
    K3bDevice *d;
    if ( !dev ) {
        devices = k3bMain()->deviceManager()->burningDevices();
        d = devices.first();
        while( d ) {
            if( d->interfaceType() == K3bDevice::SCSI ) {
                setBurnDevice(d);
                break;
            }
            d = devices.next();
        }
    }
    devices = k3bMain()->deviceManager()->readingDevices();
    d = devices.first();
    while( d ) {
        if( d->interfaceType() == K3bDevice::SCSI ) {
            m_sourceDevice = d;
            break;
        }
        d = devices.next();
    }
    if ( !m_sourceDevice )
        m_sourceDevice = burnDevice();

    connect(m_parser,SIGNAL(newSubTask(const QString&)),
            this,SIGNAL(newSubTask(const QString&)));
    connect(m_parser,SIGNAL(debuggingOutput( const QString&, const QString& )),
            this,SIGNAL(debuggingOutput( const QString&, const QString& )));
    connect(m_parser,SIGNAL(infoMessage(const QString &, int)),
            this,SIGNAL(infoMessage(const QString &,int)));
    connect(m_parser,SIGNAL(percent(int)),
            this,SIGNAL(percent(int)));
    connect(m_parser,SIGNAL(buffer(int)),
            this,SIGNAL(buffer(int)));
    connect(m_parser,SIGNAL(subPercent(int)),
            this,SIGNAL(subPercent(int)));
    connect( m_parser, SIGNAL(unknownCdrdaoLine(const QString&)),
             this, SLOT(slotUnknownCdrdaoLine(const QString&)) );
    connect(m_parser,SIGNAL(nextTrack(int, int)),
            this,SIGNAL(nextTrack(int, int)));
    connect(m_parser,SIGNAL(processedSize(int, int)),
            this,SIGNAL(processedSize(int, int)));
    connect(m_parser,SIGNAL(processedSize(int, int)),
            this, SLOT(slotProcessedSize(int, int)));

            
    if( socketpair(AF_UNIX,SOCK_STREAM,0,m_cdrdaoComm) ) {
        kdDebug() << "(K3bCdrdaoWriter) could not open socketpair for cdrdao remote messages" << endl;
    } else {
        if( m_comSock )
            delete m_comSock;
        m_comSock = new QSocket();
        m_comSock->setSocket(m_cdrdaoComm[1]);
        m_comSock->socketDevice()->setReceiveBufferSize(49152);
        // magic number from Qt documentation
        m_comSock->socketDevice()->setBlocking(false);
        connect( m_comSock, SIGNAL(readyRead()),
                 this, SLOT(getCdrdaoMessage()));
    }
}

K3bCdrdaoWriter::~K3bCdrdaoWriter() {
    // close the socket
    if( m_comSock ) {
        m_comSock->close();
        ::close( m_cdrdaoComm[0] );
    }
    delete m_process;
    delete m_parser;
    delete m_comSock;
}


void K3bCdrdaoWriter::prepareArgumentList() {

    // binary
    *m_process << m_cdrdaoBinObject->path;

    // command
    switch ( m_command ) {
    case COPY:
        *m_process << "copy";
        setWriteArguments();
        setReadArguments();
        setCopyArguments();
        break;
    case WRITE:
        *m_process << "write";
        setWriteArguments();
        break;
    case READ:
        *m_process << "read-cd";
        // source device and source driver
        if ( m_sourceDevice )
            *m_process << "--device"
            << m_sourceDevice->busTargetLun();
        if ( m_sourceDevice->cdrdaoDriver() != "auto" )
            *m_process << "--driver" << m_sourceDevice->cdrdaoDriver();
        setReadArguments();
        break;
    case BLANK:
        *m_process << "blank";
        setBlankArguments();
        break;
    }

    setCommonArguments();
}

void K3bCdrdaoWriter::setWriteArguments() {
    // device and driver
    *m_process << "--device"
    << QString("%1").arg(burnDevice()->busTargetLun());

    if( burnDevice()->cdrdaoDriver() != "auto" ) {
        *m_process << "--driver";
        if( burnDevice()->cdTextCapable() == 1 )
            *m_process << QString("%1:0x00000010").arg( burnDevice()->cdrdaoDriver() );
        else
            *m_process << burnDevice()->cdrdaoDriver();
    }

    // burn speed
    *m_process << "--speed" << QString("%1").arg(burnSpeed());

    //simulate
    if( simulate() )
        *m_process << "--simulate";

    // multi
    if( m_multi )
        *m_process << "--multi";

    // force
    if( m_force )
        *m_process << "--force";

    kapp->config()->setGroup("General Options");

    bool manualBufferSize =
        k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
    if( manualBufferSize ) {
        *m_process << "--buffers"
        << QString::number( k3bMain()->config()->
                            readNumEntry( "Cdrdao buffer", 32 ) );
    }

    bool overburn =
        k3bMain()->config()->readBoolEntry( "Allow overburning", false );
    if( overburn && m_cdrdaoBinObject->hasFeature("overburn") )
        *m_process << "--overburn";


    // additional parameters from config
    QStringList params = kapp->config()->readListEntry( "cdrdao parameters" );
    for( QStringList::Iterator it=params.begin(); it != params.end(); ++it )
        *m_process << *it;
}

void K3bCdrdaoWriter::setReadArguments() {
    // readRaw
    if ( m_readRaw )
        *m_process << "--read-raw";

    // subchan
    if ( m_readSubchan != None ) {
        *m_process << "--read-subchan";
        switch ( m_readSubchan ) {
        case RW:
            *m_process << "rw";
            break;
        case RW_RAW:
            *m_process << "rw_raw";
            break;
        case None:
            break;
        }
    }

    // TAO Source
    if ( m_taoSource )
        *m_process << "--tao-source";

    // TAO Source Adjust
    if ( m_taoSourceAdjust != -1 )
        *m_process << "--tao-source-adjust"
        << QString("%1").arg(m_taoSourceAdjust);

    // paranoia Mode
    if ( m_paranoiaMode != -1 )
        *m_process << "--paranoia-mode"
        << QString("%1").arg(m_paranoiaMode);

    // session
    if ( m_session != -1 )
        *m_process << "--session"
        << QString("%1").arg(m_session);

    // fast TOC
    if ( m_fastToc )
        *m_process << "--fast-toc";

}

void K3bCdrdaoWriter::setCopyArguments() {
    // source device and source driver
    *m_process << "--source-device" << m_sourceDevice->busTargetLun();
    if ( m_sourceDevice->cdrdaoDriver() != "auto" )
        *m_process << "--source-driver" << m_sourceDevice->cdrdaoDriver();

    // on-the-fly
    if ( m_onTheFly )
        *m_process << "--on-the-fly";
}

void K3bCdrdaoWriter::setBlankArguments() {
    // device and driver
    *m_process << "--device"
    << QString("%1").arg(burnDevice()->busTargetLun());

    if( burnDevice()->cdrdaoDriver() != "auto" ) {
        *m_process << "--driver";
        if( burnDevice()->cdTextCapable() == 1 )
            *m_process << QString("%1:0x00000010").arg( burnDevice()->cdrdaoDriver() );
        else
            *m_process << burnDevice()->cdrdaoDriver();
    }

    // burn speed
    *m_process << "--speed" << QString("%1").arg(burnSpeed());

    // blank-mode
    *m_process << "--blank-mode";
    switch (m_blankMode) {
    case FULL:
        *m_process << "full";
        break;
    case MINIMAL:
        *m_process << "minimal";
        break;
    }
}

void K3bCdrdaoWriter::setCommonArguments() {
    // display debug info
    *m_process << "-n" << "-v" << "2";
    // eject
    if( k3bMain()->eject() )
        *m_process << "--eject";
    // remote

    *m_process << "--remote" <<  QString("%1").arg(m_cdrdaoComm[0]);

    // data File
    if ( ! m_dataFile.isEmpty() )
        *m_process << "--datafile" << m_dataFile;

    // TOC File
    if ( ! m_tocFile.isEmpty() )
        *m_process << m_tocFile;
}

K3bCdrdaoWriter* K3bCdrdaoWriter::addArgument( const QString& arg ) {
    *m_process << arg;
    return this;
}


void K3bCdrdaoWriter::start() {
    if( m_process )
        delete m_process;  // kdelibs want this!
    m_process = new K3bProcess();
    m_process->setSplitStdout(false);
    connect( m_process, SIGNAL(stderrLine(const QString&)),
             this, SLOT(slotStdLine(const QString&)) );
    connect( m_process, SIGNAL(processExited(KProcess*)),
             this, SLOT(slotProcessExited(KProcess*)) );
    connect( m_process, SIGNAL(wroteStdin(KProcess*)),
             this, SIGNAL(dataWritten()) );

    // workaround, cdrdao kill the tocfile when --remote parameter is set
    // hope to fix it in the future
    switch ( m_command ) {
      case WRITE:
      case COPY:
      	   if (!m_tocFile.isEmpty())
             if ( link(m_tocFile.latin1(),(m_tocFile+QString(".bak")).latin1()) == -1 )
               kdDebug() << "(cdrdaowriter) backup tocfile " <<   m_tocFile << " failed." << endl;
           break;
      case BLANK:
      case READ:
        break;
    }
    prepareArgumentList();
// set working dir to dir part of toc file (to allow rel names in toc-file)
    m_process->setWorkingDirectory(QUrl(m_tocFile).dirPath());
    
    kdDebug() << "***** cdrdao parameters:\n";
    const QValueList<QCString>& args = m_process->args();
    QString s;
    for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
        s += *it + " ";
    }
    kdDebug() << s << endl << flush;

    m_currentTrack = 0;
    m_parser->reinit();

    switch ( m_command ) {
    case READ:
        emit newSubTask( i18n("Preparing read process...") );
        break;
    case WRITE:
        emit newSubTask( i18n("Preparing write process...") );
        break;
    case COPY:
        emit newSubTask( i18n("Preparing copy process...") );
        break;
    case BLANK:
        emit newSubTask( i18n("Preparing blanking process...") );
        break;
    }
    if( !m_process->start( KProcess::NotifyOnExit, m_stdin ? KProcess::All : KProcess::AllOutput ) ) {
        // something went wrong when starting the program
        // it "should" be the executable
        kdDebug() << "(K3bCdrdaoWriter) could not start cdrdao" << endl;
        emit infoMessage( i18n("Could not start cdrdao!"), K3bJob::ERROR );
    } else {
        switch ( m_command ) {
        case WRITE:
            if( simulate() ) {
                emit infoMessage(i18n("Start simulation write at %1x speed...").arg(burnSpeed()), K3bJob::STATUS );
                emit newTask( i18n("Simulating") );
            } else {
                emit infoMessage( i18n("Start writing at %1x speed...").arg(burnSpeed()), K3bJob::STATUS );
                emit newTask( i18n("Writing") );
            }
            break;
        case READ:
            emit infoMessage(i18n("Start reading..."), K3bJob::STATUS );
            emit newTask( i18n("Reading") );
            break;
        case COPY:
            if( simulate() ) {
                emit infoMessage(i18n("Start simulation copy at %1x speed...").arg(burnSpeed()), K3bJob::STATUS );
                emit newTask( i18n("Simulating") );
            } else {
                emit infoMessage( i18n("Start copying at %1x speed...").arg(burnSpeed()), K3bJob::STATUS );
                emit newTask( i18n("Copying") );
            }
            break;
        case BLANK:
            emit infoMessage(i18n("Start blanking..."), K3bJob::STATUS );
            emit newTask( i18n("Blanking") );
        }

	// initialize estimation
	createEstimatedWriteSpeed( 0, true );

        emit started();
    }
}


void K3bCdrdaoWriter::cancel() {
    if( m_process ) {
        if( m_process->isRunning() ) {
            m_process->disconnect();
            m_process->kill();
            // we need to unlock the writer because cdrdao locked it while writing
            if ( burnDevice() ) {
                bool block = burnDevice()->block( false );
                if( !block )
                    emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR );
                else if( k3bMain()->eject() )
                    burnDevice()->eject();
            }
            if ( m_command == COPY || m_command == READ ) {
                bool block = m_sourceDevice->block( false );
                if( !block )
                    emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR );
                else if( k3bMain()->eject() )
                    m_sourceDevice->eject();
            }

        }
        switch ( m_command ) {
          case WRITE:
          case COPY:
            if ( !m_tocFile.isEmpty() )
	      if ( rename((m_tocFile+QString(".bak")).latin1(),m_tocFile.latin1()) == -1 )
                kdDebug() << "(cdrdaowriter) restore tocfile " <<   m_tocFile << " failed." << endl;
            break;
          case BLANK:
          case READ:
            break;
        }
    }
    emit canceled();
    emit finished( false );
}


void K3bCdrdaoWriter::slotStdLine( const QString& line ) {
    m_parser->parseCdrdaoLine(line);
}


void K3bCdrdaoWriter::slotProcessExited( KProcess* p ) {

    // rename toc-file before emit finished( ... );
    switch ( m_command ) {
      case WRITE:
      case COPY:
        if (!m_tocFile.isEmpty() )
          if ( rename((m_tocFile+QString(".bak")).latin1(),m_tocFile.latin1()) == -1 )
            kdDebug() << "(cdrdaowriter) restore tocfile " <<   m_tocFile << " failed." << endl;
        break;
      case BLANK:
      case READ:
        break;
    }

    if( p->normalExit() ) {
        switch( p->exitStatus() ) {
        case 0:
            if( simulate() )
                emit infoMessage( i18n("Simulation successfully finished"), K3bJob::STATUS );
            else
                switch ( m_command ) {
                case READ:
                    emit infoMessage( i18n("Reading successfully finished"), K3bJob::STATUS );
                    break;
                case WRITE:
                    emit infoMessage( i18n("Writing successfully finished"), K3bJob::STATUS );
                    break;
                case COPY:
                    emit infoMessage( i18n("Copying successfully finished"), K3bJob::STATUS );
                    break;
                case BLANK:
                    emit infoMessage( i18n("Blanking successfully finished"), K3bJob::STATUS );
                    break;
                }

	    if( m_command == WRITE || m_command == COPY )
	      createAverageWriteSpeedInfoMessage();

            emit finished( true );
            break;

        default:
            // no recording device and also other errors!! :-(
            emit infoMessage( i18n("Cdrdao returned an error! (code %1)").arg(p->exitStatus()), K3bJob::ERROR );
            emit infoMessage( strerror(p->exitStatus()), K3bJob::ERROR );
            emit infoMessage( i18n("Please send me an email with the last output..."), K3bJob::ERROR );
            emit finished( false );
            break;
        }
    } else {
        emit infoMessage( i18n("Cdrdao did not exit cleanly."), K3bJob::ERROR );
        emit finished( false );
    }
 }

void K3bCdrdaoWriter::getCdrdaoMessage() {
    m_parser->parseCdrdaoMessage(m_comSock);
}

bool K3bCdrdaoWriter::write( const char* data, int len ) {
    return m_process->writeStdin( data, len );
}



void K3bCdrdaoWriter::slotUnknownCdrdaoLine( const QString& line ) {
    if( line.contains( "at speed" ) ) {
        // parse the speed and inform the user if cdrdao switched it down
        int pos = line.find( "at speed" );
        int po2 = line.find( QRegExp("\\D"), pos + 9 );
        int speed = line.mid( pos+9, po2-pos-9 ).toInt();
        if( speed < burnSpeed() ) {
            emit infoMessage( i18n("Medium does not support writing at %1x speed").arg(burnSpeed()), K3bJob::INFO );
            emit infoMessage( i18n("Switching down burn speed to %1x").arg(speed), K3bJob::PROCESS );
        }
    }
}


void K3bCdrdaoWriter::slotProcessedSize( int s, int )
{
  createEstimatedWriteSpeed( s );
}

#include "k3bcdrdaowriter.moc"
