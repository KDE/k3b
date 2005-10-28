/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 * 
 * KTar Copyright (C) 2000 David Faure <faure@kde.org>
 *      Copyright (C) 2003 Leo Savernik <l.savernik@aon.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h> // strtol
#include <time.h> // time()
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <qcstring.h>
#include <qfile.h>
#include <qdict.h>

#include <kdebug.h>

#include "k3bsimpletar.h"



class K3bSimpleTar::Private
{
public:
    Private() 
      : mode( -1 ),
	fd( -1 ) {}

  QStringList dirList;

  int mode;

  int fd;

  QDict<K3bSimpleTarEntry> fileEntries;
};

K3bSimpleTar::K3bSimpleTar()
{
  d = new Private;
  d->fileEntries.setAutoDelete(true);
}


K3bSimpleTar::~K3bSimpleTar()
{
  close();

  delete d;
}


int K3bSimpleTar::mode() const
{
  return d->mode;
}


bool K3bSimpleTar::isOpen() const
{
  return ( d->mode != -1 );
}


K3bSimpleTarEntry* K3bSimpleTar::fileEntry( const QString& name ) const
{
  return d->fileEntries[name];
}


Q_LONG K3bSimpleTar::readRawHeader(char *buffer) {
  // Read header
  Q_LONG n = readFromArchiveFile( buffer, 0x200 );
  if ( n == 0x200 && buffer[0] != 0 ) {
    // Make sure this is actually a tar header
    if (strncmp(buffer + 257, "ustar", 5)) {
      // The magic isn't there (broken/old tars), but maybe a correct checksum?
      QCString s;

      int check = 0;
      for( uint j = 0; j < 0x200; ++j )
        check += buffer[j];

      // adjust checksum to count the checksum fields as blanks
      for( uint j = 0; j < 8 /*size of the checksum field including the \0 and the space*/; j++ )
        check -= buffer[148 + j];
      check += 8 * ' ';

      s.sprintf("%o", check );

      // only compare those of the 6 checksum digits that mean something,
      // because the other digits are filled with all sorts of different chars by different tars ...
      if( strncmp( buffer + 148 + 6 - s.length(), s.data(), s.length() ) ) {
        kdWarning(7041) << "K3bSimpleTar: invalid TAR file. Header is: " << QCString( buffer+257, 5 ) << endl;
        return -1;
      }
    }/*end if*/
  } else {
    // reset to 0 if 0x200 because logical end of archive has been reached
    if (n == 0x200) n = 0;
  }/*end if*/
  return n;
}


Q_LONG K3bSimpleTar::readHeader( char *buffer, QString &name, QString &symlink )
{
  name.truncate(0);
  symlink.truncate(0);
  while (true) {
    Q_LONG n = readRawHeader(buffer);
    if(n != 0x200) 
      return n;

    // is it a longlink?
    if(strcmp(buffer,"././@LongLink") == 0) {
      kdDebug() << "(K3bSimpleTar) no long link support." << endl;
      return -1;
    } 
    else {
      break;
    }
  }

  // if not result of longlink, read names directly from the header
  if (name.isEmpty())
    // there are names that are exactly 100 bytes long
    // and neither longlink nor \0 terminated (bug:101472)
    name = QFile::decodeName(QCString(buffer, 101));
  if (symlink.isEmpty())
    symlink = QFile::decodeName(QCString(buffer + 0x9d, 101));

  return 0x200;
}


bool K3bSimpleTar::open( const QString& filename, int mode )
{
  close();

  // open the file
  int flags = O_LARGEFILE;
  if( mode == IO_WriteOnly )
    flags |= O_WRONLY|O_CREAT;
  else
    flags |= O_RDONLY;
  d->fd = ::open( QFile::encodeName(filename), flags, 644 );

  if( d->fd < 0 ) {
    kdDebug() << "(K3bSimpleTar) could not open " << filename << ": " << strerror(errno) << endl;
    return false;
  }

  d->mode = mode;
  m_filename = filename;

  if ( !(mode & IO_ReadOnly) )
    return true;

  // read dir infos
  char buffer[ 0x200 ];

  while( 1 ) {
    QString name;
    QString symlink;

    // Read header
    Q_LONG n = readHeader(buffer,name,symlink);
    if (n < 0) return false;
    if (n == 0x200) {

      if ( name.contains( "/" ) ) {
	kdDebug() << "(K3bSimpleTar) no directory support!" << endl;
	close();
	return false;
      }

      // read access
//       buffer[ 0x6b ] = 0;
//       char *dummy;
//       const char* p = buffer + 0x64;
//       while( *p == ' ' ) ++p;
//       int access = (int)strtol( p, &dummy, 8 );

//       // read time
//       buffer[ 0x93 ] = 0;
//       p = buffer + 0x88;
//       while( *p == ' ' ) ++p;
//       int time = (int)strtol( p, &dummy, 8 );

      // read type flag
      char typeflag = buffer[ 0x9c ];
      // '0' for files, '1' hard link, '2' symlink, '5' for directory
      // (and 'L' for longlink filenames, 'K' for longlink symlink targets)
      // and 'D' for GNU tar extension DUMPDIR
      bool isdir = false;
      if ( typeflag == '1' )
	isdir = true;

      bool isDumpDir = false;
      if ( typeflag == 'D' ) {
	isdir = false;
	isDumpDir = true;
      }

      if ( isdir ) {
	kdDebug() << "(K3bSimpleTar) no directory support!" << endl;
	close();
	return false;
      }

      // read size
      buffer[ 0x88 ] = 0;
      const char* p = buffer + 0x7c;
      while( *p == ' ' ) ++p;
      int size = (int)strtol( p, 0, 8 );

      // for isDumpDir we will skip the additional info about that dirs contents
      if ( !isDumpDir ) {

	if ( typeflag == '1' ) {
	  kdDebug() << "(K3bSimpleTar) no hard link support!" << endl;
	  close();
	  return false;
	}

	kdDebug() << "(K3bSimpleTar) found file entry: " << name << " (" << archiveFileAt() << " + " << size << ")" << endl;

	d->fileEntries.insert( name, new K3bSimpleTarEntry( this, name, archiveFileAt(), (KIO::filesize_t)size ) );
      }

      // Skip contents + align bytes
      int rest = size % 0x200;
      KIO::filesize_t skip = (KIO::filesize_t)size + (rest ? (KIO::filesize_t)(0x200 - rest) : (KIO::filesize_t)0);

      if( !archiveFileAt( archiveFileAt() + skip ) ) {
	kdWarning() << "(K3bSimpleTar) skipping " << skip << " failed" << endl;
	close();
	return false;
      }
    }
    else {
      break;
    }
  }

  return true;
}


void K3bSimpleTar::close()
{
  if( isOpen() ) {
    ::close( d->fd ); 
    d->fd = -1;
    d->mode = -1;
    m_filename.truncate( 0 );
    d->fileEntries.clear();
  }
}


bool K3bSimpleTar::prepareWriting( const QString& name, KIO::filesize_t size )
{
  if ( !isOpen() ) {
    kdWarning() << "K3bSimpleTar::prepareWriting: You must open the tar file before writing to it\n";
    return false;
  }

  if ( !(mode() & IO_WriteOnly) ) {
    kdWarning(7041) << "K3bSimpleTar::prepareWriting: You must open the tar file for writing\n";
    return false;
  }

  if( name.contains( "/" ) ) {
    kdDebug() << "(K3bSimpleTar) no dir support!" << endl;
    return false;
  }

  if( name.length() > 99 ) {
    kdDebug() << "(K3bSimpleTar) no long link support!" << endl;
    return false;
  }

  if( size >= 2ULL*1024ULL*1024ULL*1024ULL ) {
    kdDebug() << "(K3bSimpleTar) file size too big!" << endl;
    return false;
  }

  char buffer[ 0x201 ];
  memset( buffer, 0, 0x200 );

  // provide converted stuff we need lateron
  QCString encodedFilename = QFile::encodeName(name);
  mode_t perm = 0100644;
  time_t mtime = time(0);

  // Write (potentially truncated) name
  strncpy( buffer, encodedFilename, 99 );
  buffer[99] = 0;
  // zero out the rest (except for what gets filled anyways)
  memset(buffer+0x9d, 0, 0x200 - 0x9d);

  QCString permstr;
  permstr.sprintf("%o",perm);
  permstr.rightJustify(6, ' ');
  fillBuffer(buffer, permstr, (int)size, mtime, 0x30, "nobody", "nobody");

  // Write header
  return ( writeToArchiveFile( buffer, 0x200 ) == 0x200 );
}


bool K3bSimpleTar::writeData( const char* data, int len )
{
  return ( writeToArchiveFile( data, len ) == len );
}


bool K3bSimpleTar::doneWriting( KIO::filesize_t size )
{
  // Write alignment
  int rest = size % 0x200;

  if( rest ) {
    char buffer[ 0x201 ];
    for( uint i = 0; i < 0x200; ++i )
      buffer[i] = 0;
    Q_LONG nwritten = writeToArchiveFile( buffer, 0x200 - rest );
    return nwritten == 0x200 - rest;
  }

  return true;
}


bool K3bSimpleTar::writeFile( const QString& filename, int size, const char* data )
{
  return( prepareWriting( filename, (KIO::filesize_t)size ) && writeData( data, size ) && doneWriting( (KIO::filesize_t)size ) );
}


/*** Some help from the tar sources
     struct posix_header
     {                               byte offset
     char name[100];               *   0 *     0x0
     char mode[8];                 * 100 *     0x64
     char uid[8];                  * 108 *     0x6c
     char gid[8];                  * 116 *     0x74
     char size[12];                * 124 *     0x7c
     char mtime[12];               * 136 *     0x88
     char chksum[8];               * 148 *     0x94
     char typeflag;                * 156 *     0x9c
     char linkname[100];           * 157 *     0x9d
     char magic[6];                * 257 *     0x101
     char version[2];              * 263 *     0x107
     char uname[32];               * 265 *     0x109
     char gname[32];               * 297 *     0x129
     char devmajor[8];             * 329 *     0x149
     char devminor[8];             * 337 *     ...
     char prefix[155];             * 345 *
     * 500 *
     };
*/

void K3bSimpleTar::fillBuffer( char * buffer,
			       const char * mode, int size, time_t mtime, char typeflag,
			       const char * uname, const char * gname )
{
  // mode (as in stat())
  //  assert( strlen(mode) == 6 );
  strcpy( buffer+0x64, mode );
  buffer[ 0x6a ] = ' ';
  buffer[ 0x6b ] = '\0';

  // dummy uid
  strcpy( buffer + 0x6c, "   765 ");
  // dummy gid
  strcpy( buffer + 0x74, "   144 ");

  // size
  QCString s;
  s.sprintf("%o", size); // OCT
  s = s.rightJustify( 11, ' ' );
  strcpy( buffer + 0x7c, s.data() );
  buffer[ 0x87 ] = ' '; // space-terminate (no null after)

  // modification time
  s.sprintf("%lo", static_cast<unsigned long>(mtime) ); // OCT
  s = s.rightJustify( 11, ' ' );
  strcpy( buffer + 0x88, s.data() );
  buffer[ 0x93 ] = ' '; // space-terminate (no null after)

  // spaces, replaced by the check sum later
  buffer[ 0x94 ] = 0x20;
  buffer[ 0x95 ] = 0x20;
  buffer[ 0x96 ] = 0x20;
  buffer[ 0x97 ] = 0x20;
  buffer[ 0x98 ] = 0x20;
  buffer[ 0x99 ] = 0x20;

  /* From the tar sources :
     Fill in the checksum field.  It's formatted differently from the
     other fields: it has [6] digits, a null, then a space -- rather than
     digits, a space, then a null. */

  buffer[ 0x9a ] = '\0';
  buffer[ 0x9b ] = ' ';

  // type flag (dir, file, link)
  buffer[ 0x9c ] = typeflag;

  // magic + version
  strcpy( buffer + 0x101, "ustar");
  strcpy( buffer + 0x107, "00" );

  // user
  strcpy( buffer + 0x109, uname );
  // group
  strcpy( buffer + 0x129, gname );

  // Header check sum
  int check = 32;
  for( uint j = 0; j < 0x200; ++j )
    check += buffer[j];
  s.sprintf("%o", check ); // OCT
  s = s.rightJustify( 7, ' ' );
  strcpy( buffer + 0x94, s.data() );
}


Q_LONG K3bSimpleTar::writeToArchiveFile( const char* data, Q_LONG len )
{
  return ::write( d->fd, data, (size_t)len );
}


Q_LONG K3bSimpleTar::readFromArchiveFile( char* data, Q_LONG maxlen )
{
  return ::read( d->fd, data, (size_t)maxlen );
}


KIO::filesize_t K3bSimpleTar::archiveFileAt() const
{
  return (KIO::filesize_t)::lseek64( d->fd, (off64_t)0, SEEK_CUR );
}


bool K3bSimpleTar::archiveFileAt( KIO::filesize_t pos )
{
  return ( (KIO::filesize_t)::lseek64( d->fd, (off64_t)pos, SEEK_SET ) == pos );
}




K3bSimpleTarEntry::K3bSimpleTarEntry( K3bSimpleTar* tar, const QString& filename,
				      KIO::filesize_t start, KIO::filesize_t size )
  : m_filename( filename ),
    m_size( size ),
    m_startOffset( start ),
    m_tar( tar )
{
}


K3bSimpleTarEntry::~K3bSimpleTarEntry()
{
}


KIO::filesize_t K3bSimpleTarEntry::at() const
{
  return m_tar->archiveFileAt() - m_startOffset;
}


bool K3bSimpleTarEntry::at( KIO::filesize_t pos )
{
  return m_tar->archiveFileAt( m_startOffset + pos );
}


Q_LONG K3bSimpleTarEntry::read( char* data, Q_LONG maxlen )
{
  maxlen = QMIN( maxlen, (Q_LONG)(m_size - m_tar->archiveFileAt() + m_startOffset) );
  return m_tar->readFromArchiveFile( data, maxlen );
}


QByteArray K3bSimpleTarEntry::readAll()
{
  QByteArray buf( (int)size() );
  at( 0 );
  read( buf.data(), (Q_LONG)size() );
  return buf;
}
