// GENERATED FILE. Do not edit! Generated from kzip.cpp by ./update_kzip.sh
/* This file is part of the KDE libraries
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
#ifndef __kzip_h
#define __kzip_h

#include <sys/stat.h>
#include <sys/types.h>

#include <qdatetime.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qdict.h>
#include <qvaluelist.h>
#define private public // need protected for m_open access for the HACK, and public for setting KArchiveFile::m_size
#include <karchive.h>
#undef private
#include <karchive.h>

class KoZipFileEntry;
/**
 * @short A class for reading/writing zip archives.
 * @author Holger Schroeder <holger-kde@holgis.net>
 */
class KoZip : public KArchive
{
public:
    /**
     * Creates an instance that operates on the given filename.
     * using the compression filter associated to given mimetype.
     *
     * @param filename is a local path (e.g. "/home/holger/myfile.zip")
     */
    KoZip( const QString& filename );

    /**
     * Creates an instance that operates on the given device.
     * The device can be compressed (KFilterDev) or not (QFile, etc.).
     * WARNING: don't assume that giving a QFile here will decompress the file,
     * in case it's compressed!
     */
    KoZip( QIODevice * dev );

    /**
     * If the zip file is still opened, then it will be
     * closed automatically by the destructor.
     */
    virtual ~KoZip();

    /**
     * The name of the zip file, as passed to the constructor
     * Null if you used the QIODevice constructor.
     */
    QString fileName() { return m_filename; }

    enum Compression { NoCompression = 0, DeflateCompression = 1 };
    /**
     * Call this before writeFile or prepareWriting, to define whether the next
     * files to be written should be compressed or not.
     */
    void setCompression( Compression c );
    Compression compression() const;

    /**
     * If an archive is opened for writing then you can add a new file
     * using this function.
     * This method takes the whole data at once.
     * @param name can include subdirs e.g. path/to/the/file
     */
    virtual bool writeFile( const QString& name, const QString& user, const QString& group, uint size, const char* data );

    /**
     * Alternative method: call prepareWriting, writeData in small chunks, doneWriting
     * @param size unused
     */
    virtual bool prepareWriting( const QString& name, const QString& user, const QString& group, uint size );
    bool writeData( const char* data, uint size );
    virtual bool doneWriting( uint size );
    virtual void close(); // HACK for misplaced closeArchive() call in KDE-3.0s KArchive
    virtual bool closeArchive() { return true; } // part of the same hack

protected:
    /**
     * Opens the archive for reading.
     * Parses the directory listing of the archive
     * and creates the KArchiveDirectory/KArchiveFile entries.
     *
     */
    virtual bool openArchive( int mode );
    virtual bool closeArchiveHack();

    /**
     * @internal Not needed for zip
     */
    virtual bool writeDir( const QString&, const QString&, const QString& ) { return true; }

protected:
    virtual void virtual_hook( int id, void* data );
private:
    QString m_filename;
    class KoZipPrivate;
    KoZipPrivate * d;
};


/**
 * @internal
 */
class KoZipFileEntry : public KArchiveFile
{
public:
    /*KoZipFileEntry() : st(-1)
      {}*/
    KoZipFileEntry( KoZip* zip, const QString& name, int access, int date,
                   const QString& user, const QString& group, const QString& symlink,
                   const QString& path, Q_LONG start, Q_LONG uncompressedSize,
                   int encoding, Q_LONG compressedSize) :
        KArchiveFile( zip, name, access, date, user, group, symlink,
                      start, uncompressedSize ),
        m_crc(0),
        m_compressedSize(compressedSize),
        m_headerStart(0),
        m_encoding(encoding),
        m_path( path )
    {}
    int encoding() const { return m_encoding; }
    Q_LONG compressedSize() const { return m_compressedSize; }

    // Only used when writing
    void setCompressedSize(Q_LONG compressedSize) { m_compressedSize = compressedSize; }

    // Header start: only used when writing
    void setHeaderStart(Q_LONG headerstart) { m_headerStart = headerstart; }
    Q_LONG headerStart() const {return m_headerStart; }

    // CRC: only used when writing
    unsigned long crc32() const { return m_crc; }
    void setCRC32(unsigned long crc32) { m_crc=crc32; }

    // Name with complete path - KArchiveFile::name() is the filename only (no path)
    QString path() const { return m_path; }

    /**
     * @return the content of this file.
     * Call data() with care (only once per file), this data isn't cached.
     */
    virtual QByteArray data() const;

    /**
     * This method returns a QIODevice to read the file contents.
     * This is obviously for reading only.
     * Note that the ownership of the device is being transferred to the caller,
     * who will have to delete it.
     * The returned device auto-opens (in readonly mode), no need to open it.
     */
    QIODevice* device() const; // WARNING, not virtual!

private:
    unsigned long m_crc;
    Q_LONG m_compressedSize;
    Q_LONG m_headerStart;
    int m_encoding;
    QString m_path;
};

#endif
