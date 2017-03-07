/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bcdtext.h"
#include "k3bcrc.h"

#include <config-k3b.h>

#include <QtCore/QDebug>
#include <QtCore/QSharedData>
#include <QtCore/QTextCodec>

#include <string.h>


namespace {

    struct cdtext_pack {
        unsigned char id1;
        unsigned char id2;
        unsigned char id3;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
        unsigned char dbcc:       1;
        unsigned char blocknum:   3;
        unsigned char charpos:    4;
#else
        unsigned char charpos:    4;
        unsigned char blocknum:   3;
        unsigned char dbcc:       1;
#endif
        unsigned char data[12];
        unsigned char crc[2];
    };

    /**
     * This one is taken from cdrecord
     */
    struct text_size_block {
        char charcode;
        char first_track;
        char last_track;
        char copyr_flags;
        char pack_count[16];
        char last_seqnum[8];
        char language_codes[8];
    };

    void debugRawTextPackData( const unsigned char* data, int dataLen )
    {
        qDebug() << endl << " id1    | id2    | id3    | charps | blockn | dbcc | data           | crc |";

        cdtext_pack* pack = (cdtext_pack*)data;

        for( int i = 0; i < dataLen/18; ++i ) {
            QString s;
            s += QString( " %1 |" ).arg( pack[i].id1, 6, 16 );
            s += QString( " %1 |" ).arg( pack[i].id2, 6 );
            s += QString( " %1 |" ).arg( pack[i].id3, 6 );
            s += QString( " %1 |" ).arg( pack[i].charpos, 6 );
            s += QString( " %1 |" ).arg( pack[i].blocknum, 6 );
            s += QString( " %1 |" ).arg( pack[i].dbcc, 4 );
//       char str[12];
//       sprintf( str, "%c%c%c%c%c%c%c%c%c%c%c%c",
// 	       pack[i].data[0] == '\0' ? '�' : pack[i].data[0],
// 	       pack[i].data[1] == '\0' ? '�' : pack[i].data[1],
// 	       pack[i].data[2] == '\0' ? '�' : pack[i].data[2],
// 	       pack[i].data[3] == '\0' ? '�' : pack[i].data[3],
// 	       pack[i].data[4] == '\0' ? '�' : pack[i].data[4],
// 	       pack[i].data[5] == '\0' ? '�' : pack[i].data[5],
// 	       pack[i].data[6] == '\0' ? '�' : pack[i].data[6],
// 	       pack[i].data[7] == '\0' ? '�' : pack[i].data[7],
// 	       pack[i].data[8] == '\0' ? '�' : pack[i].data[8],
// 	       pack[i].data[9] == '\0' ? '�' : pack[i].data[9],
// 	       pack[i].data[10] == '\0' ? '�' : pack[i].data[10],
// 	       pack[i].data[11] == '\0' ? '�' : pack[i].data[11] );
//       s += QString( " %1 |" ).arg( "'" + QCString(str,13) + "'", 14 );
//       quint16 crc = pack[i].crc[0]<<8|pack[i].crc[1];
//       s += QString( " %1 |" ).arg( crc );
            qDebug() << s;
        }
    }

    // TODO: remove this (see above)
    void fixup( QString& s )
    {
        s.replace( '/', "_" );
        s.replace( '\"', "_" );
    }

    void savePack( cdtext_pack* pack, QByteArray& data, int& dataFill )
    {
        // create CRC
        quint16 crc = K3b::Device::calcX25( reinterpret_cast<unsigned char*>(pack), sizeof(cdtext_pack)-2 );

        // invert for Redbook compliance
        crc ^= 0xffff;

        pack->crc[0] = (crc>>8) & 0xff;
        pack->crc[1] = crc & 0xff;


        // append the pack to data
        if( data.size() < dataFill + ( int )sizeof(cdtext_pack) )
            data.resize( dataFill + sizeof(cdtext_pack) );

        ::memcpy( &data.data()[dataFill], reinterpret_cast<char*>( pack ), sizeof(cdtext_pack) );

        dataFill += sizeof(cdtext_pack);
    }


    void appendByteArray( QByteArray& a, const QByteArray& b )
    {
        int oldSize = a.size();
        a.resize( oldSize + b.size() );
        ::memcpy( &a.data()[oldSize], b.data(), b.size() );
    }


    QByteArray encodeCdText( const QString& s, bool* illegalChars = 0 )
    {
        if( illegalChars )
            *illegalChars = false;

        // TODO: do this without QT
        QTextCodec* codec = QTextCodec::codecForName("ISO8859-1");
        if( codec ) {
            QByteArray encoded = codec->fromUnicode( s );
            return encoded;
        }
        else {
            QByteArray r( s.length()+1, 0 );

            for( int i = 0; i < s.length(); ++i ) {
                if( s[i].toLatin1() == 0 ) { // non-ASCII character
                    r[i] = ' ';
                    if( illegalChars )
                        *illegalChars = true;
                }
                else
                    r[i] = s[i].toLatin1();
            }

            return r;
        }
    }
}


class K3b::Device::TrackCdText::Private : public QSharedData
{
public:
    QString title;
    QString performer;
    QString songwriter;
    QString composer;
    QString arranger;
    QString message;
    QString isrc;
};


K3b::Device::TrackCdText::TrackCdText()
    : d( new Private() )
{
}


K3b::Device::TrackCdText::TrackCdText( const TrackCdText& other )
{
    d = other.d;
}


K3b::Device::TrackCdText::~TrackCdText()
{
}


K3b::Device::TrackCdText& K3b::Device::TrackCdText::operator=( const TrackCdText& other )
{
    d = other.d;
    return *this;
}


void K3b::Device::TrackCdText::clear()
{
    d->title.truncate(0);
    d->performer.truncate(0);
    d->songwriter.truncate(0);
    d->composer.truncate(0);
    d->arranger.truncate(0);
    d->message.truncate(0);
    d->isrc.truncate(0);
}


QString K3b::Device::TrackCdText::title() const
{
    return d->title;
}


QString K3b::Device::TrackCdText::performer() const
{
    return d->performer;
}


QString K3b::Device::TrackCdText::songwriter() const
{
    return d->songwriter;
}


QString K3b::Device::TrackCdText::composer() const
{
    return d->composer;
}


QString K3b::Device::TrackCdText::arranger() const
{
    return d->arranger;
}


QString K3b::Device::TrackCdText::message() const
{
    return d->message;
}


QString K3b::Device::TrackCdText::isrc() const
{
    return d->isrc;
}


void K3b::Device::TrackCdText::setTitle( const QString& s )
{
    d->title = s;
    fixup(d->title);
}


void K3b::Device::TrackCdText::setPerformer( const QString& s )
{
    d->performer = s;
    fixup(d->performer);
}


void K3b::Device::TrackCdText::setSongwriter( const QString& s )
{
    d->songwriter = s;
    fixup(d->songwriter);
}


void K3b::Device::TrackCdText::setComposer( const QString& s )
{
    d->composer = s;
    fixup(d->composer);
}


void K3b::Device::TrackCdText::setArranger( const QString& s )
{
    d->arranger = s;
    fixup(d->arranger);
}


void K3b::Device::TrackCdText::setMessage( const QString& s )
{
    d->message = s;
    fixup(d->message);
}


void K3b::Device::TrackCdText::setIsrc( const QString& s )
{
    d->isrc = s;
    fixup(d->isrc);
}


bool K3b::Device::TrackCdText::isEmpty() const
{
    if( !d->title.isEmpty() )
        return false;
    if( !d->performer.isEmpty() )
        return false;
    if( !d->songwriter.isEmpty() )
        return false;
    if( !d->composer.isEmpty() )
        return false;
    if( !d->arranger.isEmpty() )
        return false;
    if( !d->message.isEmpty() )
        return false;
    if( !d->isrc.isEmpty() )
        return false;

    return true;
}


bool K3b::Device::TrackCdText::operator==( const K3b::Device::TrackCdText& other ) const
{
    return( d->title == other.d->title &&
            d->performer == other.d->performer &&
            d->songwriter == other.d->songwriter &&
            d->composer == other.d->composer &&
            d->arranger == other.d->arranger &&
            d->message == other.d->message &&
            d->isrc == other.d->isrc );
}


bool K3b::Device::TrackCdText::operator!=( const K3b::Device::TrackCdText& other ) const
{
    return !operator==( other );
}



// TODO: use the real CD-TEXT charset (a modified ISO8859-1)

class K3b::Device::CdText::Private : public QSharedData
{
public:
    Private() {
    }

    Private( const Private& other )
        : QSharedData( other ),
          title(other.title),
          performer(other.performer),
          songwriter(other.songwriter),
          composer(other.composer),
          arranger(other.arranger),
          message(other.message),
          discId(other.discId),
          upcEan(other.upcEan),
          tracks(other.tracks) {
        // do not copy rawData. it needs to be recreated if the cdtext changes
    }

    QString title;
    QString performer;
    QString songwriter;
    QString composer;
    QString arranger;
    QString message;
    QString discId;
    QString upcEan;

    QList<TrackCdText> tracks;

    mutable QByteArray rawData;

    QString textForPackType( int packType, int track ) const;
    int textLengthForPackType( int packType ) const;
    QByteArray createPackData( int packType, int& ) const;
};


K3b::Device::CdText::CdText()
    : d( new Private() )
{
}


K3b::Device::CdText::CdText( const K3b::Device::CdText& text )
{
    d = text.d;
}


K3b::Device::CdText::CdText( const unsigned char* data, int len )
    : d( new Private() )
{
    setRawPackData( data, len );
}


K3b::Device::CdText::CdText( const QByteArray& b )
    : d( new Private() )
{
    setRawPackData( b );
}


K3b::Device::CdText::~CdText()
{
}


K3b::Device::CdText& K3b::Device::CdText::operator=( const CdText& other )
{
    d = other.d;
    return *this;
}


K3b::Device::TrackCdText K3b::Device::CdText::operator[]( int i ) const
{
    return d->tracks[i];
}


K3b::Device::TrackCdText& K3b::Device::CdText::operator[]( int i )
{
    return track( i );
}


int K3b::Device::CdText::count() const
{
    return d->tracks.count();
}


K3b::Device::TrackCdText K3b::Device::CdText::track( int i ) const
{
    return d->tracks[i];
}


K3b::Device::TrackCdText& K3b::Device::CdText::track( int i )
{
    while ( i >= d->tracks.count() ) {
        d->tracks.append( TrackCdText() );
    }
    return d->tracks[i];
}


void K3b::Device::CdText::insert( int index, const TrackCdText& tt )
{
    d->tracks.insert( index, tt );
}


void K3b::Device::CdText::clear()
{
    d->tracks.clear();

    d->title.clear();
    d->performer.clear();
    d->songwriter.clear();
    d->composer.clear();
    d->arranger.clear();
    d->message.clear();
    d->discId.clear();
    d->upcEan.clear();
}


bool K3b::Device::CdText::empty() const
{
    if( !d->title.isEmpty() )
        return false;
    if( !d->performer.isEmpty() )
        return false;
    if( !d->songwriter.isEmpty() )
        return false;
    if( !d->composer.isEmpty() )
        return false;
    if( !d->arranger.isEmpty() )
        return false;
    if( !d->message.isEmpty() )
        return false;
    if( !d->discId.isEmpty() )
        return false;
    if( !d->upcEan.isEmpty() )
        return false;

    for( int i = 0; i < count(); ++i )
        if( !d->tracks.at(i).isEmpty() )
            return false;

    return true;
}


bool K3b::Device::CdText::isEmpty() const
{
    return empty();
}


QString K3b::Device::CdText::title() const
{
    return d->title;
}


QString K3b::Device::CdText::performer() const
{
    return d->performer;
}


QString K3b::Device::CdText::songwriter() const
{
    return d->songwriter;
}


QString K3b::Device::CdText::composer() const
{
    return d->composer;
}


QString K3b::Device::CdText::arranger() const
{
    return d->arranger;
}


QString K3b::Device::CdText::message() const
{
    return d->message;
}


QString K3b::Device::CdText::discId() const
{
    return d->discId;
}


QString K3b::Device::CdText::upcEan() const
{
    return d->upcEan;
}


void K3b::Device::CdText::setTitle( const QString& s )
{
    d->title = s;
    fixup(d->title);
}


void K3b::Device::CdText::setPerformer( const QString& s )
{
    d->performer = s;
    fixup(d->performer);
}


void K3b::Device::CdText::setSongwriter( const QString& s )
{
    d->songwriter = s;
    fixup(d->songwriter);
}


void K3b::Device::CdText::setComposer( const QString& s )
{
    d->composer = s;
    fixup(d->composer);
}


void K3b::Device::CdText::setArranger( const QString& s )
{
    d->arranger = s;
    fixup(d->arranger);
}


void K3b::Device::CdText::setMessage( const QString& s )
{
    d->message = s;
    fixup(d->message);
}


void K3b::Device::CdText::setDiscId( const QString& s )
{
    d->discId = s;
    fixup(d->discId);
}


void K3b::Device::CdText::setUpcEan( const QString& s )
{
    d->upcEan = s;
    fixup(d->upcEan);
}


void K3b::Device::CdText::setRawPackData( const unsigned char* data, int len )
{
    clear();

    int r = len%18;
    if( r > 0 && r != 4 ) {
        qDebug() << "(K3b::Device::CdText) invalid cdtext size: " << len;
    }
    else if( len-r > 0 ) {
        debugRawTextPackData( &data[r], len-r );

        cdtext_pack* pack = (cdtext_pack*)&data[r];


        for( int i = 0; i < (len-r)/18; ++i ) {

            if( pack[i].dbcc ) {
                qDebug() << "(K3b::Device::CdText) Double byte code not supported";
                return;
            }

            //
            // For some reason all crc bits are inverted.
            //
            pack[i].crc[0] ^= 0xff;
            pack[i].crc[1] ^= 0xff;

            quint16 crc = calcX25( reinterpret_cast<unsigned char*>(&pack[i]), 18 );

            pack[i].crc[0] ^= 0xff;
            pack[i].crc[1] ^= 0xff;

            if( crc != 0x0000 )
                qDebug() << "(K3b::Device::CdText) CRC invalid!";


            //
            // pack.data has a length of 12
            //
            // id1 tells us the tracknumber of the data (0 for global)
            // data may contain multiple \0. In that case after every \0 the track number increases 1
            //

            char* nullPos = (char*)pack[i].data - 1;

            int trackNo = pack[i].id2;

            while( nullPos ) {
                char* nextNullPos = (char*)::memchr( nullPos+1, '\0', 11 - (nullPos - (char*)pack[i].data) );
                QString txtstr;
                if( nextNullPos ) // take all chars up to the next null
                    txtstr = QString::fromLatin1( (char*)nullPos+1, nextNullPos - nullPos - 1 );
                else // take all chars to the end of the pack data (12 bytes)
                    txtstr = QString::fromLatin1( (char*)nullPos+1, 11 - (nullPos - (char*)pack[i].data) );

                //
                // a tab character means to use the same as for the previous track
                //
                if( txtstr == "\t" )
                    txtstr = d->textForPackType( pack[i].id1, trackNo-1 );

                switch( pack[i].id1 ) {
                case 0x80: // Title
                    if( trackNo == 0 )
                        d->title.append( txtstr );
                    else
                        track(trackNo-1).d->title.append( txtstr );
                    break;

                case 0x81: // Performer
                    if( trackNo == 0 )
                        d->performer.append( txtstr );
                    else
                        track(trackNo-1).d->performer.append( txtstr );
                    break;

                case 0x82: // Writer
                    if( trackNo == 0 )
                        d->songwriter.append( txtstr );
                    else
                        track(trackNo-1).d->songwriter.append( txtstr );
                    break;

                case 0x83: // Composer
                    if( trackNo == 0 )
                        d->composer.append( txtstr );
                    else
                        track(trackNo-1).d->composer.append( txtstr );
                    break;

                case 0x84: // Arranger
                    if( trackNo == 0 )
                        d->arranger.append( txtstr );
                    else
                        track(trackNo-1).d->arranger.append( txtstr );
                    break;

                case 0x85: // Message
                    if( trackNo == 0 )
                        d->message.append( txtstr );
                    else
                        track(trackNo-1).d->message.append( txtstr );
                    break;

                case 0x86: // Disc identification
                    // only global
                    if( trackNo == 0 )
                        d->discId.append( txtstr );
                    break;

                case 0x8e: // Upc or isrc
                    if( trackNo == 0 )
                        d->upcEan.append( txtstr );
                    else
                        track(trackNo-1).d->isrc.append( txtstr );
                    break;

                    // TODO: support for binary data
                    // 0x88: TOC
                    // 0x89: second TOC
                    // 0x8f: Size information

                default:
                    break;
                }

                trackNo++;
                nullPos = nextNullPos;
            }
        }

        // remove empty fields at the end
        while( !d->tracks.isEmpty() && d->tracks.last().isEmpty() ) {
            d->tracks.removeLast();
        }

        // we preserve the original data for clean 1-to-1 copies
        d->rawData = QByteArray( reinterpret_cast<const char*>(data), len );
    }
    else
        qDebug() << "(K3b::Device::CdText) zero-sized CD-TEXT: " << len;
}


void K3b::Device::CdText::setRawPackData( const QByteArray& b )
{
    setRawPackData( reinterpret_cast<const unsigned char*>(b.data()), b.size() );
}

QByteArray K3b::Device::CdText::rawPackData() const
{
    if( d->rawData.isEmpty() ) {
        // FIXME: every pack block may only consist of up to 255 packs.

        int pc = 0;
        int alreadyCountedPacks = 0;


        //
        // prepare the size information block
        //
        text_size_block tsize;
        ::memset( &tsize, 0, sizeof(text_size_block) );
        tsize.charcode = 0;              // ISO 8859-1
        tsize.first_track = 1;
        tsize.last_track = count();
        tsize.pack_count[0xF] = 3;
        tsize.language_codes[0] = 0x09;  // English (from cdrecord)


        //
        // create the CD-Text packs
        //
        QByteArray data;
        for( int i = 0; i <= 6; ++i ) {
            if( d->textLengthForPackType( 0x80 | i ) ) {
                appendByteArray( data, d->createPackData( 0x80 | i, pc ) );
                tsize.pack_count[i] = pc - alreadyCountedPacks;
                alreadyCountedPacks = pc;
            }
        }
        if( d->textLengthForPackType( 0x8E ) ) {
            appendByteArray( data, d->createPackData( 0x8E, pc ) );
            tsize.pack_count[0xE] = pc - alreadyCountedPacks;
            alreadyCountedPacks = pc;
        }


        // pc is the number of the next pack and we add 3 size packs
        tsize.last_seqnum[0] = pc + 2;


        //
        // create the size info packs
        //
        int dataFill = data.size();
        data.resize( data.size() + 3 * sizeof(cdtext_pack) );
        for( int i = 0; i < 3; ++i ) {
            cdtext_pack pack;
            ::memset( &pack, 0, sizeof(cdtext_pack) );
            pack.id1 = 0x8F;
            pack.id2 = i;
            pack.id3 = pc+i;
            ::memcpy( pack.data, &reinterpret_cast<char*>(&tsize)[i*12], 12 );
            savePack( &pack, data, dataFill );
        }

        //
        // add MMC header
        //
        QByteArray a( 4, 0 );
        a[0] = (data.size()+2)>>8 & 0xff;
        a[1] = (data.size()+2) & 0xff;
        a[2] = a[3] = 0;
        appendByteArray( a, data );

        d->rawData = a;
    }

    return d->rawData;
}


// this method also creates completely empty packs
QByteArray K3b::Device::CdText::Private::createPackData( int packType, int& packCount ) const
{
    QByteArray data;
    int dataFill = 0;
    QByteArray text = encodeCdText( textForPackType( packType, 0 ) );
    int currentTrack = 0;
    unsigned int textPos = 0;
    unsigned int packPos = 0;

    //
    // initialize the first pack
    //
    cdtext_pack pack;
    ::memset( &pack, 0, sizeof(cdtext_pack) );
    pack.id1 = packType;
    pack.id3 = packCount;

    //
    // We break this loop when all texts have been packed
    //
    while( 1 ) {
        //
        // Copy as many bytes as possible into the pack
        //
        size_t copyBytes = qMin(12 - packPos, text.length() - textPos);
        if (copyBytes) {
            ::memcpy(reinterpret_cast<char*>(&pack.data[packPos]),
                     &text.data()[textPos], copyBytes );
        }
        textPos += copyBytes;
        packPos += copyBytes;


        //
        // Check if the packdata is full
        //
        if( packPos > 11 ) {

            savePack( &pack, data, dataFill );
            ++packCount;

            //
            // reset the pack
            //
            ::memset( &pack, 0, sizeof(cdtext_pack) );
            pack.id1 = packType;
            pack.id2 = currentTrack;
            pack.id3 = packCount;
            packPos = 0;

            // update the charpos in case we continue a text in the next pack
            if( textPos <= text.length() )
                pack.charpos = ( textPos > 15 ? 15 : textPos );
        }


        //
        // Check if we have no text data left
        //
        if( textPos >= text.length() ) {

            // add one zero spacer byte
            ++packPos;

            ++currentTrack;

            // Check if all texts have been packed
            if( currentTrack > tracks.count() ) {
                savePack( &pack, data, dataFill );
                ++packCount;

                data.resize( dataFill );
                return data;
            }

            // next text block
            text = encodeCdText( textForPackType( packType, currentTrack ) );
            textPos = 0;
        }
    }
}


// track 0 means global cdtext
QString K3b::Device::CdText::Private::textForPackType( int packType, int track ) const
{
    switch( packType ) {
    default:
    case 0x80:
        if( track == 0 )
            return title;
        else if( track > 0 && track <= tracks.count() )
            return tracks[track-1].title();
        else
            return QString();

    case 0x81:
        if( track == 0 )
            return performer;
        else if( track > 0 && track <= tracks.count() )
            return tracks[track-1].performer();
        else
            return QString();

    case 0x82:
        if( track == 0 )
            return songwriter;
        else if( track > 0 && track <= tracks.count() )
            return tracks[track-1].songwriter();
        else
            return QString();

    case 0x83:
        if( track == 0 )
            return composer;
        else if( track > 0 && track <= tracks.count() )
            return tracks[track-1].composer();
        else
            return QString();

    case 0x84:
        if( track == 0 )
            return arranger;
        else if( track > 0 && track <= tracks.count() )
            return tracks[track-1].arranger();
        else
            return QString();

    case 0x85:
        if( track == 0 )
            return message;
        else if( track > 0 && track <= tracks.count() )
            return tracks[track-1].message();
        else
            return QString();

    case 0x86:
        if( track == 0 )
            return discId;
        else
            return QString();

//     case 0x87:
//         if( track == 0 )
//             return genre;
//         else if( track > 0 && track <= tracks.count() )
//             return tracks[track-1].title();
//         else
//             return QString();

    case 0x8E:
        if( track == 0 )
            return upcEan;
        else if( track > 0 && track <= tracks.count() )
            return tracks[track-1].isrc();
        else
            return QString();
    }
}


// count the overall length of a certain packtype texts
int K3b::Device::CdText::Private::textLengthForPackType( int packType ) const
{
    int len = 0;
    for( int i = 0; i <= tracks.count(); ++i )
        len += encodeCdText( textForPackType( packType, i ) ).length();
    return len;
}


void K3b::Device::CdText::debug() const
{
    // debug the stuff
    qDebug() << "CD-TEXT data:" << endl
             << "Global:" << endl
             << "  Title:      '" << title() << "'" << endl
             << "  Performer:  '" << performer() << "'" << endl
             << "  Songwriter: '" << songwriter() << "'" << endl
             << "  Composer:   '" << composer() << "'" << endl
             << "  Arranger:   '" << arranger() << "'" << endl
             << "  Message:    '" << message() << "'" << endl
             << "  Disc ID:    '" << discId() << "'" << endl
             << "  Upc Ean:    '" << upcEan() << "'" << endl;
    for( int i = 0; i < count(); ++i ) {
        qDebug() << "Track " << (i+1) << ":" << endl
                 << "  Title:      '" << d->tracks[i].title() << "'" << endl
                 << "  Performer:  '" << d->tracks[i].performer() << "'" << endl
                 << "  Songwriter: '" << d->tracks[i].songwriter() << "'" << endl
                 << "  Composer:   '" << d->tracks[i].composer() << "'" << endl
                 << "  Arranger:   '" << d->tracks[i].arranger() << "'" << endl
                 << "  Message:    '" << d->tracks[i].message() << "'" << endl
                 << "  Isrc:       '" << d->tracks[i].isrc() << "'" << endl;
    }
}


bool K3b::Device::CdText::operator==( const K3b::Device::CdText& other ) const
{
    return( d->title == other.d->title &&
            d->performer == other.d->performer &&
            d->songwriter == other.d->songwriter &&
            d->composer == other.d->composer &&
            d->arranger == other.d->arranger &&
            d->message == other.d->message &&
            d->discId == other.d->discId &&
            d->upcEan == other.d->upcEan &&
            d->tracks == other.d->tracks );
}


bool K3b::Device::CdText::operator!=( const K3b::Device::CdText& other ) const
{
    return !operator==( other );
}


bool K3b::Device::CdText::checkCrc( const unsigned char* data, int len )
{
    int r = len%18;
    if( r > 0 && r != 4 ) {
        qDebug() << "(K3b::Device::CdText) invalid cdtext size: " << len;
        return false;
    }
    else {
        len -= r;

        // TODO: what if the crc field is not used? All zeros?

        for( int i = 0; i < (len-r)/18; ++i ) {
            cdtext_pack* pack = (cdtext_pack*)&data[r];

            //
            // For some reason all crc bits are inverted.
            //
            pack[i].crc[0] ^= 0xff;
            pack[i].crc[1] ^= 0xff;

            int crc = calcX25( reinterpret_cast<unsigned char*>(&pack[i]), 18 );

            pack[i].crc[0] ^= 0xff;
            pack[i].crc[1] ^= 0xff;

            if( crc != 0x0000 )
                return false;
        }

        return true;
    }
}


bool K3b::Device::CdText::checkCrc( const QByteArray& rawData )
{
    return checkCrc( reinterpret_cast<const unsigned char*>(rawData.data()), rawData.size() );
}
