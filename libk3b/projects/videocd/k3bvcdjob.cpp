/*
 *
 * Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
 * Copyright (C) 2007-2009 Sebastian Trueg <trueg@k3b.org>
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

#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <ktemporaryfile.h>
#include <kio/global.h>

#include <qstring.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qtimer.h>
#include <kdebug.h>
#include <qregexp.h>
#include <qdom.h>

#include "k3bvcdjob.h"

// K3b Includes
#include "k3bvcddoc.h"
#include "k3bvcdtrack.h"
#include "k3bvcdxmlview.h"
#include "k3bcore.h"
#include "k3bdoc.h"
#include "k3bprocess.h"
#include "k3bdevice.h"
#include "k3bexternalbinmanager.h"
#include "k3bglobals.h"
#include "k3bcdrecordwriter.h"
#include "k3bcdrdaowriter.h"
#include "k3bglobalsettings.h"
#include "k3bdevicehandler.h"


class K3b::VcdJob::Private
{
public:
    KTemporaryFile* xmlFile;
};


K3b::VcdJob::VcdJob( K3b::VcdDoc* doc, K3b::JobHandler* jh, QObject* parent )
    : K3b::BurnJob( jh, parent ),
      d( new Private )
{
    d->xmlFile = 0;
    
    m_doc = doc;
    m_doc->setCopies( m_doc->dummy() || m_doc->onlyCreateImages() ? 1 : m_doc->copies() );
    m_process = 0;
    m_currentWrittenTrackNumber = 0;
    m_bytesFinishedTracks = 0;
    m_writerJob = 0;
    // m_createimageonlypercent = 33.3;
    m_createimageonlypercent = 100 / ( m_doc->copies() + 2 );
    m_currentcopy = 1;
    m_imageFinished = false;
}


K3b::VcdJob::~VcdJob()
{
    delete d->xmlFile;
    delete d;
    
    delete m_process;

    delete m_writerJob;
}


K3b::Doc* K3b::VcdJob::doc() const
{
    return m_doc;
}


K3b::Device::Device* K3b::VcdJob::writer() const
{
    if( doc()->onlyCreateImages() )
        return 0;
    else
        return doc() ->burner();
}

void K3b::VcdJob::cancel()
{
    cancelAll();

    emit canceled();
    jobFinished( false );
}


void K3b::VcdJob::cancelAll()
{
    m_canceled = true;

    if ( m_writerJob )
        m_writerJob->cancel();

    if ( m_process->isRunning() ) {
        m_process->disconnect( this );
        m_process->terminate();
    }

    // remove bin-file if it is unfinished or the user selected to remove image
    if ( QFile::exists( m_doc->vcdImage() ) ) {
        if ( (!m_doc->onTheFly() && m_doc->removeImages()) || !m_imageFinished ) {
            emit infoMessage( i18n( "Removing Binary file %1" , m_doc->vcdImage() ), K3b::Job::MessageSuccess );
            QFile::remove
                ( m_doc->vcdImage() );
            m_doc->setVcdImage( "" );
        }
    }

    // remove cue-file if it is unfinished or the user selected to remove image
    if ( QFile::exists( m_cueFile ) ) {
        if ( (!m_doc->onTheFly() && m_doc->removeImages()) || !m_imageFinished ) {
            emit infoMessage( i18n( "Removing Cue file %1" , m_cueFile ), K3b::Job::MessageSuccess );
            QFile::remove
                ( m_cueFile );
            m_cueFile = "";
        }
    }
}


void K3b::VcdJob::start()
{
    kDebug() << "(K3b::VcdJob) starting job";

    jobStarted();
    emit burning( false );
    m_canceled = false;

    int pos = QString( m_doc->vcdImage() ).indexOf( ".bin", QString( m_doc->vcdImage() ).length() - 4 );
    if ( pos > 0 ) {
        m_cueFile = m_doc->vcdImage().left( pos ) + ".cue";
    } else {
        m_cueFile = m_doc->vcdImage() + ".cue";
        m_doc->setVcdImage( m_doc->vcdImage() + ".bin" );
    }

    if ( vcdDoc() ->onlyCreateImages() )
        m_createimageonlypercent = 50.0;

    // vcdxGen();
    xmlGen();
}

void K3b::VcdJob::xmlGen()
{
    delete d->xmlFile;
    d->xmlFile = new KTemporaryFile;
    
    if( d->xmlFile->open() ) {
        kDebug() << "(K3b::VcdJob) writing XML data to" << d->xmlFile->fileName();

        K3b::VcdXmlView xmlView( m_doc );
        xmlView.write( *d->xmlFile );

        //    emit infoMessage( i18n( "XML-file successfully created" ), K3b::Job::MessageSuccess );
        emit debuggingOutput( "K3b::VcdXml:", xmlView.xmlString() );

        vcdxBuild();
    }
    else {
        kDebug() << "(K3b::VcdJob) could not write xmlfile.";
        emit infoMessage( i18n( "Could not write correct XML-file." ), K3b::Job::MessageError );
        cancelAll();
        jobFinished( false );
    }
}

void K3b::VcdJob::vcdxBuild()
{
    emit newTask( i18n( "Creating image files" ) );

    m_stage = stageUnknown;
    firstTrack = true;
    delete m_process;
    m_process = new K3b::Process();
    m_process->setSplitStdout( true );

    emit infoMessage( i18n( "Creating Cue/Bin files ..." ), K3b::Job::MessageInfo );
    const K3b::ExternalBin* bin = k3bcore ->externalBinManager() ->binObject( "vcdxbuild" );
    if ( !bin ) {
        kDebug() << "(K3b::VcdJob) could not find vcdxbuild executable";
        emit infoMessage( i18n( "Could not find %1 executable." , QString("vcdxbuild") ), K3b::Job::MessageError );
        emit infoMessage( i18n( "To create VideoCDs you must install VcdImager Version %1." ,QString( ">= 0.7.12") ), K3b::Job::MessageInfo );
        emit infoMessage( i18n( "You can find this on your distribution disks or download it from http://www.vcdimager.org" ), K3b::Job::MessageInfo );
        cancelAll();
        jobFinished( false );
        return ;
    }

    if ( bin->version() < K3b::Version( "0.7.12" ) ) {
        kDebug() << "(K3b::VcdJob) vcdxbuild executable too old!";
        emit infoMessage( i18n( "%1 executable too old: need version %2 or greater." ,QString( "Vcdxbuild" ),QString( "0.7.12" )), K3b::Job::MessageError );
        emit infoMessage( i18n( "You can find this on your distribution disks or download it from http://www.vcdimager.org" ), K3b::Job::MessageInfo );
        cancelAll();
        jobFinished( false );
        return ;
    }

    if ( !bin->copyright().isEmpty() )
        emit infoMessage( i18n( "Using %1 %2 - Copyright (C) %3" , bin->name() , bin->version() ,bin->copyright() ), MessageInfo );

    *m_process << bin;

    // additional user parameters from config
    const QStringList& params = k3bcore->externalBinManager() ->program( "vcdxbuild" ) ->userParameters();
    for ( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
        *m_process << *it;


    if ( vcdDoc() ->vcdOptions() ->Sector2336() ) {
        kDebug() << "(K3b::VcdJob) Write 2336 Sectors = on";
        *m_process << "--sector-2336";
    }

    *m_process << "--progress" << "--gui";

    *m_process << QString( "--cue-file=%1" ).arg( m_cueFile );

    *m_process << QString( "--bin-file=%1" ).arg( m_doc->vcdImage() );

    *m_process << d->xmlFile->fileName();

    connect( m_process, SIGNAL(stdoutLine(QString)),
             this, SLOT(slotParseVcdxBuildOutput(QString)) );
    connect( m_process, SIGNAL( finished( int, QProcess::ExitStatus ) ),
             this, SLOT( slotVcdxBuildFinished( int, QProcess::ExitStatus ) ) );

    // vcdxbuild commandline parameters
    kDebug() << "***** vcdxbuild parameters:";
    QString s = m_process->joinedArgs();
    kDebug() << s << flush;
    emit debuggingOutput( "vcdxbuild command:", s );

    if ( !m_process->start( KProcess::MergedChannels ) ) {
        kDebug() << "(K3b::VcdJob) could not start vcdxbuild";
        emit infoMessage( i18n( "Could not start %1." , QString("vcdxbuild") ), K3b::Job::MessageError );
        cancelAll();
        jobFinished( false );
    }
}

void K3b::VcdJob::slotParseVcdxBuildOutput( const QString& line )
{
    QDomDocument xml_doc;
    QDomElement xml_root;

    QString str = line.trimmed();

    emit debuggingOutput( "vcdxbuild", str );

    xml_doc.setContent( QString( "<?xml version='1.0'?><vcdxbuild>" ) + str + "</vcdxbuild>" );

    xml_root = xml_doc.documentElement();

    // There should be only one... but ...
    for ( QDomNode node = xml_root.firstChild(); !node.isNull(); node = node.nextSibling() ) {
        QDomElement el = node.toElement();
        if ( el.isNull() )
            continue;

        const QString tagName = el.tagName().toLower();

        if ( tagName == "progress" ) {
            const QString oper = el.attribute( "operation" ).toLower();
            const unsigned long long pos = el.attribute( "position" ).toLong();
            const long long size = el.attribute( "size" ).toLong();

            if ( oper == "scan" ) {
                // Scan Video Files
                if ( m_stage == stageUnknown || pos < m_bytesFinished ) {
                    const uint index = el.attribute( "id" ).replace( QRegExp( "sequence-" ), "" ).toUInt();

                    m_currentWrittenTrack = m_doc->at( m_currentWrittenTrackNumber );
                    emit newSubTask( i18n( "Scanning video file %1 of %2 (%3)" , index + 1 , doc() ->numOfTracks() , m_currentWrittenTrack->fileName() ) );
                    m_bytesFinished = 0;

                    if ( !firstTrack ) {
                        m_bytesFinishedTracks += m_doc->at( m_currentWrittenTrackNumber ) ->size();
                        m_currentWrittenTrackNumber++;
                    } else
                        firstTrack = false;
                }
                emit subPercent( ( int ) ( 100.0 * ( double ) pos / ( double ) size ) );
                emit processedSubSize( pos / 1024 / 1024, size / 1024 / 1024 );

                // this is the first of three processes.
                double relOverallWritten = ( ( double ) m_bytesFinishedTracks + ( double ) pos ) / ( double ) doc() ->size();
                emit percent( ( int ) ( m_createimageonlypercent * relOverallWritten ) );

                m_bytesFinished = pos;
                m_stage = stageScan;

            } else if ( oper == "write" ) {
                emit subPercent( ( int ) ( 100.0 * ( double ) pos / ( double ) size ) );
                emit processedSubSize( ( pos * 2048 ) / 1024 / 1024, ( size * 2048 ) / 1024 / 1024 );
                emit percent( ( int ) ( m_createimageonlypercent + ( m_createimageonlypercent * ( double ) pos / ( double ) size ) ) );

                m_stage = stageWrite;
            } else {
                return ;
            }
        } else if ( tagName == "log" ) {
            QDomText tel = el.firstChild().toText();
            const QString level = el.attribute( "level" ).toLower();
            if ( tel.isText() ) {
                const QString text = tel.data();
                if ( m_stage == stageWrite && level == "information" )
                    kDebug() << QString( "(K3b::VcdJob) VcdxBuild information, %1" ).arg( text );
                if ( ( text ).startsWith( "writing track" ) )
                    emit newSubTask( i18n( "Creating Image for track %1" , ( text ).mid( 14 ) ) );
                else {
                    if ( level != "error" ) {
                        kDebug() << QString( "(K3b::VcdJob) vcdxbuild warning, %1" ).arg( text );
                        parseInformation( text );
                    } else {
                        kDebug() << QString( "(K3b::VcdJob) vcdxbuild error, %1" ).arg( text );
                        emit infoMessage( text, K3b::Job::MessageError );
                    }
                }
            }
        }
    }
}


void K3b::VcdJob::slotVcdxBuildFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    if ( exitStatus == QProcess::NormalExit ) {
        // TODO: check the process' exitStatus()
        switch ( exitCode ) {
        case 0:
            emit infoMessage( i18n( "Cue/Bin files successfully created." ), K3b::Job::MessageSuccess );
            m_imageFinished = true;
            break;
        default:
            emit infoMessage( i18n( "%1 returned an unknown error (code %2)." , QString("vcdxbuild") , exitCode ),
                              K3b::Job::MessageError );
            emit infoMessage( i18n( "Please send me an email with the last output." ), K3b::Job::MessageError );
            cancelAll();
            jobFinished( false );
            return ;
        }
    } else {
        emit infoMessage( i18n( "%1 did not exit cleanly." , QString("Vcdxbuild") ), K3b::Job::MessageError );
        cancelAll();
        jobFinished( false );
        return ;
    }

    //remove xml-file
    delete d->xmlFile;
    d->xmlFile = 0;

    kDebug() << QString( "(K3b::VcdJob) create only image: %1" ).arg( vcdDoc() ->onlyCreateImages() );
    if ( !vcdDoc() ->onlyCreateImages() )
        startWriterjob();
    else
        jobFinished( true );
}


void K3b::VcdJob::startWriterjob()
{
    kDebug() << QString( "(K3b::VcdJob) writing copy %1 of %2" ).arg( m_currentcopy ).arg( m_doc->copies() );
    if ( prepareWriterJob() ) {
        if ( waitForMedium( m_doc->burner() ) == Device::MEDIA_UNKNOWN ) {
            cancel();
            return ;
        }
        // just to be sure we did not get canceled during the async discWaiting
        if ( m_canceled )
            return ;

        if ( m_doc->copies() > 1 )
            emit newTask( i18n( "Writing Copy %1 of %2" , m_currentcopy , m_doc->copies() ) );

        emit burning( true );
        m_writerJob->start();
    }
}

bool K3b::VcdJob::prepareWriterJob()
{
    if ( m_writerJob )
        delete m_writerJob;

    const K3b::ExternalBin* cdrecordBin = k3bcore->externalBinManager() ->binObject( "cdrecord" );
    if ( writingApp() == K3b::WritingAppAuto && cdrecordBin->hasFeature( "cuefile" ) && m_doc->burner() ->dao() )
        setWritingApp( K3b::WritingAppCdrecord );

    if ( writingApp() == K3b::WritingAppCdrdao || writingApp() == K3b::WritingAppAuto ) {
        K3b::CdrdaoWriter * writer = new K3b::CdrdaoWriter( m_doc->burner(), this, this );
        // create cdrdao job
        writer->setCommand( K3b::CdrdaoWriter::WRITE );
        writer->setSimulate( m_doc->dummy() );
        writer->setBurnSpeed( m_doc->speed() );

        writer->setTocFile( m_cueFile );

        m_writerJob = writer;

    } else if ( writingApp() == K3b::WritingAppCdrecord ) {
        K3b::CdrecordWriter * writer = new K3b::CdrecordWriter( m_doc->burner(), this, this );
        // create cdrecord job

        writer->setSimulate( m_doc->dummy() );
        writer->setBurnSpeed( m_doc->speed() );
        writer->setDao( true );
        writer->setCueFile( m_cueFile );

        m_writerJob = writer;

    }

    connect( m_writerJob, SIGNAL( infoMessage( const QString&, int ) ), this, SIGNAL( infoMessage( const QString&, int ) ) );
    connect( m_writerJob, SIGNAL( percent( int ) ), this, SLOT( slotWriterJobPercent( int ) ) );
    connect( m_writerJob, SIGNAL( processedSize( int, int ) ), this, SLOT( slotProcessedSize( int, int ) ) );
    connect( m_writerJob, SIGNAL( subPercent( int ) ), this, SIGNAL( subPercent( int ) ) );
    connect( m_writerJob, SIGNAL( processedSubSize( int, int ) ), this, SIGNAL( processedSubSize( int, int ) ) );
    connect( m_writerJob, SIGNAL( nextTrack( int, int ) ), this, SLOT( slotWriterNextTrack( int, int ) ) );
    connect( m_writerJob, SIGNAL( buffer( int ) ), this, SIGNAL( bufferStatus( int ) ) );
    connect( m_writerJob, SIGNAL( deviceBuffer( int ) ), this, SIGNAL( deviceBuffer( int ) ) );
    connect( m_writerJob, SIGNAL( writeSpeed( int, K3b::Device::SpeedMultiplicator ) ), this, SIGNAL( writeSpeed( int, K3b::Device::SpeedMultiplicator ) ) );
    connect( m_writerJob, SIGNAL( finished( bool ) ), this, SLOT( slotWriterJobFinished( bool ) ) );
    connect( m_writerJob, SIGNAL( newTask( const QString& ) ), this, SIGNAL( newTask( const QString& ) ) );
    connect( m_writerJob, SIGNAL( newSubTask( const QString& ) ), this, SIGNAL( newSubTask( const QString& ) ) );
    connect( m_writerJob, SIGNAL( debuggingOutput( const QString&, const QString& ) ), this, SIGNAL( debuggingOutput( const QString&, const QString& ) ) );

    return true;
}

void K3b::VcdJob::slotWriterJobPercent( int p )
{
    emit percent( ( int ) ( ( m_createimageonlypercent * ( m_currentcopy + 1 ) ) + p / ( m_doc->copies() + 2 ) ) );
}

void K3b::VcdJob::slotProcessedSize( int cs, int ts )
{
    emit processedSize( cs + ( ts * ( m_currentcopy - 1 ) ) , ts * m_doc->copies() );
}

void K3b::VcdJob::slotWriterNextTrack( int t, int tt )
{
    emit newSubTask( i18n( "Writing Track %1 of %2" , t , tt ) );
}

void K3b::VcdJob::slotWriterJobFinished( bool success )
{
    if ( m_canceled )
        return ;

    if ( m_currentcopy >= m_doc->copies() ) {
        // remove bin-file if it is unfinished or the user selected to remove image
        if ( QFile::exists( m_doc->vcdImage() ) ) {
            if ( (!m_doc->onTheFly() && m_doc->removeImages()) || !m_imageFinished ) {
                emit infoMessage( i18n( "Removing Binary file %1" , m_doc->vcdImage() ), K3b::Job::MessageSuccess );
                QFile::remove
                    ( m_doc->vcdImage() );
                m_doc->setVcdImage( "" );
            }
        }

        // remove cue-file if it is unfinished or the user selected to remove image
        if ( QFile::exists( m_cueFile ) ) {
            if ( (!m_doc->onTheFly() && m_doc->removeImages()) || !m_imageFinished ) {
                emit infoMessage( i18n( "Removing Cue file %1" , m_cueFile ), K3b::Job::MessageSuccess );
                QFile::remove
                    ( m_cueFile );
                m_cueFile = "";
            }
        }
    }

    if ( success ) {
        // allright
        // the writerJob should have emitted the "simulation/writing successful" signal
        if ( m_currentcopy >= m_doc->copies() ) {
            if ( k3bcore->globalSettings()->ejectMedia() ) {
                K3b::Device::eject( m_doc->burner() );
            }
            jobFinished( true );
        } else {
            if( !K3b::eject( m_doc->burner() ) ) {
                blockingInformation( i18n("K3b was unable to eject the written disk. Please do so manually.") );
            }
            m_currentcopy++;
            startWriterjob();
        }
    } else {
        cancelAll();
        jobFinished( false );
    }
}

void K3b::VcdJob::parseInformation( const QString &text )
{
    // parse warning
    if ( text.contains( "mpeg user scan data: one or more BCD fields out of range for" ) ) {
        int index = text.indexOf( " for" );

        emit infoMessage( i18n( "One or more BCD fields out of range for %1" , text.mid( index + 4 ).trimmed() ), K3b::Job::MessageWarning );

    } else if ( text.contains( "mpeg user scan data: from now on, scan information data errors will not be reported anymore" ) ) {
        emit infoMessage( i18n( "From now on, scan information data errors will not be reported anymore" ), K3b::Job::MessageInfo );
        emit infoMessage( i18n( "Consider enabling the 'update scan offsets' option, if it is not enabled already." ), K3b::Job::MessageInfo );

    } else if ( text.contains( "APS' pts seems out of order (actual pts" ) ) {
        int index = text.indexOf( "(actual pts" );
        int index2 = text.indexOf( ", last seen pts" );
        int index3 = text.indexOf( ") -- ignoring this aps" );

        emit infoMessage( i18n( "APS' pts seems out of order (actual pts %1, last seen pts %2)" , text.mid( index + 12, index2 - index - 12 ).trimmed() , text.mid( index2 + 14, index3 - index2 - 14 ).trimmed() ), K3b::Job::MessageWarning );
        emit infoMessage( i18n( "Ignoring this aps" ), K3b::Job::MessageInfo );

    } else if ( text.contains( "bad packet at packet" ) ) {
        int index = text.indexOf( "at packet #" );
        int index2 = text.indexOf( "(stream byte offset" );
        int index3 = text.indexOf( ") -- remaining " );
        int index4 = text.indexOf( "bytes of stream will be ignored" );

        emit infoMessage( i18n( "Bad packet at packet #%1 (stream byte offset %2)" , text.mid( index + 11, index2 - index - 11 ).trimmed() , text.mid( index2 + 19, index3 - index2 - 19 ).trimmed() ), K3b::Job::MessageWarning );

        const QString ignoredString = text.mid( index3 + 15, index4 - index3 - 15 ).trimmed();
        bool okay = true;
        const int ignoredBytes = ignoredString.toInt(&okay);

        if (okay) {
            emit infoMessage( i18np( "The remaining byte of the stream will be ignored.", "The remaining %1 bytes of the stream will be ignored." , ignoredBytes ), K3b::Job::MessageWarning );
        } else {
            emit infoMessage( i18n( "An unknown number of remaining stream bytes will be ignored." ), K3b::Job::MessageWarning );
        }
    }
}

QString K3b::VcdJob::jobDescription() const
{
    switch ( m_doc->vcdType() ) {
    case K3b::VcdDoc::VCD11:
        return i18n( "Writing Video CD (Version 1.1)" );
    case K3b::VcdDoc::VCD20:
        return i18n( "Writing Video CD (Version 2.0)" );
    case K3b::VcdDoc::SVCD10:
        return i18n( "Writing Super Video CD" );
    case K3b::VcdDoc::HQVCD:
        return i18n( "Writing High-Quality Video CD" );
    default:
        return i18n( "Writing Video CD" );
    }
}


QString K3b::VcdJob::jobDetails() const
{
    return ( i18np( "1 MPEG (%2)",
                    "%1 MPEGs (%2)",
                    m_doc->tracks() ->count(), KIO::convertSize( m_doc->size() ) )
             + ( m_doc->copies() > 1
                 ? i18np( " - %1 copy", " - %1 copies", m_doc->copies() )
                 : QString() ) );
}

#include "k3bvcdjob.moc"
