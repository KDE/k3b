// GENERATED FILE. Do not edit! Generated from kzip.cpp by ./update_kzip.sh

/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002 Holger Schroeder <holger-kde@holgis.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <sys/time.h>

#include <qfile.h>
#include <qdir.h>
#include <time.h>
#include <qdatetime.h>
#include <kdebug.h>
#include <qptrlist.h>
#include <kmimetype.h>
#include <zlib.h>

#include "kofilterdev.h"
#include "kozip.h"
#include "kolimitediodevice.h"

static void transformToMsDos(const QDateTime& dt, char* buffer)
{
    if ( dt.isValid() )
    {
        Q_UINT16 time =
             ( ( ( ( dt.time().hour() ) ) & 0x1f ) << 11 )      // 5 bit hour
             + ( ( ( ( dt.time().minute() ) ) & 0x3f ) << 5 )   // 6 bit minute
             + ( ( ( ( dt.time().second() >> 1 ) ) & 0x1f ) );  // 5 bit double seconds

        buffer[0]=char( time & 0xff );          // Least significant byte of time
        buffer[1]=char( ( time >> 8 ) & 0xff ); // Most significant byte of time

        Q_UINT16 date =
             ( ( ( ( dt.date().year() - 1980 ) ) & 0x7f ) << 9 )    // 7 bit year 1980-based
             + ( ( ( ( dt.date().month() ) ) & 0xf ) << 5 )         // 4 bit month
             + ( ( ( ( dt.date().day() ) ) & 0x1f ) );              // 5 bit day

        buffer[2]=char( date & 0xff );          // Least significant byte of date
        buffer[3]=char( ( date >> 8 ) & 0xff ); // Most significant byte of date

    }
    else
    {
        // We have no valid date, so assume 1980-01-01 midnight
        buffer[0]=0;
        buffer[1]=0;
        buffer[2]=33;
        buffer[3]=0;
    }
}

static int getActualTime( void )
{
    timeval value;
    gettimeofday( &value, NULL );
    return value.tv_sec;
}

////////////////////////////////////////////////////////////////////////
/////////////////////////// KoZip ///////////////////////////////////////
////////////////////////////////////////////////////////////////////////

class KoZip::KoZipPrivate
{
public:
    KoZipPrivate()
        : m_crc( 0 ), m_currentFile( 0L ), m_currentDev( 0L ), m_compression( 8 ) {}
    unsigned long m_crc;
    KoZipFileEntry* m_currentFile; // file currently being written
    QIODevice* m_currentDev; // filterdev used to write to the above file
    QPtrList<KoZipFileEntry> m_fileList; // flat list of all files, for the index (saves a recursive method ;)
    int m_compression;
};

KoZip::KoZip( const QString& filename )
    : KArchive( 0L )
{
    //kdDebug(7040) << "KoZip(filename) reached." << endl;
    m_filename = filename;
    d = new KoZipPrivate;
    setDevice( new QFile( filename ) );
}

KoZip::KoZip( QIODevice * dev )
    : KArchive( dev )
{
    //kdDebug(7040) << "KoZip::KoZip( QIODevice * dev) reached." << endl;
    d = new KoZipPrivate;
}

KoZip::~KoZip()
{
    // mjarrett: Closes to prevent ~KArchive from aborting w/o device
    //kdDebug(7040) << "~KoZip reached." << endl;
    if( isOpened() )
        close();
    if ( !m_filename.isEmpty() )
        delete device(); // we created it ourselves
    delete d;
}

bool KoZip::openArchive( int mode )
{
    //kdDebug(7040) << "openarchive reached." << endl;
    d->m_fileList.clear();

    if ( mode == IO_WriteOnly )
        return true;
    if ( mode != IO_ReadOnly && mode != IO_ReadWrite )
    {
        kdWarning(7040) << "Unsupported mode " << mode << endl;
        return false;
    }

    char buffer[47];

    // Check that it's a valid ZIP file
    // KArchive::open() opened the underlying device already.
    QIODevice* dev = device();
    int n = dev->readBlock( buffer, 4 );
    if ( n < 4 )
    {
        kdWarning(7040) << "Zip file too small " << m_filename << endl;
        return false;
    }
    if ( buffer[0] != 'P' || buffer[1] != 'K' || buffer[2] != 3 || buffer[3] != 4 )
    {
        kdWarning(7040) << "Not a Zip file " << m_filename << endl;
        return false;
    }

    uint size = dev->size(); // size of archive

    bool b = (dev->at( size - 6 )); //location of offset of start of central directry
			    // FIXME works only if archive contains no comment
    //kdDebug(7040) << "dev->at() " << dev->at() << endl;
    Q_ASSERT( b );
    if ( !b ) return false;
    n = dev->readBlock( buffer, 4 );
//    kdDebug(7040) << "buf1: " << buffer << endl;
//    kdDebug(7040) << "buf1[0]: " << (uchar)buffer[0] << endl;
//    kdDebug(7040) << "buf1[1]: " << (uchar)buffer[1] << endl;
//    kdDebug(7040) << "buf1[2]: " << (uchar)buffer[2] << endl;
//    kdDebug(7040) << "buf1[3]: " << (uchar)buffer[3] << endl;

    // begin of central header
    uint offset = (uchar)buffer[3]*256*256*256 +(uchar)buffer[2]*256*256
	    +(uchar)buffer[1]*256 + (uchar)buffer[0];
    //kdDebug(7040) << "central header starts at offset=" << offset << endl;
    if (offset >= size) kdWarning(7040) << "offset >= size" << endl;

    b = dev->at(offset);
    Q_ASSERT( b );
    if ( !b ) return false;

    bool end = false;
    do {
        n = dev->readBlock( buffer, 46 );
	if (n < 4)
	{
	    kdWarning(7040) << "Invalid ZIP file." << endl;
	    return false;
	}
	if (buffer[0] == 0x50 && buffer[1] == 0x4b
	    && buffer[2] == 0x01 && buffer[3] == 0x02 )
	{  // valid central entry signature

	    if (n < 46) {
                kdWarning(7040) << "Invalid ZIP file, central entry too short" << endl; // not long enough for valid entry
                return false;
            }

	    int namelen = (uchar)buffer[29] * 256 + (uchar)buffer[28];
            char* bufferName = new char[ namelen + 1 ];
            n = dev->readBlock( bufferName, namelen );
            if ( n < namelen )
                kdWarning(7040) << "Invalid ZIP file. Name not completely read" << endl;
	    QString name( QString::fromLocal8Bit(bufferName, namelen) );
            delete[] bufferName;

	    //kdDebug(7040) << "name: " << name << endl;
	    // only in central header ! see below.
    	    int extralen = (uchar)buffer[31] * 256 + (uchar)buffer[30];
	    int commlen = (uchar)buffer[33] * 256 + (uchar)buffer[32];
	    int cmethod = (uchar)buffer[11] * 256 + (uchar)buffer[10];

	    //kdDebug(7040) << "cmethod: " << cmethod << endl;
	    //kdDebug(7040) << "extralen: " << extralen << endl;
//	    kdDebug(7040) << "buf1[2]: " << (uchar)buffer[26] << endl;
//	    kdDebug(7040) << "buf1[3]: " << (uchar)buffer[27] << endl;

	    // uncompressed file size
	    uint esize = (uchar)buffer[27]*256*256*256 +(uchar)buffer[26]*256*256
		    +(uchar)buffer[25]*256 + (uchar)buffer[24];
	    // compressed file size
	    uint csize = (uchar)buffer[23]*256*256*256 +(uchar)buffer[22]*256*256
		    +(uchar)buffer[21]*256 + (uchar)buffer[20];
	    // offset of local header
	    uint eoffset = (uchar)buffer[45]*256*256*256 +(uchar)buffer[44]*256*256
		    +(uchar)buffer[43]*256 + (uchar)buffer[42];

	    // some clever people use different extra field lengths
	    // in the central header and in the local header... funny.
	    char localbuf[5];
	    int save_at = dev->at();
	    dev->at( eoffset + 28 );
	    dev->readBlock( localbuf, 4);
	    int localextralen = (uchar)localbuf[1] * 256 + (uchar)localbuf[0];
	    dev->at(save_at);

	    //kdDebug(7040) << "localextralen: " << localextralen << endl;

	    eoffset = eoffset + 30 + localextralen + namelen; //comment only in central header

	    //kdDebug(7040) << "esize: " << esize << endl;
	    //kdDebug(7040) << "eoffset: " << eoffset << endl;
	    //kdDebug(7040) << "csize: " << csize << endl;
	    //kdDebug(7040) << "buffer[29]: " << buffer[29] << endl;

            bool isdir = false;
            int access = 0777; // TODO available in zip file?
            int time = getActualTime();

	    QString entryName;

            if ( name.right(1) == "/" ) // Entries with a trailing slash are directories
            {
                isdir = true;
                name = name.left( name.length() - 1 );
                access |= S_IFDIR;
            }

            int pos = name.findRev( '/' );
            if ( pos == -1 )
                entryName = name;
            else
                entryName = name.mid( pos + 1 );
            Q_ASSERT( !entryName.isEmpty() );

            KArchiveEntry* entry;
            if ( isdir )
                entry = new KArchiveDirectory( this, entryName, access, time, rootDir()->user(), rootDir()->group(), QString::null );
            else
            {
	        entry = new KoZipFileEntry( this, entryName, access, time, rootDir()->user(), rootDir()->group(), QString::null,
                                          name, eoffset, esize, cmethod, csize );
                static_cast<KoZipFileEntry *>(entry)->setHeaderStart( offset );
	        //kdDebug(7040) << "KoZipFileEntry created" << endl;
                d->m_fileList.append( static_cast<KoZipFileEntry *>( entry ) );
            }

            if ( pos == -1 )
            {
	        rootDir()->addEntry(entry);
	    }
            else
	    {
                // In some tar files we can find dir/./file => call cleanDirPath
                QString path = QDir::cleanDirPath( name.left( pos ) );
                // Ensure container directory exists, create otherwise
                KArchiveDirectory * tdir = findOrCreate( path );
	        tdir->addEntry(entry);
	    }

	    //calculate offset to next entry
	    //kdDebug(7040) << "offset before: " << offset << endl;
	    offset = offset + 46 + commlen + extralen + namelen;
	    //kdDebug(7040) << "offset after: " << offset << endl;
	    b = dev->at(offset);
            Q_ASSERT( b );
            if ( !b ) return false;
	}
	else
	{
    	    if (buffer[0] == 0x50 && buffer[1] == 0x4b
		&& buffer[2] == 0x05 && buffer[3] == 0x06 )
		end = true;	//start of end of central dir reached.
            else {
                // Hmm, we arrived onto something not valid
                kdWarning(7040) << "Invalid ZIP file. Offset " << offset << " has neither 'central entry' nor 'end of entries' signature." << endl;
                return false;
            }
	} // do exit
    } while ( !end);
    //kdDebug(7040) << "*** done *** " << endl;
    return true;
}

bool KoZip::closeArchiveHack()
{
    if ( ! ( mode() & IO_WriteOnly ) )
    {
        //kdDebug(7040) << "closearchive readonly reached." << endl;
        return true;
    }
    //ReadWrite or WriteOnly
    //write all central dir file entries

    // to be written at the end of the file...
    char buffer[ 22 ]; // first used for 12, then for 22 at the end
    uLong crc = crc32(0L, Z_NULL, 0);

    Q_LONG centraldiroffset = device()->at();
    //kdDebug(7040) << "closearchive: centraldiroffset: " << centraldiroffset << endl;
    Q_LONG atbackup = device()->at();
    QPtrListIterator<KoZipFileEntry> it( d->m_fileList );

    for ( ; it.current() ; ++it )
    {	//set crc and compressed size in each local file header
        device()->at( it.current()->headerStart() + 14);
//	kdDebug(7040) << "closearchive setcrcandcsize: filename: "
//	    << (*it).filename()
//	    << " encoding: "<< (*it).encoding() << endl;

        uLong mycrc = it.current()->crc32();
	buffer[ 0 ] = (uchar)(mycrc % 256); //crc checksum, at headerStart+14
        buffer[ 1 ] = (uchar)((mycrc / 256) % 256);
	buffer[ 2 ] = (uchar)((mycrc / (256*256)) % 256);
        buffer[ 3 ] = (uchar)((mycrc / (256*256*256))% 256);

        int mysize1 = it.current()->compressedSize();
	buffer[ 4 ] = (uchar)(mysize1 % 256); //compressed file size, at headerStart+18
        buffer[ 5 ] = (uchar)((mysize1 / 256) % 256);
	buffer[ 6 ] = (uchar)((mysize1 / (256*256)) % 256);
        buffer[ 7 ] = (uchar)((mysize1 / (256*256*256))% 256);

        int myusize = it.current()->size();
	buffer[ 8 ] = (uchar)(myusize % 256); //uncompressed file size, at headerStart+22
        buffer[ 9 ] = (uchar)((myusize / 256) % 256);
	buffer[ 10 ] = (uchar)((myusize / (256*256)) % 256);
        buffer[ 11 ] = (uchar)((myusize / (256*256*256))% 256);

	device()->writeBlock( buffer, 12 );
    }
    device()->at( atbackup);

    for ( it.toFirst(); it.current() ; ++it )
    {
	kdDebug(7040) << "closearchive: filename: " << it.current()->path()
	    << " encoding: "<< it.current()->encoding() << endl;

        QCString path = QFile::encodeName(it.current()->path());
	int bufferSize = path.length() + 46;
        char* buffer = new char[ bufferSize ];

        buffer[ 0 ] = 'P'; //central file header signature
        buffer[ 1 ] = 'K';
        buffer[ 2 ] = 1;
        buffer[ 3 ] = 2;

        buffer[ 4 ] = 0x14; // version made by
        buffer[ 5 ] = 0;

        buffer[ 6 ] = 0x14; // version needed to extract
	buffer[ 7 ] = 0;

	if ( it.current()->encoding() == 8 )
	    buffer[ 8 ] = 8; // general purpose bit flag,deflated
	else
    	    buffer[ 8 ] = 0; // general purpose bit flag,stored

	buffer[ 9 ] = 0;

	if ( it.current()->encoding() == 8 )
            buffer[ 10 ] = 8; // compression method, deflated
	else
            buffer[ 10 ] = 0; // compression method, stored
        buffer[ 11 ] = 0;

        transformToMsDos( it.current()->datetime(), &buffer[ 12 ] );

        uLong mycrc = it.current()->crc32();
	buffer[ 16 ] = (uchar)(mycrc % 256); //crc checksum
        buffer[ 17 ] = (uchar)((mycrc / 256) % 256);
	buffer[ 18 ] = (uchar)((mycrc / (256*256)) % 256);
        buffer[ 19 ] = (uchar)((mycrc / (256*256*256))% 256);

        int mysize1 = it.current()->compressedSize();
	buffer[ 20 ] = (uchar)(mysize1 % 256); //compressed file size
        buffer[ 21 ] = (uchar)((mysize1 / 256) % 256);
	buffer[ 22 ] = (uchar)((mysize1 / (256*256)) % 256);
        buffer[ 23 ] = (uchar)((mysize1 / (256*256*256))% 256);

        int mysize = it.current()->size();
	buffer[ 24 ] = (uchar)(mysize % 256); //uncompressed file size
        buffer[ 25 ] = (uchar)((mysize / 256) % 256);
	buffer[ 26 ] = (uchar)((mysize / (256*256)) % 256);
        buffer[ 27 ] = (uchar)((mysize / (256*256*256))% 256);

	buffer[ 28 ] = (uchar)it.current()->path().length(); //filename length
        buffer[ 29 ] = (uchar)((it.current()->path().length() / 256) % 256);

	buffer[ 30 ] = 0; // extra field length
        buffer[ 31 ] = 0;

	buffer[ 32 ] = 0; // file comment length
        buffer[ 33 ] = 0;

	buffer[ 34 ] = 0; // disk number start
        buffer[ 35 ] = 0;

	buffer[ 36 ] = 0; // internal file attributes
        buffer[ 37 ] = 0;

        buffer[ 38 ] = 0; // external file attributes
	buffer[ 39 ] = 0;
        buffer[ 40 ] = 0;
	buffer[ 41 ] = 0;

        int myhst = it.current()->headerStart();
	buffer[ 42 ] = (uchar)(myhst % 256); //relative offset of local header
        buffer[ 43 ] = (uchar)((myhst / 256) % 256);
	buffer[ 44 ] = (uchar)((myhst / (256*256)) % 256 );
        buffer[ 45 ] = (uchar)((myhst / (256*256*256)) % 256);

        // file name
	strncpy( buffer + 46, path, path.length() );
	//kdDebug(7040) << "closearchive length to write: " << bufferSize << endl;
	crc = crc32(crc, (Bytef *)buffer, bufferSize );
	device()->writeBlock( buffer, bufferSize );
        delete[] buffer;
    }
    Q_LONG centraldirendoffset = device()->at();
    //kdDebug(7040) << "closearchive: centraldirendoffset: " << centraldirendoffset << endl;
    //kdDebug(7040) << "closearchive: device()->at(): " << device()->at() << endl;

    //write end of central dir record.
    buffer[ 0 ] = 'P'; //end of central dir signature
    buffer[ 1 ] = 'K';
    buffer[ 2 ] = 5;
    buffer[ 3 ] = 6;

    buffer[ 4 ] = 0; // number of this disk
    buffer[ 5 ] = 0;

    buffer[ 6 ] = 0; // number of disk with start of central dir
    buffer[ 7 ] = 0;

    int count = d->m_fileList.count();
    //kdDebug(7040) << "number of files (count): " << count << endl;


    buffer[ 8 ] = (uchar)(count % 256); // total number of entries in central dir of
    buffer[ 9 ] = (uchar)((count / 256) % 256); // this disk

    buffer[ 10 ] = buffer[ 8 ]; // total number of entries in the central dir
    buffer[ 11 ] = buffer[ 9 ];

    int cdsize = centraldirendoffset - centraldiroffset;
    buffer[ 12 ] = (uchar)(cdsize % 256); //size of the central dir
    buffer[ 13 ] = (uchar)((cdsize / 256) % 256);
    buffer[ 14 ] = (uchar)((cdsize / (256*256)) % 256);
    buffer[ 15 ] = (uchar)((cdsize / (256*256*256))% 256);

    //kdDebug(7040) << "end : centraldiroffset: " << centraldiroffset << endl;
    //kdDebug(7040) << "end : centraldirsize: " << cdsize << endl;

    buffer[ 16 ] = (uchar)(centraldiroffset % 256) ; //central dir offset
    buffer[ 17 ] = (uchar)((centraldiroffset / 256) % 256);
    buffer[ 18 ] = (uchar)((centraldiroffset / (256*256)) % 256);
    buffer[ 19 ] = (uchar)((centraldiroffset / (256*256*256)) % 256);

    buffer[ 20 ] = 0; //zipfile comment length
    buffer[ 21 ] = 0;

    device()->writeBlock( buffer, 22);

    //kdDebug(7040) << "kzip.cpp reached." << endl;
    return true;
}

// Reimplemented to replace device()->writeBlock with writeData
bool KoZip::writeFile( const QString& name, const QString& user, const QString& group, uint size, const char* data )
{

    if ( !prepareWriting( name, user, group, size ) )
    {
        kdWarning() << "KoZip::writeFile prepareWriting failed" << endl;
        return false;
    }

    // Write data
    if ( data && size && !writeData( data, size ) )
    {
        kdWarning() << "KoZip::writeFile writeData failed" << endl;
        return false;
    }

    if ( ! doneWriting( size ) )
    {
        kdWarning() << "KoZip::writeFile doneWriting failed" << endl;
        return false;
    }
    return true;
}

bool KoZip::prepareWriting( const QString& name, const QString& user, const QString& group, uint /*size*/ )
{
    //kdDebug(7040) << "prepareWriting reached." << endl;
    if ( !isOpened() )
    {
        qWarning( "KoZip::writeFile: You must open the zip file before writing to it\n");
        return false;
    }

    if ( ! ( mode() & IO_WriteOnly ) ) // accept WriteOnly and ReadWrite
    {
        qWarning( "KoZip::writeFile: You must open the zip file for writing\n");
        return false;
    }

    //kdDebug(7040) << "prepareWriting: currently at: " << device()->at() << " going to end of file: " << device()->size() << endl;
    if ( device()->at() < device()->size() )
    {
        bool bEnd = device()->at( device()->size() );
        Q_ASSERT( bEnd );
    }
    // Find or create parent dir
    KArchiveDirectory* parentDir = rootDir();
    QString fileName( name );
    int i = name.findRev( '/' );
    if ( i != -1 )
    {
        QString dir = name.left( i );
        fileName = name.mid( i + 1 );
        //kdDebug(7040) << "KoZip::prepareWriting ensuring " << dir << " exists. fileName=" << fileName << endl;
        parentDir = findOrCreate( dir );
    }

    int time = getActualTime();

    // construct a KoZipFileEntry and add it to list
    KoZipFileEntry * e = new KoZipFileEntry( this, fileName, 0777, time, user, group, QString::null,
                                           name, device()->at() + 30 + name.length(), // start
                                           0 /*size unknown yet*/, d->m_compression, 0 /*csize unknown yet*/ );
    e->setHeaderStart( device()->at() );
    //kdDebug(7040) << "wrote file start: " << e->position() << " name: " << name << endl;
    parentDir->addEntry( e );

    d->m_currentFile = e;
    d->m_fileList.append( e );

    // write out zip header
    QCString encodedName = QFile::encodeName(name);
    int bufferSize = encodedName.length() + 30;
    //kdDebug(7040) << "KoZip::prepareWriting bufferSize=" << bufferSize << endl;
    char* buffer = new char[ bufferSize ];

    buffer[ 0 ] = 'P'; //local file header signature
    buffer[ 1 ] = 'K';
    buffer[ 2 ] = 3;
    buffer[ 3 ] = 4;

    buffer[ 4 ] = 0x14; // version needed to extract
    buffer[ 5 ] = 0;

    buffer[ 6 ] = 0; // general purpose bit flag
    buffer[ 7 ] = 0;

    buffer[ 8 ] = (uchar)(e->encoding() % 256); // compression method
    buffer[ 9 ] = (uchar)(e->encoding() / 256);

    transformToMsDos( e->datetime(), &buffer[ 10 ] );

    buffer[ 14 ] = 'C'; //dummy crc
    buffer[ 15 ] = 'R';
    buffer[ 16 ] = 'C';
    buffer[ 17 ] = 'q';

    buffer[ 18 ] = 'C'; //compressed file size
    buffer[ 19 ] = 'S';
    buffer[ 20 ] = 'I';
    buffer[ 21 ] = 'Z';

    buffer[ 22 ] = 'U'; //uncompressed file size
    buffer[ 23 ] = 'S';
    buffer[ 24 ] = 'I';
    buffer[ 25 ] = 'Z';

    buffer[ 26 ] = (uchar)name.length(); //filename length
    buffer[ 27 ] = (uchar)((name.length() / 256) % 256);

    buffer[ 28 ] = 0; // extra field length
    buffer[ 29 ] = 0;

    // file name
    strncpy( buffer + 30, encodedName, encodedName.length() );

    // Write header
    bool b = (device()->writeBlock( buffer, bufferSize ) == bufferSize );
    d->m_crc = 0L;
    delete[] buffer;

    Q_ASSERT( b );
    if (!b)
        return false;

    // Prepare device for writing the data
    // Either device() if no compression, or a KoFilterDev to compress
    if ( d->m_compression == 0 ) {
        d->m_currentDev = device();
        return true;
    }

    d->m_currentDev = KoFilterDev::device( device(), "application/x-gzip", false );
    Q_ASSERT( d->m_currentDev );
    if ( !d->m_currentDev )
        return false; // ouch
    static_cast<KoFilterDev *>(d->m_currentDev)->setSkipHeaders(); // Just zlib, not gzip

    b = d->m_currentDev->open( IO_WriteOnly );
    Q_ASSERT( b );
    return b;
}

void KoZip::close() { // HACK for misplaced closeArchive() call in KDE-3.0s KArchive
    if (!isOpened()) return;
    closeArchiveHack();
    device()->close();
    m_open = false;
}

bool KoZip::doneWriting( uint size )
{
    if ( d->m_currentFile->encoding() == 8 ) {
        // Finish
        (void)d->m_currentDev->writeBlock( 0, 0 );
        delete d->m_currentDev;
    }
    // If 0, d->m_currentDev was device() - don't delete ;)
    d->m_currentDev = 0L;

    Q_ASSERT( d->m_currentFile );
    //kdDebug(7040) << "donewriting reached." << endl;
    //kdDebug(7040) << "filename: " << d->m_currentFile->path() << endl;
    //kdDebug(7040) << "getpos (at): " << device()->at() << endl;
    d->m_currentFile->m_size = size;
    int csize = device()->at() -
        d->m_currentFile->headerStart() - 30 -
	d->m_currentFile->path().length();
    d->m_currentFile->setCompressedSize(csize);
    //kdDebug(7040) << "usize: " << d->m_currentFile->size() << endl;
    //kdDebug(7040) << "csize: " << d->m_currentFile->compressedSize() << endl;
    //kdDebug(7040) << "headerstart: " << d->m_currentFile->headerStart() << endl;

    //kdDebug(7040) << "crc: " << d->m_crc << endl;
    d->m_currentFile->setCRC32( d->m_crc );

    d->m_currentFile = 0L;
    return true;
}

void KoZip::virtual_hook( int id, void* data )
{ KArchive::virtual_hook( id, data ); }

bool KoZip::writeData(const char * c, uint i)
{
    Q_ASSERT( d->m_currentFile );
    Q_ASSERT( d->m_currentDev );
    if (!d->m_currentFile || !d->m_currentDev)
        return false;

    // crc to be calculated over uncompressed stuff...
    // and they didn't mention it in their docs...
    d->m_crc = crc32(d->m_crc, (const Bytef *) c , i);

    Q_LONG written = d->m_currentDev->writeBlock( c, i );
    //kdDebug(7040) << "KoZip::writeData wrote " << i << " bytes." << endl;
    Q_ASSERT( written == (Q_LONG)i );
    return written == (Q_LONG)i;
}

void KoZip::setCompression( Compression c )
{
    d->m_compression = ( c == NoCompression ) ? 0 : 8;
}

KoZip::Compression KoZip::compression() const
{
   return ( d->m_compression == 8 ) ? DeflateCompression : NoCompression;
}

///////////////

QByteArray KoZipFileEntry::data() const
{
    QIODevice* dev = device();
    QByteArray arr = dev->readAll();
    delete dev;
    return arr;
}

QIODevice* KoZipFileEntry::device() const
{
    //kdDebug(7040) << "KoZipFileEntry::device creating iodevice limited to pos=" << position() << ", csize=" << compressedSize() << endl;
    // Limit the reading to the appropriate part of the underlying device (e.g. file)
    KoLimitedIODevice* limitedDev = new KoLimitedIODevice( archive()->device(), position(), compressedSize() );
    if ( encoding() == 0 || compressedSize() == 0 ) // no compression (or even no data)
        return limitedDev;

    if ( encoding() == 8 )
    {
        // On top of that, create a device that uncompresses the zlib data
        QIODevice* filterDev = KoFilterDev::device( limitedDev, "application/x-gzip" );
        if ( !filterDev )
            return 0L; // ouch
        static_cast<KoFilterDev *>(filterDev)->setSkipHeaders(); // Just zlib, not gzip
        bool b = filterDev->open( IO_ReadOnly );
        Q_ASSERT( b );
        return filterDev;
    }

    kdError() << "This zip file contains files compressed with method "
              << encoding() <<", this method is currently not supported by KoZip,"
              <<" please use a command-line tool to handle this file." << endl;
    return 0L;
}
