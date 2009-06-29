/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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

#include "k3bglobals.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3baudiojob.h"
#include "k3baudiofile.h"
#include "k3baudiozerodata.h"
#include "k3baudiocdtracksource.h"
#include "k3brawaudiodatasource.h"
#include "k3bcuefileparser.h"
#include "k3bcdtextvalidator.h"
#include "k3bcore.h"
#include "k3baudiodecoder.h"

#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QDir>
#include <QDomElement>
#include <QStringList>
#include <QTextStream>

#include <KApplication>
#include <KConfig>
#include <KDebug>
#include <KLocale>
#include <KMessageBox>
#include <kio/global.h>
#include <KStandardDirs>


class K3b::AudioDoc::Private
{
public:
    Private() {
        cdTextValidator = new K3b::CdTextValidator();
    }

    ~Private() {
        delete cdTextValidator;
    }

    K3b::CdTextValidator* cdTextValidator;
};


K3b::AudioDoc::AudioDoc( QObject* parent )
    : K3b::Doc( parent ),
      m_firstTrack(0),
      m_lastTrack(0)
{
    d = new Private;
}

K3b::AudioDoc::~AudioDoc()
{
    // delete all tracks
    int i = 1;
    int cnt = numOfTracks();
    while( m_firstTrack ) {
        kDebug() << "(K3b::AudioDoc::AudioDoc) deleting track " << i << " of " << cnt;
        delete m_firstTrack->take();
        kDebug() << "(K3b::AudioDoc::AudioDoc) deleted.";
        ++i;
    }

    delete d;
}

bool K3b::AudioDoc::newDocument()
{
    clear();
    m_normalize = false;
    m_hideFirstTrack = false;
    m_cdText = false;
    m_cdTextData.clear();
    m_audioRippingParanoiaMode = 0;
    m_audioRippingRetries = 5;
    m_audioRippingIgnoreReadErrors = true;

    return K3b::Doc::newDocument();
}


void K3b::AudioDoc::clear()
{
    // delete all tracks
    while( m_firstTrack )
        delete m_firstTrack->take();
}


QString K3b::AudioDoc::name() const
{
    if( !m_cdTextData.title().isEmpty() )
        return m_cdTextData.title();
    else
        return K3b::Doc::name();
}


K3b::AudioTrack* K3b::AudioDoc::firstTrack() const
{
    return m_firstTrack;
}


K3b::AudioTrack* K3b::AudioDoc::lastTrack() const
{
    return m_lastTrack;
}


// this one is called by K3b::AudioTrack to update the list
void K3b::AudioDoc::setFirstTrack( K3b::AudioTrack* track )
{
    m_firstTrack = track;
}

// this one is called by K3b::AudioTrack to update the list
void K3b::AudioDoc::setLastTrack( K3b::AudioTrack* track )
{
    m_lastTrack = track;
}


KIO::filesize_t K3b::AudioDoc::size() const
{
    // This is not really correct but what the user expects ;)
    return length().mode1Bytes();
}


K3b::Msf K3b::AudioDoc::length() const
{
    K3b::Msf length = 0;
    K3b::AudioTrack* track = m_firstTrack;
    while( track ) {
        length += track->length();
        track = track->next();
    }

    return length;
}


void K3b::AudioDoc::addUrls( const KUrl::List& urls )
{
    // make sure we add them at the end even if urls are in the queue
    addTracks( urls, 99 );
}


void K3b::AudioDoc::addTracks( const KUrl::List& urls, int position )
{
    KUrl::List allUrls = extractUrlList( K3b::convertToLocalUrls(urls) );
    KUrl::List::iterator end( allUrls.end());
    for( KUrl::List::iterator it = allUrls.begin(); it != end; it++, position++ ) {
        KUrl& url = *it;
        if( url.toLocalFile().right(3).toLower() == "cue" ) {
            // try adding a cue file
            if( K3b::AudioTrack* newAfter = importCueFile( url.toLocalFile(), getTrack(position) ) ) {
                position = newAfter->trackNumber();
                continue;
            }
        }

        if( K3b::AudioTrack* track = createTrack( url ) ) {
            addTrack( track, position );

            K3b::AudioDecoder* dec = static_cast<K3b::AudioFile*>( track->firstSource() )->decoder();
            track->setTitle( dec->metaInfo( K3b::AudioDecoder::META_TITLE ) );
            track->setArtist( dec->metaInfo( K3b::AudioDecoder::META_ARTIST ) );
            track->setSongwriter( dec->metaInfo( K3b::AudioDecoder::META_SONGWRITER ) );
            track->setComposer( dec->metaInfo( K3b::AudioDecoder::META_COMPOSER ) );
            track->setCdTextMessage( dec->metaInfo( K3b::AudioDecoder::META_COMMENT ) );
        }
    }

    emit changed();

    informAboutNotFoundFiles();
}


KUrl::List K3b::AudioDoc::extractUrlList( const KUrl::List& urls )
{
    KUrl::List allUrls = urls;
    KUrl::List urlsFromPlaylist;
    KUrl::List::iterator it = allUrls.begin();
    while( it != allUrls.end() ) {

        const KUrl& url = *it;
        QFileInfo fi( url.toLocalFile() );

        if( !url.isLocalFile() ) {
            kDebug() << url.toLocalFile() << " no local file";
            it = allUrls.erase( it );
            m_notFoundFiles.append( url );
        }
        else if( !fi.exists() ) {
            it = allUrls.erase( it );
            kDebug() << url.toLocalFile() << " not found";
            m_notFoundFiles.append( url );
        }
        else if( fi.isDir() ) {
            it = allUrls.erase( it );
            // add all files in the dir
            QDir dir(fi.filePath());
            QStringList entries = dir.entryList( QDir::Files );
            KUrl::List::iterator oldIt = it;
            // add all files into the list after the current item
            for( QStringList::iterator dirIt = entries.begin();
                 dirIt != entries.end(); ++dirIt )
                it = allUrls.insert( oldIt, KUrl( dir.absolutePath() + "/" + *dirIt ) );
        }
        else if( readPlaylistFile( url, urlsFromPlaylist ) ) {
            it = allUrls.erase( it );
            KUrl::List::iterator oldIt = it;
            // add all files into the list after the current item
            for( KUrl::List::iterator dirIt = urlsFromPlaylist.begin();
                 dirIt != urlsFromPlaylist.end(); ++dirIt )
                it = allUrls.insert( oldIt, *dirIt );
        }
        else
            ++it;
    }

    return allUrls;
}


bool K3b::AudioDoc::readPlaylistFile( const KUrl& url, KUrl::List& playlist )
{
    // check if the file is a m3u playlist
    // and if so add all listed files

    QFile f( url.toLocalFile() );
    if( !f.open( QIODevice::ReadOnly ) )
        return false;

    QByteArray buf = f.read( 7 );
    if( buf.size() != 7 || QString::fromLatin1( buf ) != "#EXTM3U" )
        return false;
    f.seek( 0 );

    QTextStream t( &f );

    // skip the first line
    t.readLine();

    // read the file
    while( !t.atEnd() ) {
        QString line = t.readLine();
        if( line[0] != '#' ) {
            KUrl mp3url;
            QFileInfo pathInfo(line);
            if (pathInfo.isRelative())
                mp3url.setPath( url.directory(false) + line );
            else
                mp3url.setPath( line );

            playlist.append( mp3url );
        }
    }

    return true;
}


void K3b::AudioDoc::addSources( K3b::AudioTrack* parent,
                              const KUrl::List& urls,
                              K3b::AudioDataSource* sourceAfter )
{
    kDebug() << "(K3b::AudioDoc::addSources( " << parent << ", "
             << urls.first().toLocalFile() << ", "
             << sourceAfter << " )" << endl;
    KUrl::List allUrls = extractUrlList( urls );
    KUrl::List::const_iterator end(allUrls.constEnd());
    for( KUrl::List::const_iterator it = allUrls.constBegin(); it != end; ++it ) {
        if( K3b::AudioFile* file = createAudioFile( *it ) ) {
            if( sourceAfter )
                file->moveAfter( sourceAfter );
            else
                file->moveAhead( parent->firstSource() );
            sourceAfter = file;
        }
    }

    informAboutNotFoundFiles();
    kDebug() << "(K3b::AudioDoc::addSources) finished.";
}


K3b::AudioTrack* K3b::AudioDoc::importCueFile( const QString& cuefile, K3b::AudioTrack* after, K3b::AudioDecoder* decoder )
{
    if( !after )
        after = m_lastTrack;

    kDebug() << "(K3b::AudioDoc::importCueFile( " << cuefile << ", " << after << ")";
    K3b::CueFileParser parser( cuefile );
    if( parser.isValid() && parser.toc().contentType() == K3b::Device::AUDIO ) {

        kDebug() << "(K3b::AudioDoc::importCueFile) parsed with image: " << parser.imageFilename();

        // global cd-text
        if( !parser.cdText().title().isEmpty() )
            setTitle( parser.cdText().title() );
        if( !parser.cdText().performer().isEmpty() )
            setPerformer( parser.cdText().performer() );

        bool isBin = parser.imageFileType() == QLatin1String( "bin" );

        bool reused = true;
        if( !decoder && !isBin )
            if ( !( decoder = getDecoderForUrl( KUrl(parser.imageFilename()), &reused ) ) )
                return 0;

        AudioDataSource* source = 0;
        int i = 0;
        foreach( const K3b::Device::Track& track, parser.toc() ) {
            if ( isBin ) {
                source = new RawAudioDataSource( parser.imageFilename() );
            }
            else {
                if( !reused )
                    decoder->analyseFile();

                source = new K3b::AudioFile( decoder, this );
            }

            source->setStartOffset( track.firstSector() );
            source->setEndOffset( track.lastSector()+1 );

            K3b::AudioTrack* newTrack = new K3b::AudioTrack( this );
            newTrack->addSource( source );
            newTrack->moveAfter( after );
            emit trackAdded(newTrack);

            // we do not know the length of the source yet so we have to force the index value
            if( track.index0() > 0 )
                newTrack->m_index0Offset = track.length() - track.index0();
            else
                newTrack->m_index0Offset = 0;

            // cd-text
            newTrack->setTitle( parser.cdText()[i].title() );
            newTrack->setPerformer( parser.cdText()[i].performer() );

            // add the next track after this one
            after = newTrack;
            ++i;
        }

        // let the last source use the data up to the end of the file
        if( source )
            source->setEndOffset(0);

        return after;
    }

    return 0;
}


K3b::AudioDecoder* K3b::AudioDoc::getDecoderForUrl( const KUrl& url, bool* reused )
{
    K3b::AudioDecoder* decoder = 0;

    // check if we already have a proper decoder
    if( m_decoderPresenceMap.contains( url.toLocalFile() ) ) {
        decoder = m_decoderPresenceMap[url.toLocalFile()];
        *reused = true;
    }
    else if( (decoder = K3b::AudioDecoderFactory::createDecoder( url )) ) {
        kDebug() << "(K3b::AudioDoc) using " << decoder->metaObject()->className()
                 << " for decoding of " << url.toLocalFile() << endl;

        decoder->setFilename( url.toLocalFile() );
        *reused = false;
    }

    return decoder;
}


K3b::AudioFile* K3b::AudioDoc::createAudioFile( const KUrl& url )
{
    if( !QFile::exists( url.toLocalFile() ) ) {
        m_notFoundFiles.append( url.toLocalFile() );
        kDebug() << "(K3b::AudioDoc) could not find file " << url.toLocalFile();
        return 0;
    }

    bool reused;
    K3b::AudioDecoder* decoder = getDecoderForUrl( url, &reused );
    if( decoder ) {
        if( !reused )
            decoder->analyseFile();
        return new K3b::AudioFile( decoder, this );
    }
    else {
        m_unknownFileFormatFiles.append( url.toLocalFile() );
        kDebug() << "(K3b::AudioDoc) unknown file type in file " << url.toLocalFile();
        return 0;
    }
}


K3b::AudioTrack* K3b::AudioDoc::createTrack( const KUrl& url )
{
    kDebug() << "(K3b::AudioDoc::createTrack( " << url.toLocalFile() << " )";
    if( K3b::AudioFile* file = createAudioFile( url ) ) {
        K3b::AudioTrack* newTrack = new K3b::AudioTrack( this );
        newTrack->setFirstSource( file );
        return newTrack;
    }
    else
        return 0;
}


void K3b::AudioDoc::addTrack( const KUrl& url, int position )
{
    addTracks( KUrl::List(url), position );
}



K3b::AudioTrack* K3b::AudioDoc::getTrack( int trackNum )
{
    K3b::AudioTrack* track = m_firstTrack;
    int i = 1;
    while( track ) {
        if( i == trackNum )
            return track;
        track = track->next();
        ++i;
    }

    return 0;
}


void K3b::AudioDoc::addTrack( K3b::AudioTrack* track, int position )
{
    kDebug() << "(K3b::AudioDoc::addTrack( " << track << ", " << position << " )";
    track->m_parent = this;
    if( !m_firstTrack )
        m_firstTrack = m_lastTrack = track;
    else if( position == 0 )
        track->moveAhead( m_firstTrack );
    else {
        K3b::AudioTrack* after = getTrack( position );
        if( after )
            track->moveAfter( after );
        else
            track->moveAfter( m_lastTrack );  // just to be sure it's anywhere...
    }

    emit trackAdded(track);
    emit changed();
}

void K3b::AudioDoc::removeTrack( K3b::AudioTrack* track )
{
    delete track;
}


void K3b::AudioDoc::moveTrack( K3b::AudioTrack* track, K3b::AudioTrack* after )
{
    track->moveAfter( after );
}


bool K3b::AudioDoc::loadDocumentData( QDomElement* root )
{
    newDocument();

    // we will parse the dom-tree and create a K3b::AudioTrack for all entries immediately
    // this should not take long and so not block the gui

    QDomNodeList nodes = root->childNodes();

    for( int i = 0; i < nodes.count(); i++ ) {

        QDomElement e = nodes.item(i).toElement();

        if( e.isNull() )
            return false;

        if( e.nodeName() == "general" ) {
            if( !readGeneralDocumentData( e ) )
                return false;
        }

        else if( e.nodeName() == "normalize" )
            setNormalize( e.text() == "yes" );

        else if( e.nodeName() == "hide_first_track" )
            setHideFirstTrack( e.text() == "yes" );

        else if( e.nodeName() == "audio_ripping" ) {
            QDomNodeList ripNodes = e.childNodes();
            for( uint j = 0; j < ripNodes.length(); j++ ) {
                if( ripNodes.item(j).nodeName() == "paranoia_mode" )
                    setAudioRippingParanoiaMode( ripNodes.item(j).toElement().text().toInt() );
                else if( ripNodes.item(j).nodeName() == "read_retries" )
                    setAudioRippingRetries( ripNodes.item(j).toElement().text().toInt() );
                else if( ripNodes.item(j).nodeName() == "ignore_read_errors" )
                    setAudioRippingIgnoreReadErrors( ripNodes.item(j).toElement().text() == "yes" );
            }
        }

        // parse cd-text
        else if( e.nodeName() == "cd-text" ) {
            if( !e.hasAttribute( "activated" ) )
                return false;

            writeCdText( e.attributeNode( "activated" ).value() == "yes" );

            QDomNodeList cdTextNodes = e.childNodes();
            for( uint j = 0; j < cdTextNodes.length(); j++ ) {
                if( cdTextNodes.item(j).nodeName() == "title" )
                    setTitle( cdTextNodes.item(j).toElement().text() );

                else if( cdTextNodes.item(j).nodeName() == "artist" )
                    setArtist( cdTextNodes.item(j).toElement().text() );

                else if( cdTextNodes.item(j).nodeName() == "arranger" )
                    setArranger( cdTextNodes.item(j).toElement().text() );

                else if( cdTextNodes.item(j).nodeName() == "songwriter" )
                    setSongwriter( cdTextNodes.item(j).toElement().text() );

                else if( cdTextNodes.item(j).nodeName() == "composer" )
                    setComposer( cdTextNodes.item(j).toElement().text() );

                else if( cdTextNodes.item(j).nodeName() == "disc_id" )
                    setDisc_id( cdTextNodes.item(j).toElement().text() );

                else if( cdTextNodes.item(j).nodeName() == "upc_ean" )
                    setUpc_ean( cdTextNodes.item(j).toElement().text() );

                else if( cdTextNodes.item(j).nodeName() == "message" )
                    setCdTextMessage( cdTextNodes.item(j).toElement().text() );
            }
        }

        else if( e.nodeName() == "contents" ) {

            QDomNodeList contentNodes = e.childNodes();

            for( uint j = 0; j< contentNodes.length(); j++ ) {

                QDomElement trackElem = contentNodes.item(j).toElement();

                // first of all we need a track
                K3b::AudioTrack* track = new K3b::AudioTrack();


                // backwards compatibility
                // -----------------------------------------------------------------------------------------------------
                QDomAttr oldUrlAttr = trackElem.attributeNode( "url" );
                if( !oldUrlAttr.isNull() ) {
                    if( K3b::AudioFile* file =
                        createAudioFile( KUrl( oldUrlAttr.value() ) ) ) {
                        track->addSource( file );
                    }
                }
                // -----------------------------------------------------------------------------------------------------


                QDomNodeList trackNodes = trackElem.childNodes();
                for( uint trackJ = 0; trackJ < trackNodes.length(); trackJ++ ) {

                    if( trackNodes.item(trackJ).nodeName() == "sources" ) {
                        QDomNodeList sourcesNodes = trackNodes.item(trackJ).childNodes();
                        for( unsigned int sourcesIndex = 0; sourcesIndex < sourcesNodes.length(); sourcesIndex++ ) {
                            QDomElement sourceElem = sourcesNodes.item(sourcesIndex).toElement();
                            if( sourceElem.nodeName() == "file" ) {
                                if( K3b::AudioFile* file =
                                    createAudioFile( KUrl( sourceElem.attributeNode( "url" ).value() ) ) ) {
                                    file->setStartOffset( K3b::Msf::fromString( sourceElem.attributeNode( "start_offset" ).value() ) );
                                    file->setEndOffset( K3b::Msf::fromString( sourceElem.attributeNode( "end_offset" ).value() ) );
                                    track->addSource( file );
                                }
                            }
                            else if( sourceElem.nodeName() == "silence" ) {
                                K3b::AudioZeroData* zero = new K3b::AudioZeroData();
                                zero->setLength( K3b::Msf::fromString( sourceElem.attributeNode( "length" ).value() ) );
                                track->addSource( zero );
                            }
                            else if( sourceElem.nodeName() == "cdtrack" ) {
                                K3b::Msf length = K3b::Msf::fromString( sourceElem.attributeNode( "length" ).value() );
                                int titlenum = 0;
                                int discid = 0;
                                QString title, artist, cdTitle, cdArtist;

                                QDomNodeList cdTrackSourceNodes = sourceElem.childNodes();
                                for( unsigned int cdTrackSourceIndex = 0; cdTrackSourceIndex < cdTrackSourceNodes.length(); ++cdTrackSourceIndex ) {
                                    QDomElement cdTrackSourceItemElem = cdTrackSourceNodes.item(cdTrackSourceIndex).toElement();
                                    if( cdTrackSourceItemElem.nodeName() == "title_number" )
                                        titlenum = cdTrackSourceItemElem.text().toInt();
                                    else if( cdTrackSourceItemElem.nodeName() == "disc_id" )
                                        discid = cdTrackSourceItemElem.text().toUInt( 0, 16 );
                                    else if( cdTrackSourceItemElem.nodeName() == "title" )
                                        title = cdTrackSourceItemElem.text().toInt();
                                    else if( cdTrackSourceItemElem.nodeName() == "artist" )
                                        artist = cdTrackSourceItemElem.text().toInt();
                                    else if( cdTrackSourceItemElem.nodeName() == "cdtitle" )
                                        cdTitle = cdTrackSourceItemElem.text().toInt();
                                    else if( cdTrackSourceItemElem.nodeName() == "cdartist" )
                                        cdArtist = cdTrackSourceItemElem.text().toInt();
                                }

                                if( discid != 0 && titlenum > 0 ) {
                                    K3b::AudioCdTrackSource* cdtrack = new K3b::AudioCdTrackSource( discid, length, titlenum,
                                                                                                artist, title,
                                                                                                cdArtist, cdTitle );
                                    cdtrack->setStartOffset( K3b::Msf::fromString( sourceElem.attributeNode( "start_offset" ).value() ) );
                                    cdtrack->setEndOffset( K3b::Msf::fromString( sourceElem.attributeNode( "end_offset" ).value() ) );
                                    track->addSource( cdtrack );
                                }
                                else {
                                    kDebug() << "(K3b::AudioDoc) invalid cdtrack source.";
                                    return false;
                                }
                            }
                            else {
                                kDebug() << "(K3b::AudioDoc) unknown source type: " << sourceElem.nodeName();
                                return false;
                            }
                        }
                    }

                    // load cd-text
                    else if( trackNodes.item(trackJ).nodeName() == "cd-text" ) {
                        QDomNodeList cdTextNodes = trackNodes.item(trackJ).childNodes();
                        for( uint trackCdTextJ = 0; trackCdTextJ < cdTextNodes.length(); trackCdTextJ++ ) {
                            if( cdTextNodes.item(trackCdTextJ).nodeName() == "title" )
                                track->setTitle( cdTextNodes.item(trackCdTextJ).toElement().text() );

                            else if( cdTextNodes.item(trackCdTextJ).nodeName() == "artist" )
                                track->setArtist( cdTextNodes.item(trackCdTextJ).toElement().text() );

                            else if( cdTextNodes.item(trackCdTextJ).nodeName() == "arranger" )
                                track->setArranger( cdTextNodes.item(trackCdTextJ).toElement().text() );

                            else if( cdTextNodes.item(trackCdTextJ).nodeName() == "songwriter" )
                                track->setSongwriter( cdTextNodes.item(trackCdTextJ).toElement().text() );

                            else if( cdTextNodes.item(trackCdTextJ).nodeName() == "composer" )
                                track->setComposer( cdTextNodes.item(trackCdTextJ).toElement().text() );

                            else if( cdTextNodes.item(trackCdTextJ).nodeName() == "isrc" )
                                track->setIsrc( cdTextNodes.item(trackCdTextJ).toElement().text() );

                            else if( cdTextNodes.item(trackCdTextJ).nodeName() == "message" )
                                track->setCdTextMessage( cdTextNodes.item(trackCdTextJ).toElement().text() );
                        }
                    }

                    else if( trackNodes.item(trackJ).nodeName() == "index0" )
                        track->setIndex0( K3b::Msf::fromString( trackNodes.item(trackJ).toElement().text() ) );

                    // TODO: load other indices

                    // load options
                    else if( trackNodes.item(trackJ).nodeName() == "copy_protection" )
                        track->setCopyProtection( trackNodes.item(trackJ).toElement().text() == "yes" );

                    else if( trackNodes.item(trackJ).nodeName() == "pre_emphasis" )
                        track->setPreEmp( trackNodes.item(trackJ).toElement().text() == "yes" );
                }

                // add the track
                if( track->numberSources() > 0 )
                    addTrack( track, 99 ); // append to the end // TODO improve
                else {
                    kDebug() << "(K3b::AudioDoc) no sources. deleting track " << track;
                    delete track;
                }
            }
        }
    }

    informAboutNotFoundFiles();

    setModified(false);

    return true;
}

bool K3b::AudioDoc::saveDocumentData( QDomElement* docElem )
{
    QDomDocument doc = docElem->ownerDocument();
    saveGeneralDocumentData( docElem );

    // add normalize
    QDomElement normalizeElem = doc.createElement( "normalize" );
    normalizeElem.appendChild( doc.createTextNode( normalize() ? "yes" : "no" ) );
    docElem->appendChild( normalizeElem );

    // add hide track
    QDomElement hideFirstTrackElem = doc.createElement( "hide_first_track" );
    hideFirstTrackElem.appendChild( doc.createTextNode( hideFirstTrack() ? "yes" : "no" ) );
    docElem->appendChild( hideFirstTrackElem );

    // save the audio cd ripping settings
    // paranoia mode, read retries, and ignore read errors
    // ------------------------------------------------------------
    QDomElement ripMain = doc.createElement( "audio_ripping" );
    docElem->appendChild( ripMain );

    QDomElement ripElem = doc.createElement( "paranoia_mode" );
    ripElem.appendChild( doc.createTextNode( QString::number( audioRippingParanoiaMode() ) ) );
    ripMain.appendChild( ripElem );

    ripElem = doc.createElement( "read_retries" );
    ripElem.appendChild( doc.createTextNode( QString::number( audioRippingRetries() ) ) );
    ripMain.appendChild( ripElem );

    ripElem = doc.createElement( "ignore_read_errors" );
    ripElem.appendChild( doc.createTextNode( audioRippingIgnoreReadErrors() ? "yes" : "no" ) );
    ripMain.appendChild( ripElem );
    // ------------------------------------------------------------

    // save disc cd-text
    // -------------------------------------------------------------
    QDomElement cdTextMain = doc.createElement( "cd-text" );
    cdTextMain.setAttribute( "activated", cdText() ? "yes" : "no" );
    QDomElement cdTextElem = doc.createElement( "title" );
    cdTextElem.appendChild( doc.createTextNode( (title())) );
    cdTextMain.appendChild( cdTextElem );

    cdTextElem = doc.createElement( "artist" );
    cdTextElem.appendChild( doc.createTextNode( (artist())) );
    cdTextMain.appendChild( cdTextElem );

    cdTextElem = doc.createElement( "arranger" );
    cdTextElem.appendChild( doc.createTextNode( (arranger())) );
    cdTextMain.appendChild( cdTextElem );

    cdTextElem = doc.createElement( "songwriter" );
    cdTextElem.appendChild( doc.createTextNode( (songwriter())) );
    cdTextMain.appendChild( cdTextElem );

    cdTextElem = doc.createElement( "composer" );
    cdTextElem.appendChild( doc.createTextNode( composer()) );
    cdTextMain.appendChild( cdTextElem );

    cdTextElem = doc.createElement( "disc_id" );
    cdTextElem.appendChild( doc.createTextNode( (disc_id())) );
    cdTextMain.appendChild( cdTextElem );

    cdTextElem = doc.createElement( "upc_ean" );
    cdTextElem.appendChild( doc.createTextNode( (upc_ean())) );
    cdTextMain.appendChild( cdTextElem );

    cdTextElem = doc.createElement( "message" );
    cdTextElem.appendChild( doc.createTextNode( (cdTextMessage())) );
    cdTextMain.appendChild( cdTextElem );

    docElem->appendChild( cdTextMain );
    // -------------------------------------------------------------

    // save the tracks
    // -------------------------------------------------------------
    QDomElement contentsElem = doc.createElement( "contents" );

    for( K3b::AudioTrack* track = firstTrack(); track != 0; track = track->next() ) {

        QDomElement trackElem = doc.createElement( "track" );

        // add sources
        QDomElement sourcesParent = doc.createElement( "sources" );

        for( K3b::AudioDataSource* source = track->firstSource(); source; source = source->next() ) {
            // TODO: save a source element with a type attribute and start- and endoffset
            //       then distict between the different source types.
            if( K3b::AudioFile* file = dynamic_cast<K3b::AudioFile*>(source) ) {
                QDomElement sourceElem = doc.createElement( "file" );
                sourceElem.setAttribute( "url", file->filename() );
                sourceElem.setAttribute( "start_offset", file->startOffset().toString() );
                sourceElem.setAttribute( "end_offset", file->endOffset().toString() );
                sourcesParent.appendChild( sourceElem );
            }
            else if( K3b::AudioZeroData* zero = dynamic_cast<K3b::AudioZeroData*>(source) ) {
                QDomElement sourceElem = doc.createElement( "silence" );
                sourceElem.setAttribute( "length", zero->length().toString() );
                sourcesParent.appendChild( sourceElem );
            }
            else if( K3b::AudioCdTrackSource* cdTrack = dynamic_cast<K3b::AudioCdTrackSource*>(source) ) {
                QDomElement sourceElem = doc.createElement( "cdtrack" );
                sourceElem.setAttribute( "length", cdTrack->originalLength().toString() );
                sourceElem.setAttribute( "start_offset", cdTrack->startOffset().toString() );
                sourceElem.setAttribute( "end_offset", cdTrack->endOffset().toString() );

                QDomElement subElem = doc.createElement( "title_number" );
                subElem.appendChild( doc.createTextNode( QString::number(cdTrack->cdTrackNumber()) ) );
                sourceElem.appendChild( subElem );

                subElem = doc.createElement( "disc_id" );
                subElem.appendChild( doc.createTextNode( QString::number(cdTrack->discId(), 16) ) );
                sourceElem.appendChild( subElem );

                subElem = doc.createElement( "title" );
                subElem.appendChild( doc.createTextNode( cdTrack->title() ) );
                sourceElem.appendChild( subElem );

                subElem = doc.createElement( "artist" );
                subElem.appendChild( doc.createTextNode( cdTrack->artist() ) );
                sourceElem.appendChild( subElem );

                subElem = doc.createElement( "cdtitle" );
                subElem.appendChild( doc.createTextNode( cdTrack->cdTitle() ) );
                sourceElem.appendChild( subElem );

                subElem = doc.createElement( "cdartist" );
                subElem.appendChild( doc.createTextNode( cdTrack->cdArtist() ) );
                sourceElem.appendChild( subElem );

                sourcesParent.appendChild( sourceElem );
            }
            else {
                kError() << "(K3b::AudioDoc) saving sources other than file or zero not supported yet." << endl;
                return false;
            }
        }
        trackElem.appendChild( sourcesParent );

        // index 0
        QDomElement index0Elem = doc.createElement( "index0" );
        index0Elem.appendChild( doc.createTextNode( track->index0().toString() ) );
        trackElem.appendChild( index0Elem );

        // TODO: other indices

        // add cd-text
        cdTextMain = doc.createElement( "cd-text" );
        cdTextElem = doc.createElement( "title" );
        cdTextElem.appendChild( doc.createTextNode( (track->title())) );
        cdTextMain.appendChild( cdTextElem );

        cdTextElem = doc.createElement( "artist" );
        cdTextElem.appendChild( doc.createTextNode( (track->artist())) );
        cdTextMain.appendChild( cdTextElem );

        cdTextElem = doc.createElement( "arranger" );
        cdTextElem.appendChild( doc.createTextNode( (track->arranger()) ) );
        cdTextMain.appendChild( cdTextElem );

        cdTextElem = doc.createElement( "songwriter" );
        cdTextElem.appendChild( doc.createTextNode( (track->songwriter()) ) );
        cdTextMain.appendChild( cdTextElem );

        cdTextElem = doc.createElement( "composer" );
        cdTextElem.appendChild( doc.createTextNode( (track->composer()) ) );
        cdTextMain.appendChild( cdTextElem );

        cdTextElem = doc.createElement( "isrc" );
        cdTextElem.appendChild( doc.createTextNode( ( track->isrc()) ) );
        cdTextMain.appendChild( cdTextElem );

        cdTextElem = doc.createElement( "message" );
        cdTextElem.appendChild( doc.createTextNode( (track->cdTextMessage()) ) );
        cdTextMain.appendChild( cdTextElem );

        trackElem.appendChild( cdTextMain );

        // add copy protection
        QDomElement copyElem = doc.createElement( "copy_protection" );
        copyElem.appendChild( doc.createTextNode( track->copyProtection() ? "yes" : "no" ) );
        trackElem.appendChild( copyElem );

        // add pre emphasis
        copyElem = doc.createElement( "pre_emphasis" );
        copyElem.appendChild( doc.createTextNode( track->preEmp() ? "yes" : "no" ) );
        trackElem.appendChild( copyElem );

        contentsElem.appendChild( trackElem );
    }
    // -------------------------------------------------------------

    docElem->appendChild( contentsElem );

    return true;
}


int K3b::AudioDoc::numOfTracks() const
{
    return ( m_lastTrack ? m_lastTrack->trackNumber() : 0 );
}


K3b::BurnJob* K3b::AudioDoc::newBurnJob( K3b::JobHandler* hdl, QObject* parent )
{
    return new K3b::AudioJob( this, hdl, parent );
}


void K3b::AudioDoc::informAboutNotFoundFiles()
{
    if( !m_notFoundFiles.isEmpty() ) {
        QStringList l;
        for( KUrl::List::const_iterator it = m_notFoundFiles.constBegin();
             it != m_notFoundFiles.constEnd(); ++it )
            l.append( (*it).toLocalFile() );
        KMessageBox::informationList( qApp->activeWindow(),
                                      i18n("Could not find the following files:"),
                                      l,
                                      i18n("Not Found") );

        m_notFoundFiles.clear();
    }
    if( !m_unknownFileFormatFiles.isEmpty() ) {
        QStringList l;
        for( KUrl::List::const_iterator it = m_unknownFileFormatFiles.constBegin();
             it != m_unknownFileFormatFiles.constEnd(); ++it )
            l.append( (*it).toLocalFile() );
        KMessageBox::informationList( qApp->activeWindow(),
                                      i18n("<p>Unable to handle the following files due to an unsupported format:"
                                           "<p>You may manually convert these audio files to wave using another "
                                           "application supporting the audio format and then add the wave files "
                                           "to the K3b project."),
                                      l,
                                      i18n("Unsupported Format") );

        m_unknownFileFormatFiles.clear();
    }
}



void K3b::AudioDoc::removeCorruptTracks()
{
//   K3b::AudioTrack* track = m_tracks->first();
//   while( track ) {
//     if( track->status() != 0 ) {
//       removeTrack(track);
//       track = m_tracks->current();
//     }
//     else
//       track = m_tracks->next();
//   }
}


void K3b::AudioDoc::slotTrackChanged( K3b::AudioTrack* track )
{
    kDebug() << "(K3b::AudioDoc::slotTrackChanged " << track;
    setModified( true );
    // if the track is empty now we simply delete it
    if( track->firstSource() ) {
        emit trackChanged(track);
        emit changed();
    }
    else {
        kDebug() << "(K3b::AudioDoc::slotTrackChanged) track " << track << " empty. Deleting.";
        delete track; // this will emit the proper signal
    }
    kDebug() << "(K3b::AudioDoc::slotTrackChanged done";
}


void K3b::AudioDoc::slotTrackRemoved( K3b::AudioTrack* track )
{
    setModified( true );
    emit trackRemoved(track);
    emit changed();
}

void K3b::AudioDoc::increaseDecoderUsage( K3b::AudioDecoder* decoder )
{
    kDebug() << "(K3b::AudioDoc::increaseDecoderUsage)";
    if( !m_decoderUsageCounterMap.contains( decoder ) ) {
        m_decoderUsageCounterMap[decoder] = 1;
        m_decoderPresenceMap[decoder->filename()] = decoder;
    }
    else
        m_decoderUsageCounterMap[decoder]++;
    kDebug() << "(K3b::AudioDoc::increaseDecoderUsage) finished";
}


void K3b::AudioDoc::decreaseDecoderUsage( K3b::AudioDecoder* decoder )
{
    m_decoderUsageCounterMap[decoder]--;
    if( m_decoderUsageCounterMap[decoder] <= 0 ) {
        m_decoderUsageCounterMap.remove(decoder);
        m_decoderPresenceMap.remove(decoder->filename());
        delete decoder;
    }
}


K3b::Device::CdText K3b::AudioDoc::cdTextData() const
{
    K3b::Device::CdText text( m_cdTextData );
    K3b::AudioTrack* track = firstTrack();
    int i = 0;
    while( track ) {
        text.track( i++ ) = track->cdText();
        track = track->next();
    }
    return text;
}


K3b::Device::Toc K3b::AudioDoc::toToc() const
{
    K3b::Device::Toc toc;

    // FIXME: add MCN

    K3b::AudioTrack* track = firstTrack();
    K3b::Msf pos = 0;
    while( track ) {
        toc.append( track->toCdTrack() );
        track = track->next();
    }

    return toc;
}


void K3b::AudioDoc::setTitle( const QString& v )
{
    m_cdTextData.setTitle( v );
    emit changed();
}


void K3b::AudioDoc::setArtist( const QString& v )
{
    setPerformer( v );
}


void K3b::AudioDoc::setPerformer( const QString& v )
{
    QString s( v );
    d->cdTextValidator->fixup( s );
    m_cdTextData.setPerformer( s );
    emit changed();
}


void K3b::AudioDoc::setDisc_id( const QString& v )
{
    QString s( v );
    d->cdTextValidator->fixup( s );
    m_cdTextData.setDiscId( s );
    emit changed();
}


void K3b::AudioDoc::setArranger( const QString& v )
{
    QString s( v );
    d->cdTextValidator->fixup( s );
    m_cdTextData.setArranger( s );
    emit changed();
}


void K3b::AudioDoc::setSongwriter( const QString& v )
{
    QString s( v );
    d->cdTextValidator->fixup( s );
    m_cdTextData.setSongwriter( s );
    emit changed();
}


void K3b::AudioDoc::setComposer( const QString& v )
{
    QString s( v );
    d->cdTextValidator->fixup( s );
    m_cdTextData.setComposer( s );
    emit changed();
}


void K3b::AudioDoc::setUpc_ean( const QString& v )
{
    QString s( v );
    d->cdTextValidator->fixup( s );
    m_cdTextData.setUpcEan( s );
    emit changed();
}


void K3b::AudioDoc::setCdTextMessage( const QString& v )
{
    QString s( v );
    d->cdTextValidator->fixup( s );
    m_cdTextData.setMessage( s );
    emit changed();
}


K3b::Device::MediaTypes K3b::AudioDoc::supportedMediaTypes() const
{
    return K3b::Device::MEDIA_WRITABLE_CD;
}

#include "k3baudiodoc.moc"
