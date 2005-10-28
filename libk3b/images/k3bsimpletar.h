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



#ifndef _K3B_SIMPLE_TAR_H_
#define _K3B_SIMPLE_TAR_H_

#include <time.h> // time()

#include <qstring.h>
#include <qstringlist.h>

#include <kio/global.h>

class K3bSimpleTar;


class K3bSimpleTarEntry
{
 public:
  ~K3bSimpleTarEntry();

  const QString& filename() const { return m_filename; }
  KIO::filesize_t size() const { return m_size; }

  KIO::filesize_t at() const;
  bool at( KIO::filesize_t );

  Q_LONG read( char* data, Q_LONG maxlen );

  QByteArray readAll();

 private:
  K3bSimpleTarEntry( K3bSimpleTar*, const QString&, KIO::filesize_t, KIO::filesize_t );

  QString m_filename;
  KIO::filesize_t m_size;
  KIO::filesize_t m_startOffset;

  K3bSimpleTar* m_tar;

  friend class K3bSimpleTar;
};


/**
 * K3bSimpleTar is a replacement for KTar.
 *
 * In comparision to KTar it supports large files but no compression.
 * Due to the large file support it cannot be based on KArchive.
 *
 * This class onyl supports the very basic features K3b needs:
 * writing and reading plain files. No support for directories or links.
 *
 * Parts of this class are based on the KTar code.
 */
class K3bSimpleTar
{
 public:
  K3bSimpleTar();

  /**
   * If the tar ball is still opened, then it will be
   * closed automatically by the destructor.
   */
  virtual ~K3bSimpleTar();

  /**
   * Opens the archive for reading.
   *
   * @param mode the mode of the file (IO_ReadOnly or IO_WriteOnly)
   */
  bool open( const QString& filename, int mode );
  bool isOpen() const;
  void close();

  int mode() const;

  /**
   * The name of the tar file, as passed to the constructor
   * Null if you used the QIODevice constructor.
   * @return the name of the file, or QString::null if unknown
   */
  const QString& fileName() { return m_filename; }

  K3bSimpleTarEntry* fileEntry( const QString& name ) const;

  bool writeFile( const QString& filename, int size, const char* data );

  bool prepareWriting( const QString& name, KIO::filesize_t size );
  bool writeData( const char* data, int len );
  bool doneWriting( KIO::filesize_t size );

 private:
  /**
   * @internal
   * Fills @p buffer for writing a file as required by the tar format
   * Has to be called LAST, since it does the checksum
   * (normally, only the name has to be filled in before)
   * @param mode is expected to be 6 chars long, [uname and gname 31].
   */
  void fillBuffer( char * buffer, const char * mode, int size, time_t mtime,
		   char typeflag, const char * uname, const char * gname );

  Q_LONG readRawHeader(char *buffer);
  Q_LONG readHeader(char *buffer,QString &name,QString &symlink);

  Q_LONG writeToArchiveFile( const char* data, Q_LONG len );
  Q_LONG readFromArchiveFile( char* data, Q_LONG maxlen );

  KIO::filesize_t archiveFileAt() const;
  bool archiveFileAt( KIO::filesize_t );

  QString m_filename;

 private:
  class Private;
  Private * d;

  friend class K3bSimpleTarEntry;
};

#endif
