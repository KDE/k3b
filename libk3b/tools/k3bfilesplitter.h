/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_FILE_SPLITTER_H_
#define _K3B_FILE_SPLITTER_H_

#include <qiodevice.h>
#include <qstring.h>

#include <kio/global.h>

#include <k3b_export.h>


/**
 * QFile replacement which splits
 * big files according to the underlying file system's
 * maximum file size.
 *
 * The filename will be changed to include a counter
 * if the file has to be splitted like so:
 *
 * <pre>
 * filename.iso
 * filename.iso.001
 * filename.iso.002
 * ...
 * </pre>
 */
class LIBK3B_EXPORT K3bFileSplitter : public QIODevice
{
 public:
  K3bFileSplitter();
  K3bFileSplitter( const QString& filename );
  ~K3bFileSplitter();

  /**
   * Set the maximum file size. If this is set to 0
   * (the default) the max filesize is determined based on 
   * the filesystem type.
   *
   * Be aware that setName will reset the max file size.
   */
  void setMaxFileSize( KIO::filesize_t size );

  const QString& name() const;

  void setName( const QString& filename );

  virtual bool open( int mode );

  virtual void close();

  /**
   * File descriptor to read from and write to.
   * Not implemented yet!
   */
  int handle() const;

  virtual void flush();

  /**
   * Not implemented
   */
  virtual Offset size() const;

  /**
   * Not implemented
   */
  virtual Offset at() const;

  /**
   * Not implemented
   */
  virtual bool at( Offset );

  virtual bool atEnd() const;
  virtual Q_LONG readBlock( char *data, Q_ULONG maxlen );
  virtual Q_LONG writeBlock( const char *data, Q_ULONG len );
  virtual int getch();
  virtual int putch( int );
  virtual int ungetch( int );

  /**
   * Deletes all the splitted files.
   * Caution: Does remove all files that fit the naming scheme without any 
   * additional checks.
   */
  void remove();

 private:
  class Private;
  Private* d;
};

#endif
