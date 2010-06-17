/*
 *
 * Copyright (C) 2003-2005 Christian Kvasny <chris@k3b.org>
 * Copyright (C) 2007-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvcddoc.h"
#include "k3bvcdtrack.h"
#include "k3bvcdjob.h"
#include "k3bglobals.h"
#include "k3bmsf.h"

#include <QDataStream>
#include <QDomElement>
#include <QFile>
#include <QImage>
#include <QTimer>

#include <KApplication>
#include <KConfig>
#include <KDebug>
#include <kio/global.h>
#include <KLocale>
#include <KMessageBox>
#include <KStandardDirs>
#include <KStandardGuiItem>


#if 0
bool desperate_mode = false;
bool preserve_header = false;
bool print_progress = true;
bool aspect_correction = false;
byte forced_sequence_header = 0;
#endif

K3b::VcdDoc::VcdDoc( QObject* parent )
    : K3b::Doc( parent )
{
    m_tracks = 0L;
    m_vcdOptions = new K3b::VcdOptions();

    m_vcdType = NONE;

    m_urlAddingTimer = new QTimer( this );
    connect( m_urlAddingTimer, SIGNAL( timeout() ), this, SLOT( slotWorkUrlQueue() ) );

    // FIXME: remove the newTracks() signal and replace it with the changed signal
    connect( this, SIGNAL( newTracks() ), this, SIGNAL( changed() ) );
    connect( this, SIGNAL( trackRemoved( K3b::VcdTrack* ) ), this, SIGNAL( changed() ) );
}

K3b::VcdDoc::~VcdDoc()
{
    if ( m_tracks ) {
        qDeleteAll( *m_tracks );
        delete m_tracks;
    }

    delete m_vcdOptions;
}

bool K3b::VcdDoc::newDocument()
{
    clear();
    if ( !m_tracks )
        m_tracks = new QList<K3b::VcdTrack*>;

    return K3b::Doc::newDocument();
}


void K3b::VcdDoc::clear()
{
    if ( m_tracks )
        while ( !m_tracks->isEmpty() )
            removeTrack( m_tracks->first() );
}


QString K3b::VcdDoc::name() const
{
    return m_vcdOptions->volumeId();
}


KIO::filesize_t K3b::VcdDoc::calcTotalSize() const
{
    unsigned long long sum = 0;
    if ( m_tracks ) {
        Q_FOREACH( K3b::VcdTrack* track, *m_tracks ) {
            sum += track->size();
        }
    }
    return sum;
}

KIO::filesize_t K3b::VcdDoc::size() const
{
    // mode2 -> mode1 int(( n+2047 ) / 2048) * 2352
    // mode1 -> mode2 int(( n+2351 ) / 2352) * 2048
    long tracksize = long( ( calcTotalSize() + 2351 ) / 2352 ) * 2048;
    return tracksize + ISOsize();
}

KIO::filesize_t K3b::VcdDoc::ISOsize() const
{
    // 136000b for vcd iso reseved
    long long iso_size = 136000;
    if ( vcdOptions() ->CdiSupport() ) {
        iso_size += vcdOptions() ->CDIsize();
    }

    return iso_size;
}

K3b::Msf K3b::VcdDoc::length() const
{
    return K3b::Msf( size() / 2048 );
}


bool K3b::VcdDoc::isImage( const KUrl& url )
{
    QImage p;
    return p.load( QFile::encodeName( url.toLocalFile() ) );
}

void K3b::VcdDoc::addUrls( const KUrl::List& urls )
{
    // make sure we add them at the end even if urls are in the queue
    addTracks( urls, 99 );
}

void K3b::VcdDoc::addTracks( const KUrl::List& urls, uint position )
{
    KUrl::List::ConstIterator end( urls.end() );
    for ( KUrl::List::ConstIterator it = urls.begin(); it != end; ++it ) {
        urlsToAdd.enqueue( new PrivateUrlToAdd( K3b::convertToLocalUrl(*it), position++ ) );
    }

    m_urlAddingTimer->start( 0 );
}

void K3b::VcdDoc::slotWorkUrlQueue()
{
    if ( !urlsToAdd.isEmpty() ) {
        PrivateUrlToAdd * item = urlsToAdd.dequeue();
        lastAddedPosition = item->position;

        // append at the end by default
        if ( lastAddedPosition > m_tracks->count() )
            lastAddedPosition = m_tracks->count();

        if ( !item->url.isLocalFile() ) {
            kDebug() << item->url.toLocalFile() << " no local file";
            return ;
        }

        if ( !QFile::exists( item->url.toLocalFile() ) ) {
            kDebug() << "(K3b::VcdDoc) file not found: " << item->url.toLocalFile();
            m_notFoundFiles.append( item->url.toLocalFile() );
            return ;
        }

        if ( K3b::VcdTrack * newTrack = createTrack( item->url ) )
            addTrack( newTrack, lastAddedPosition );

        delete item;

        emit newTracks();
    } else {
        m_urlAddingTimer->stop();

        emit newTracks();

        // reorder pbc tracks
        setPbcTracks();

        informAboutNotFoundFiles();
    }
}

K3b::VcdTrack* K3b::VcdDoc::createTrack( const KUrl& url )
{
    char filename[ 255 ];
    QString error_string = "";
    strcpy( filename, QFile::encodeName( url.toLocalFile() ) );
    K3b::MpegInfo* Mpeg = new K3b::MpegInfo( filename );

    if ( Mpeg ) {
        int mpegVersion = Mpeg->version();
        if ( mpegVersion > 0 ) {

            if ( vcdType() == NONE && mpegVersion < 2 ) {
                m_urlAddingTimer->stop();
                setVcdType( vcdTypes( mpegVersion ) );
                // FIXME: properly convert the mpeg version
                vcdOptions() ->setMpegVersion( ( K3b::VcdOptions::MPEGVersion )mpegVersion );
                KMessageBox::information( kapp->activeWindow(),
                                          i18n( "K3b will create a %1 image from the given MPEG "
                                                "files, but these files must already be in %1 "
                                                "format. K3b does not yet resample MPEG files.",
                                                 i18n( "VCD" ) ),
                                          i18n( "Information" ) );
                m_urlAddingTimer->start( 0 );
            } else if ( vcdType() == NONE ) {
                m_urlAddingTimer->stop();
                vcdOptions() ->setMpegVersion( ( K3b::VcdOptions::MPEGVersion )mpegVersion );
                bool force = KMessageBox::questionYesNo( kapp->activeWindow(),
                                                         i18n( "K3b will create a %1 image from the given MPEG "
                                                               "files, but these files must already be in %1 "
                                                               "format. K3b does not yet resample MPEG files.",
                                                               i18n( "SVCD" ) )
                                                         + "\n\n"
                                                         + i18n( "Note: Forcing MPEG2 as VCD is not supported by "
                                                                 "some standalone DVD players." ),
                                                         i18n( "Information" ),
                                                         KGuiItem( i18n( "Force VCD" ) ),
                                                         KGuiItem( i18n( "Do not force VCD" ) ) ) == KMessageBox::Yes;
                if ( force ) {
                    setVcdType( vcdTypes( 1 ) );
                    vcdOptions() ->setAutoDetect( false );
                } else
                    setVcdType( vcdTypes( mpegVersion ) );

                m_urlAddingTimer->start( 0 );
            }


            if ( numOfTracks() > 0 && vcdOptions() ->mpegVersion() != mpegVersion ) {
                KMessageBox::error( kapp->activeWindow(), "(" + url.toLocalFile() + ")\n" +
                                    i18n( "You cannot mix MPEG1 and MPEG2 video files.\nPlease start a new Project for this filetype.\nResample not implemented in K3b yet." ),
                                    i18n( "Wrong File Type for This Project" ) );

                delete Mpeg;
                return 0;
            }

            K3b::VcdTrack* newTrack = new K3b::VcdTrack( m_tracks, url.toLocalFile() );
            *( newTrack->mpeg_info ) = *( Mpeg->mpeg_info );

            if ( newTrack->isSegment() && !vcdOptions()->PbcEnabled() ) {
                KMessageBox::information( kapp->activeWindow(),
                                          i18n( "PBC (Playback control) enabled.\n"
                                                "Videoplayers can not reach Segments (Mpeg Still Pictures) without Playback control ." ) ,
                                          i18n( "Information" ) );

                vcdOptions()->setPbcEnabled( true );
            }

            // set defaults;
            newTrack->setPlayTime( vcdOptions() ->PbcPlayTime() );
            newTrack->setWaitTime( vcdOptions() ->PbcWaitTime() );
            newTrack->setPbcNumKeys( vcdOptions() ->PbcNumkeysEnabled() );
            delete Mpeg;

            // debugging output
            newTrack->PrintInfo();

            return newTrack;
        }
    } else if ( isImage( url ) ) { // image track
        // woking on ...
        // for future use
        // photoalbum starts here
        // return here the new photoalbum track
    }

    if ( Mpeg ) {
        error_string = Mpeg->error_string();
        delete Mpeg;
    }

    // error (unsupported files)
    KMessageBox::error( kapp->activeWindow(), "(" + url.toLocalFile() + ")\n" +
                        i18n( "Only MPEG1 and MPEG2 video files are supported.\n" ) + error_string ,
                        i18n( "Wrong File Format" ) );


    return 0;
}

void K3b::VcdDoc::addTrack( const KUrl& url, uint position )
{
    urlsToAdd.enqueue( new PrivateUrlToAdd( url, position ) );

    m_urlAddingTimer->start( 0 );
}


void K3b::VcdDoc::addTrack( K3b::VcdTrack* track, uint position )
{
    if ( m_tracks->count() >= 98 ) {
        kDebug() << "(K3b::VcdDoc) VCD Green Book only allows 98 tracks.";
        // TODO: show some messagebox
        delete track;
        return ;
    }

    lastAddedPosition = position;

    emit aboutToAddVCDTracks( position, 1);

    m_tracks->insert( position, track );

    if ( track->isSegment() )
        vcdOptions() ->increaseSegments( );
    else
        vcdOptions() ->increaseSequence( );

    emit addedVCDTracks();

    emit newTracks();

    setModified( true );
}


void K3b::VcdDoc::removeTrack( K3b::VcdTrack* track )
{
    if ( !track ) {
        return ;
    }

    // set the current item to track
    if ( m_tracks->lastIndexOf( track ) >= 0 ) {
        // take the current item
        int removedPos = m_tracks->lastIndexOf( track );

        emit aboutToRemoveVCDTracks(removedPos, 1);

        track = m_tracks->takeAt( removedPos );

        emit removedVCDTracks();

        // remove all pbc references to us?
        if ( track->hasRevRef() )
            track->delRefToUs();

        // remove all pbc references from us?
        track->delRefFromUs();

        // emit signal before deleting the track to avoid crashes
        // when the view tries to call some of the tracks' methods
        emit trackRemoved( track );

        if ( track->isSegment() )
            vcdOptions() ->decreaseSegments( );
        else
            vcdOptions() ->decreaseSequence( );

        delete track;

        if ( numOfTracks() == 0 ) {
            setVcdType( NONE );
            vcdOptions() ->setAutoDetect( true );
        }

        // reorder pbc tracks
        setPbcTracks();
    }
}

void K3b::VcdDoc::moveTrack( K3b::VcdTrack* track, K3b::VcdTrack* before )
{
    if ( track == before )
        return ;

    // take the current item
    int removedPos = m_tracks->lastIndexOf( track );

    emit aboutToRemoveVCDTracks(removedPos, 1);

    m_tracks->removeAt(removedPos);

    emit removedVCDTracks();

    if( before != 0 ) {
        int pos = m_tracks->lastIndexOf( before );
        emit aboutToAddVCDTracks(pos, 1);
        m_tracks->insert( pos, track );
    }
    else {
        emit aboutToAddVCDTracks( m_tracks->count(), 1 );
        m_tracks->append( track );
    }

    emit addedVCDTracks();

    // reorder pbc tracks
    setPbcTracks();

    emit changed();
}


K3b::BurnJob* K3b::VcdDoc::newBurnJob( K3b::JobHandler* hdl, QObject* parent )
{
    return new K3b::VcdJob( this, hdl, parent );
}

void K3b::VcdDoc::informAboutNotFoundFiles()
{
    if ( !m_notFoundFiles.isEmpty() ) {
        KMessageBox::informationList( view(), i18n( "Could not find the following files:" ),
                                      m_notFoundFiles, i18n( "Not Found" ) );

        m_notFoundFiles.clear();
    }
}

void K3b::VcdDoc::setVcdType( int type )
{
    m_vcdType = type;
    switch ( type ) {
    case 0:
        //vcd 1.1
        vcdOptions() ->setVcdClass( "vcd" );
        vcdOptions() ->setVcdVersion( "1.1" );
        break;
    case 1:
        //vcd 2.0
        vcdOptions() ->setVcdClass( "vcd" );
        vcdOptions() ->setVcdVersion( "2.0" );
        break;
    case 2:
        //svcd 1.0
        vcdOptions() ->setVcdClass( "svcd" );
        vcdOptions() ->setVcdVersion( "1.0" );
        break;
    case 3:
        //hqvcd 1.0
        vcdOptions() ->setVcdClass( "hqvcd" );
        vcdOptions() ->setVcdVersion( "1.0" );
        break;

    }
}

void K3b::VcdDoc::setPbcTracks()
{
    // reorder pbc tracks
    /*
      if ( !vcdOptions()->PbcEnabled() )
      return;
    */

    if ( m_tracks ) {
        int count = m_tracks->count();
        kDebug() << QString( "K3b::VcdDoc::setPbcTracks() - we have %1 tracks in list." ).arg( count );

        Q_FOREACH( K3b::VcdTrack* track, *m_tracks ) {
            Q_FOREACH( VcdTrack::PbcTracks pbc, VcdTrack::trackPlaybackValues() ) {
                // do not change userdefined tracks
                if ( !track->isPbcUserDefined( pbc ) ) {
                    if ( track->getPbcTrack( pbc ) )
                        track->getPbcTrack( pbc ) ->delFromRevRefList( track );

                    K3b::VcdTrack* t = 0L;
                    int index = track->index();

                    // we are the last track
                    if ( index == count - 1 ) {
                        switch ( pbc ) {
                        case K3b::VcdTrack::PREVIOUS:
                            // we are not alone :)
                            if ( count > 1 ) {
                                t = m_tracks->at( index - 1 );
                                t->addToRevRefList( track );
                                track->setPbcTrack( pbc, t );
                            } else {
                                track->setPbcTrack( pbc );
                                track->setPbcNonTrack( pbc, K3b::VcdTrack::VIDEOEND );
                            }
                            break;
                        case K3b::VcdTrack::AFTERTIMEOUT:
                        case K3b::VcdTrack::NEXT:
                            track->setPbcTrack( pbc );
                            track->setPbcNonTrack( pbc, K3b::VcdTrack::VIDEOEND );
                            break;
                        case K3b::VcdTrack::RETURN:
                            track->setPbcTrack( pbc );
                            track->setPbcNonTrack( pbc, K3b::VcdTrack::VIDEOEND );
                            break;
                        case K3b::VcdTrack::DEFAULT:
                            track->setPbcTrack( pbc );
                            track->setPbcNonTrack( pbc, K3b::VcdTrack::DISABLED );
                            break;
                        }
                    }
                    // we are the first track
                    else if ( index == 0 ) {
                        switch ( pbc ) {
                        case K3b::VcdTrack::PREVIOUS:
                            track->setPbcTrack( pbc );
                            track->setPbcNonTrack( pbc, K3b::VcdTrack::VIDEOEND );
                            break;
                        case K3b::VcdTrack::AFTERTIMEOUT:
                        case K3b::VcdTrack::NEXT:
                            t = m_tracks->at( index + 1 );
                            t->addToRevRefList( track );
                            track->setPbcTrack( pbc, t );
                            break;
                        case K3b::VcdTrack::RETURN:
                            track->setPbcTrack( pbc );
                            track->setPbcNonTrack( pbc, K3b::VcdTrack::VIDEOEND );
                            break;
                        case K3b::VcdTrack::DEFAULT:
                            track->setPbcTrack( pbc );
                            track->setPbcNonTrack( pbc, K3b::VcdTrack::DISABLED );
                            break;
                        }
                    }
                    // we are one of the other tracks and have PREVIOUS and NEXT Track
                    else {
                        switch ( pbc ) {
                        case K3b::VcdTrack::PREVIOUS:
                            t = m_tracks->at( index - 1 );
                            t->addToRevRefList( track );
                            track->setPbcTrack( pbc, t );
                            break;
                        case K3b::VcdTrack::AFTERTIMEOUT:
                        case K3b::VcdTrack::NEXT:
                            t = m_tracks->at( index + 1 );
                            t->addToRevRefList( track );
                            track->setPbcTrack( pbc, t );
                            break;
                        case K3b::VcdTrack::RETURN:
                            track->setPbcTrack( pbc );
                            track->setPbcNonTrack( pbc, K3b::VcdTrack::VIDEOEND );
                            break;
                        case K3b::VcdTrack::DEFAULT:
                            track->setPbcTrack( pbc );
                            track->setPbcNonTrack( pbc, K3b::VcdTrack::DISABLED );
                            break;
                        }
                    }
                }
            }
        }
    }
}


bool K3b::VcdDoc::loadDocumentData( QDomElement* root )
{
    newDocument();

    QDomNodeList nodes = root->childNodes();

    if ( nodes.length() < 3 )
        return false;

    if ( nodes.item( 0 ).nodeName() != "general" )
        return false;
    if ( !readGeneralDocumentData( nodes.item( 0 ).toElement() ) )
        return false;

    if ( nodes.item( 1 ).nodeName() != "vcd" )
        return false;

    if ( nodes.item( 2 ).nodeName() != "contents" )
        return false;


    // vcd Label
    QDomNodeList vcdNodes = nodes.item( 1 ).childNodes();

    for ( int i = 0; i < vcdNodes.count(); i++ ) {
        QDomNode item = vcdNodes.item( i );
        QString name = item.nodeName();

        kDebug() << QString( "(K3b::VcdDoc::loadDocumentData) nodeName = '%1'" ).arg( name );

        if ( name == "volumeId" )
            vcdOptions() ->setVolumeId( item.toElement().text() );
        else if ( name == "albumId" )
            vcdOptions() ->setAlbumId( item.toElement().text() );
        else if ( name == "volumeSetId" )
            vcdOptions() ->setVolumeSetId( item.toElement().text() );
        else if ( name == "preparer" )
            vcdOptions() ->setPreparer( item.toElement().text() );
        else if ( name == "publisher" )
            vcdOptions() ->setPublisher( item.toElement().text() );
        else if ( name == "vcdType" )
            setVcdType( vcdTypes( item.toElement().text().toInt() ) );
        else if ( name == "mpegVersion" )
            vcdOptions() ->setMpegVersion( ( K3b::VcdOptions::MPEGVersion )item.toElement().text().toInt() );
        else if ( name == "PreGapLeadout" )
            vcdOptions() ->setPreGapLeadout( item.toElement().text().toInt() );
        else if ( name == "PreGapTrack" )
            vcdOptions() ->setPreGapTrack( item.toElement().text().toInt() );
        else if ( name == "FrontMarginTrack" )
            vcdOptions() ->setFrontMarginTrack( item.toElement().text().toInt() );
        else if ( name == "RearMarginTrack" )
            vcdOptions() ->setRearMarginTrack( item.toElement().text().toInt() );
        else if ( name == "FrontMarginTrackSVCD" )
            vcdOptions() ->setFrontMarginTrackSVCD( item.toElement().text().toInt() );
        else if ( name == "RearMarginTrackSVCD" )
            vcdOptions() ->setRearMarginTrackSVCD( item.toElement().text().toInt() );
        else if ( name == "volumeCount" )
            vcdOptions() ->setVolumeCount( item.toElement().text().toInt() );
        else if ( name == "volumeNumber" )
            vcdOptions() ->setVolumeNumber( item.toElement().text().toInt() );
        else if ( name == "AutoDetect" )
            vcdOptions() ->setAutoDetect( item.toElement().text().toInt() );
        else if ( name == "CdiSupport" )
            vcdOptions() ->setCdiSupport( item.toElement().text().toInt() );
        else if ( name == "NonCompliantMode" )
            vcdOptions() ->setNonCompliantMode( item.toElement().text().toInt() );
        else if ( name == "Sector2336" )
            vcdOptions() ->setSector2336( item.toElement().text().toInt() );
        else if ( name == "UpdateScanOffsets" )
            vcdOptions() ->setUpdateScanOffsets( item.toElement().text().toInt() );
        else if ( name == "RelaxedAps" )
            vcdOptions() ->setRelaxedAps( item.toElement().text().toInt() );
        else if ( name == "UseGaps" )
            vcdOptions() ->setUseGaps( item.toElement().text().toInt() );
        else if ( name == "PbcEnabled" )
            vcdOptions() ->setPbcEnabled( item.toElement().text().toInt() );
        else if ( name == "SegmentFolder" )
            vcdOptions() ->setSegmentFolder( item.toElement().text().toInt() );
        else if ( name == "Restriction" )
            vcdOptions() ->setRestriction( item.toElement().text().toInt() );
    }

    // vcd Tracks
    QDomNodeList trackNodes = nodes.item( 2 ).childNodes();

    for ( uint i = 0; i < trackNodes.length(); i++ ) {

        // check if url is available
        QDomElement trackElem = trackNodes.item( i ).toElement();
        QString url = trackElem.attributeNode( "url" ).value();
        if ( !QFile::exists( url ) )
            m_notFoundFiles.append( url );
        else {
            KUrl k;
            k.setPath( url );
            if ( K3b::VcdTrack * track = createTrack( k ) ) {
                track ->setPlayTime( trackElem.attribute( "playtime", "1" ).toInt() );
                track ->setWaitTime( trackElem.attribute( "waittime", "2" ).toInt() );
                track ->setReactivity( trackElem.attribute( "reactivity", "0" ).toInt() );
                track -> setPbcNumKeys( ( trackElem.attribute( "numkeys", "yes" ).contains( "yes" ) ) ? true : false );
                track -> setPbcNumKeysUserdefined( ( trackElem.attribute( "userdefinednumkeys", "no" ).contains( "yes" ) ) ? true : false );

                addTrack( track, m_tracks->count() );
            }
        }
    }

    emit newTracks();

    // do not add saved pbcTrack links when one ore more files missing.
    // TODO: add info message to informAboutNotFoundFiles();
    if ( m_notFoundFiles.isEmpty() ) {
        VcdTrack::PbcTracks type;
        VcdTrack::PbcTypes val;
        bool pbctrack;
        for ( uint trackId = 0; trackId < trackNodes.length(); trackId++ ) {
            QDomElement trackElem = trackNodes.item( trackId ).toElement();
            QDomNodeList trackNodes = trackElem.childNodes();
            for ( uint i = 0; i < trackNodes.length(); i++ ) {
                QDomElement trackElem = trackNodes.item( i ).toElement();
                QString name = trackElem.tagName();
                if ( name.contains( "pbc" ) ) {
                    if ( trackElem.hasAttribute ( "type" ) ) {
                        type = static_cast<VcdTrack::PbcTracks>( trackElem.attribute ( "type" ).toInt() );
                        if ( trackElem.hasAttribute ( "pbctrack" ) ) {
                            pbctrack = ( trackElem.attribute ( "pbctrack" ) == "yes" );
                            if ( trackElem.hasAttribute ( "val" ) ) {
                                val = static_cast<VcdTrack::PbcTypes>( trackElem.attribute ( "val" ).toInt() );
                                K3b::VcdTrack* track = m_tracks->at( trackId );
                                K3b::VcdTrack* pbcTrack = m_tracks->at( val );
                                if ( pbctrack ) {
                                    pbcTrack->addToRevRefList( track );
                                    track->setPbcTrack( type, pbcTrack );
                                    track->setUserDefined( type, true );
                                } else {
                                    track->setPbcTrack( type );
                                    track->setPbcNonTrack( type, val );
                                    track->setUserDefined( type, true );
                                }
                            }
                        }
                    }
                } else if ( name.contains( "numkeys" ) ) {
                    if ( trackElem.hasAttribute ( "key" ) ) {
                        int key = trackElem.attribute ( "key" ).toInt();
                        if ( trackElem.hasAttribute ( "val" ) ) {
                            int val = trackElem.attribute ( "val" ).toInt() - 1;
                            K3b::VcdTrack* track = m_tracks->at( trackId );
                            if ( val >= 0 ) {
                                K3b::VcdTrack * numkeyTrack = m_tracks->at( val );
                                track->setDefinedNumKey( key, numkeyTrack );
                            } else {
                                track->setDefinedNumKey( key, 0L );
                            }
                        }
                    }
                }

            }

        }
        setPbcTracks();
        setModified( false );
    }

    informAboutNotFoundFiles();
    return true;
}



bool K3b::VcdDoc::saveDocumentData( QDomElement * docElem )
{
    QDomDocument doc = docElem->ownerDocument();
    saveGeneralDocumentData( docElem );

    // save Vcd Label
    QDomElement vcdMain = doc.createElement( "vcd" );

    QDomElement vcdElem = doc.createElement( "volumeId" );
    vcdElem.appendChild( doc.createTextNode( vcdOptions() ->volumeId() ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "albumId" );
    vcdElem.appendChild( doc.createTextNode( vcdOptions() ->albumId() ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "volumeSetId" );
    vcdElem.appendChild( doc.createTextNode( vcdOptions() ->volumeSetId() ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "preparer" );
    vcdElem.appendChild( doc.createTextNode( vcdOptions() ->preparer() ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "publisher" );
    vcdElem.appendChild( doc.createTextNode( vcdOptions() ->publisher() ) );
    vcdMain.appendChild( vcdElem );

    // applicationId()
    // systemId()

    vcdElem = doc.createElement( "vcdType" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdType() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "mpegVersion" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->mpegVersion() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "PreGapLeadout" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->PreGapLeadout() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "PreGapTrack" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->PreGapTrack() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "FrontMarginTrack" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->FrontMarginTrack() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "RearMarginTrack" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->RearMarginTrack() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "FrontMarginTrackSVCD" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->FrontMarginTrackSVCD() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "RearMarginTrackSVCD" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->RearMarginTrackSVCD() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "volumeCount" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->volumeCount() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "volumeNumber" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->volumeNumber() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "AutoDetect" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->AutoDetect() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "CdiSupport" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->CdiSupport() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "NonCompliantMode" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->NonCompliantMode() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "Sector2336" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->Sector2336() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "UpdateScanOffsets" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->UpdateScanOffsets() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "RelaxedAps" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->RelaxedAps() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "UseGaps" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->UseGaps() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "PbcEnabled" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->PbcEnabled() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "SegmentFolder" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->SegmentFolder() ) ) );
    vcdMain.appendChild( vcdElem );

    vcdElem = doc.createElement( "Restriction" );
    vcdElem.appendChild( doc.createTextNode( QString::number( vcdOptions() ->Restriction() ) ) );
    vcdMain.appendChild( vcdElem );

    docElem->appendChild( vcdMain );

    // save the tracks
    // -------------------------------------------------------------
    QDomElement contentsElem = doc.createElement( "contents" );

    Q_FOREACH( K3b::VcdTrack* track, *m_tracks ) {
        QDomElement trackElem = doc.createElement( "track" );
        trackElem.setAttribute( "url", KIO::decodeFileName( track->absolutePath() ) );
        trackElem.setAttribute( "playtime", track->getPlayTime() );
        trackElem.setAttribute( "waittime", track->getWaitTime() );
        trackElem.setAttribute( "reactivity", track->Reactivity() );
        trackElem.setAttribute( "numkeys", ( track->PbcNumKeys() ) ? "yes" : "no" );
        trackElem.setAttribute( "userdefinednumkeys", ( track->PbcNumKeysUserdefined() ) ? "yes" : "no" );

        Q_FOREACH( VcdTrack::PbcTracks pbc, VcdTrack::trackPlaybackValues() ) {
            if ( track->isPbcUserDefined( pbc ) ) {
                // save pbcTracks
                QDomElement pbcElem = doc.createElement( "pbc" );
                pbcElem.setAttribute( "type", pbc );
                if ( track->getPbcTrack( pbc ) ) {
                    pbcElem.setAttribute( "pbctrack", "yes" );
                    pbcElem.setAttribute( "val", track->getPbcTrack( pbc ) ->index() );
                } else {
                    pbcElem.setAttribute( "pbctrack", "no" );
                    pbcElem.setAttribute( "val", track->getNonPbcTrack( pbc ) );
                }
                trackElem.appendChild( pbcElem );
            }
        }
        QMap<int, K3b::VcdTrack*> numKeyMap = track->DefinedNumKey();
        QMap<int, K3b::VcdTrack*>::const_iterator trackIt;

        for ( trackIt = numKeyMap.constBegin();
              trackIt != numKeyMap.constEnd();
              ++trackIt ) {
            QDomElement numElem = doc.createElement( "numkeys" );
            if ( *trackIt ) {
                numElem.setAttribute( "key", trackIt.key() );
                numElem.setAttribute( "val", trackIt.value() ->index() + 1 );
            } else {
                numElem.setAttribute( "key", trackIt.key() );
                numElem.setAttribute( "val", 0 );
            }
            trackElem.appendChild( numElem );
        }

        contentsElem.appendChild( trackElem );
    }
    // -------------------------------------------------------------

    docElem->appendChild( contentsElem );

    return true;
}


K3b::Device::MediaTypes K3b::VcdDoc::supportedMediaTypes() const
{
    return K3b::Device::MEDIA_WRITABLE_CD;
}

#include "k3bvcddoc.moc"
