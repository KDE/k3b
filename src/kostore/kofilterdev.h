// GENERATED FILE. Do not edit! Generated from kzip.cpp by ./update_kzip.sh
/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

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
#ifndef __kfilterdev_h
#define __kfilterdev_h

#include <qiodevice.h>
#include <qstring.h>

class QFile;
class KFilterBase;

/**
 * A class for reading and writing compressed data onto a device
 * (e.g. file, but other usages are possible, like a buffer or a socket)
 *
 * To simply read/write compressed files, see @ref deviceForFile.
 *
 * @author David Faure <faure@kde.org>
 */
class KoFilterDev : public QIODevice
{
public:
    /**
     * Constructs a KoFilterDev for a given filter (e.g. gzip, bzip2 etc.)
     * @param autoDeleteFilterbase when true this object will become the
     * owner of @p filter.
     */
    KoFilterDev( KFilterBase * filter, bool autodeleteFilterBase = false );
    /**
     * Destructs the KoFilterDev.
     */
    virtual ~KoFilterDev();

    virtual bool open( int mode );
    virtual void close();
    virtual void flush();

    /**
     * For writing gzip compressed files only:
     * set the name of the original file, to be used in the gzip header.
     */
    void setOrigFileName( const QCString & fileName );

    /**
     * Call this let this device skip the gzip headers when reading/writing.
     * This way KoFilterDev (with gzip filter) can be used as a direct wrapper
     * around zlib - this is used by KZip.
     */
    void setSkipHeaders();

    // Not implemented
    virtual QIODevice::Offset size() const;

    virtual QIODevice::Offset at() const;
    /**
     * That one can be quite slow, when going back. Use with care.
     */
    virtual bool at( QIODevice::Offset );

    virtual bool atEnd() const;

    virtual Q_LONG readBlock( char *data, Q_ULONG maxlen );
    virtual Q_LONG writeBlock( const char *data, Q_ULONG len );
    //int readLine( char *data, uint maxlen );

    virtual int getch();
    virtual int putch( int );
    virtual int ungetch( int );

#ifndef KDE_NO_COMPAT
    /**
     * Call this to create the appropriate filter device for @p base
     * working on @p file . The returned QIODevice has to be deleted
     * after using.
     * @deprecated. Use @ref deviceForFile instead.
     * To be removed in KDE 3.0
     */
    static QIODevice* createFilterDevice(KFilterBase* base, QFile* file);
#endif

    /**
     * Creates an i/o device that is able to read from @p fileName,
     * whether it's compressed or not. Available compression filters
     * (gzip/bzip2 etc.) will automatically be used.
     *
     * The compression filter to be used is determined from the @p fileName
     * if @p mimetype is empty. Pass "application/x-gzip" or "application/x-bzip2"
     * to force the corresponding decompression filter, if available.
     *
     * Warning: application/x-bzip2 may not be available.
     * In that case a QFile opened on the compressed data will be returned !
     * Use KFilterBase::findFilterByMimeType and code similar to what
     * deviceForFile is doing, to better control what's happening.
     *
     * The returned QIODevice has to be deleted after using.
     */
    static QIODevice * deviceForFile( const QString & fileName, const QString & mimetype = QString::null,
                                      bool forceFilter = false );

    /**
     * Creates an i/o device that is able to read from the QIODevice @p inDevice,
     * whether the data is compressed or not. Available compression filters
     * (gzip/bzip2 etc.) will automatically be used.
     *
     * The compression filter to be used is determined @p mimetype .
     * Pass "application/x-gzip" or "application/x-bzip2"
     * to use the corresponding decompression filter.
     *
     * Warning: application/x-bzip2 may not be available.
     * In that case 0 will be returned !
     *
     * The returned QIODevice has to be deleted after using.
     * @param inDevice input device, becomes owned by this device! Automatically deleted!
     */
    static QIODevice * device( QIODevice* inDevice, const QString & mimetype);
    // BIC: merge with device() method below, using default value for autoDeleteInDevice

    /**
     * Creates an i/o device that is able to read from the QIODevice @p inDevice,
     * whether the data is compressed or not. Available compression filters
     * (gzip/bzip2 etc.) will automatically be used.
     *
     * The compression filter to be used is determined @p mimetype .
     * Pass "application/x-gzip" or "application/x-bzip2"
     * to use the corresponding decompression filter.
     *
     * Warning: application/x-bzip2 may not be available.
     * In that case 0 will be returned !
     *
     * The returned QIODevice has to be deleted after using.
     * @param inDevice input device. Won't be deleted if @p autoDeleteInDevice = false
     */
    static QIODevice * device( QIODevice* inDevice, const QString & mimetype, bool autoDeleteInDevice );

private:
    KFilterBase *filter;
    class KoFilterDevPrivate;
    KoFilterDevPrivate * d;
};


#endif
