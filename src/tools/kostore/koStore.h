// -*- c-basic-offset: 2 -*-
/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __koStore_h_
#define __koStore_h_

#include <qstring.h>
#include <qstringlist.h>
#include <qiodevice.h>
#include <qvaluestack.h>

/**
 * Saves and loads koffice documents using various backends. Currently supported
 * backends are ZIP, tar and directory.
 * We call a "store" the file on the hard disk (the one the users sees)
 * and call a "file" a file inside the store.
 */
class KoStore
{
public:

  enum Mode { Read, Write };
  enum Backend { Auto, Tar, Zip, Directory };

  /**
   * Open a store (i.e. the representation on disk of a KOffice document).
   *
   * @param fileName the name of the file to open
   * @param mode if KoStore::Read, open an existing store to read it.
   *             if KoStore::Write, create or replace a store.
   * @param backend the backend to use for the data storage.
   * Auto means automatically-determined for reading,
   * and the current format (now Zip) for writing.
   *
   * @param appIdentification the application's mimetype,
   * to be written in the file for "mime-magic" identification.
   * Only meaningful if mode is Write, and if backend!=Directory.
   */
  static KoStore* createStore( const QString& fileName, Mode mode, const QCString & appIdentification = "", Backend backend = Auto );

  /**
   * Create a store for any kind of iodevice: file, memory buffer...
   * This method doesn't support the Directory store!
   */
  static KoStore* createStore( QIODevice *device, Mode mode, const QCString & appIdentification = "", Backend backend = Auto );

  /**
   * Destroys the store (i.e. closes the file on the hard disk)
   */
  virtual ~KoStore();

  /**
   * Open a new file inside the store
   * @param name The filename, internal representation ("root", "tar:/0"... ).
   *        If the tar:/ prefix is missing it's assumed to be a relative URI.
   * @return true on success.
   */
  bool open( const QString & name );

  /**
   * Allows to check for an open storage.
   */
  bool isOpen() const;

  /**
   * Close the file inside the store
   * @return true on success.
   */
  bool close();

  /**
   * Get a device for reading a file from the store directly
   * (slightly faster than read() calls)
   * You need to call @ref open first, and @ref close afterwards.
   */
  QIODevice* device() const;

  /**
   * Read data from the currently opened file. You can also use the streams
   * for this.
   */
  QByteArray read( long unsigned int max );

  /**
   * Write data into the currently opened file. You can also use the streams
   * for this.
   */
  Q_LONG write( const QByteArray& _data );

  /**
   * Read data from the currently opened file. You can also use the streams
   * for this.
   * @return size of data read, -1 on error
   */
  Q_LONG read( char *_buffer, Q_ULONG _len );

  /**
   * Write data into the currently opened file. You can also use the streams
   * for this.
   */
  virtual Q_LONG write( const char* _data, Q_ULONG _len );

  /**
   * @return the size of the currently opened file, -1 on error.
   * Can be used as an argument for the read methods, for instance
   */
  QIODevice::Offset size() const;

  /**
   * @return true if an error occured
   */
  bool bad() const { return !m_bGood; } // :)

  /**
   * @return the mode used when opening, read or write
   */
  Mode mode() const { return m_mode; }

  /**
   * Enters one or multiple directories. In Read mode this actually
   * checks whether the specified directories exist and returns false
   * if they don't. In Write mode we don't create the directory, we
   * just use the "current directory" to generate the absolute path
   * if you pass a relative path (one not starting with tar:/) when
   * opening a stream.
   * Note: Operates on internal names
   */
  bool enterDirectory( const QString& directory );

  /**
   * Leaves a directory. Equivalent to "cd .."
   * @return true on success, false if we were at the root already to
   * make it possible to "loop to the root"
   */
  bool leaveDirectory();

  /**
   * Returns the current path including a trailing slash.
   * Note: Returns a path in "internal name" style
   */
  QString currentPath() const;

  /**
   * Stacks the current directory. Restore the current path using
   * @ref popDirectory .
   */
  void pushDirectory();

  /**
   * Restores the previously pushed directory. No-op if the stack is
   * empty.
   */
  void popDirectory();

  // See QIODevice
  bool at( QIODevice::Offset pos );
  QIODevice::Offset at() const;
  bool atEnd() const;


protected:

  KoStore() {}

  /**
   * Init store - called by constructor.
   * @return true on success
   */
  virtual bool init( Mode mode );
  /**
   * Open the file @p name in the store, for writing
   * On success, this method must set m_stream to a stream in which we can write.
   * @param name "absolute path" (in the archive) to the file to open
   * @return true on success
   */
  virtual bool openWrite( const QString& name ) = 0;
  /**
   * Open the file @p name in the store, for reading.
   * On success, this method must set m_stream to a stream from which we can read,
   * as well as setting m_iSize to the size of the file.
   * @param name "absolute path" (in the archive) to the file to open
   * @return true on success
   */
  virtual bool openRead( const QString& name ) = 0;

  /**
   * @return true on success
   */
  virtual bool closeRead() = 0;
  /**
   * @return true on success
   */
  virtual bool closeWrite() = 0;

  /**
   * Enter a subdirectory of the current directory.
   * The directory might not exist yet in Write mode.
   */
  virtual bool enterRelativeDirectory( const QString& dirName ) = 0;
  /**
   * Enter a directory where we've been before.
   * It is guaranteed to always exist.
   */
  virtual bool enterAbsoluteDirectory( const QString& path ) = 0;

  virtual bool fileExists( const QString& absPath ) = 0;

private:
  static Backend determineBackend( QIODevice* dev );

  /**
   * Conversion routine
   * @param _internalNaming name used internally : "root", "tar:/0", ...
   * @return the name used in the file, more user-friendly ("maindoc.xml",
   *         "part0/maindoc.xml", ...)
   * Examples:
   *
   * tar:/0 is saved as part0/maindoc.xml
   * tar:/0/1 is saved as part0/part1/maindoc.xml
   * tar:/0/1/pictures/picture0.png is saved as part0/part1/pictures/picture0.png
   *
   * see specification (koffice/lib/store/SPEC) for details.
   */
  QString toExternalNaming( const QString & _internalNaming );

  // Expands a full path name for a stream (directories+filename)
  QString expandEncodedPath( QString intern );

  // Expans only directory names(!)
  // Needed for the path handling code, as we only operate on internal names
  QString expandEncodedDirectory( QString intern );

  enum
  {
      NAMING_VERSION_2_1,
      NAMING_VERSION_2_2
  } m_namingVersion;

  // Enter *one* single directory. Nothing like foo/bar/bleh allowed.
  // Performs some checking when in Read mode
  bool enterDirectoryInternal( const QString& directory );

protected:

  Mode m_mode;

  // Store the filenames (with full path inside the archive) when writing, to avoid duplicates
  QStringList m_strFiles;

  // The "current directory" (path)
  QStringList m_currentPath;

  // Used to push/pop directories to make it easy to save/restore
  // the state
  QValueStack<QString> m_directoryStack;

  // Current filename (between an open() and a close())
  QString m_sName;
  // Current size of the file named m_sName
  QIODevice::Offset m_iSize;

  // The stream for the current read or write operation
  QIODevice * m_stream;

  bool m_bIsOpen;
  // Must be set by the constructor.
  bool m_bGood;

  static const int s_area;

private:
  KoStore( const KoStore& store );  // don't copy
  KoStore& operator=( const KoStore& store );  // don't assign

  class Private;
  Private * d;

};

#endif
