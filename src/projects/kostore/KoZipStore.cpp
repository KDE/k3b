/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000-2002 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KoZipStore.h"

#include <KZip>
#include <KIO/StoredTransferJob>
#include <KJobWidgets>
#include <QBuffer>
#include <QByteArray>
#include <QDebug>
#include <QUrl>

KoZipStore::KoZipStore( const QString & _filename, Mode _mode, const QByteArray & appIdentification )
{
    qCDebug(KOSTORE) <<"KoZipStore Constructor filename =" << _filename
                    << " mode = " << int(_mode)
                    << " mimetype = " << appIdentification << Qt::endl;

    m_pZip = new KZip( _filename );

        m_bGood = initZipStore( _mode, appIdentification ); // open the zip file and init some vars
}

KoZipStore::KoZipStore( QIODevice *dev, Mode mode, const QByteArray & appIdentification )
{
    m_pZip = new KZip( dev );
    m_bGood = initZipStore( mode, appIdentification );
}

KoZipStore::KoZipStore( QWidget* window, const QUrl & _url, const QString & _filename, Mode _mode, const QByteArray & appIdentification )
{
    qCDebug(KOSTORE) <<"KoZipStore Constructor url" << _url.toDisplayString( QUrl::PreferLocalFile )
                    << " filename = " << _filename
                    << " mode = " << int(_mode)
                    << " mimetype = " << appIdentification << Qt::endl;

    m_url = _url;
    m_window = window;

    if ( _mode == KoStore::Read )
    {
        m_fileMode = KoStoreBase::RemoteRead;
        m_localFileName = _filename;

    }
    else
    {
        m_fileMode = KoStoreBase::RemoteWrite;
        m_localFileName = "/var/tmp/kozip"; // ### FIXME with KTempFile
    }

    m_pZip = new KZip( m_localFileName );
    m_bGood = initZipStore( _mode, appIdentification ); // open the zip file and init some vars
}

KoZipStore::~KoZipStore()
{
    qCDebug(KOSTORE) <<"KoZipStore::~KoZipStore";
    if ( !m_bFinalized )
        finalize(); // ### no error checking when the app forgot to call finalize itself
    delete m_pZip;

    // Now we have still some job to do for remote files.
    if ( m_fileMode == KoStoreBase::RemoteRead )
    {
        QFile::remove( m_localFileName );
    }
    else if ( m_fileMode == KoStoreBase::RemoteWrite )
    {
        QFile file( m_localFileName );
        if( file.open( QFile::ReadOnly ) )
        {
            KIO::StoredTransferJob* transferJob = KIO::storedPut( file.readAll(), m_url, -1 );
            KJobWidgets::setWindow( transferJob, m_window );
            transferJob->exec();
        }
        // ### FIXME: delete temp file
    }
}

bool KoZipStore::initZipStore( Mode _mode, const QByteArray& appIdentification )
{
    KoStore::init( _mode );
    m_currentDir = 0;
    bool good = m_pZip->open( _mode == Write ? QIODevice::WriteOnly : QIODevice::ReadOnly );

    if ( good && _mode == Read )
        good = m_pZip->directory() != 0;
    else if ( good && _mode == Write )
    {
        //qCDebug(KOSTORE) <<"KoZipStore::init writing mimetype" << appIdentification;

        m_pZip->setCompression( KZip::NoCompression );
        m_pZip->setExtraField( KZip::NoExtraField );
        // Write identification
        m_pZip->writeFile( "mimetype", appIdentification );
        m_pZip->setCompression( KZip::DeflateCompression );
        // We don't need the extra field in KOffice - so we leave it as "no extra field".
    }
    return good;
}

bool KoZipStore::doFinalize()
{
    return m_pZip->close();
}

bool KoZipStore::openWrite( const QString& name )
{
#if 0
    // Prepare memory buffer for writing
    m_byteArray.resize( 0 );
    m_stream = new QBuffer( m_byteArray );
    m_stream->open( QIODevice::WriteOnly );
    return true;
#endif
    m_stream = 0L; // Don't use!
    return m_pZip->prepareWriting( name, "", "" /*m_pZip->rootDir()->user(), m_pZip->rootDir()->group()*/, 0 );
}

bool KoZipStore::openRead( const QString& name )
{
    const KArchiveEntry * entry = m_pZip->directory()->entry( name );
    if ( entry == 0L )
    {
        //qCWarning(KOSTORE) << "Unknown filename " << name;
        //return KIO::ERR_DOES_NOT_EXIST;
        return false;
    }
    if ( entry->isDirectory() )
    {
        qCWarning(KOSTORE) << name << " is a directory !";
        //return KIO::ERR_IS_DIRECTORY;
        return false;
    }
    // Must cast to KZipFileEntry, not only KArchiveFile, because device() isn't virtual!
    const KZipFileEntry * f = static_cast<const KZipFileEntry *>(entry);
    delete m_stream;
    m_stream = f->createDevice();
    m_iSize = f->size();
    return true;
}

qint64 KoZipStore::write( const char* _data, qint64 _len )
{
  if ( _len == 0L ) return 0;
  //qCDebug(KOSTORE) <<"KoZipStore::write" << _len;

  if ( !m_bIsOpen )
  {
    qCCritical(KOSTORE) << "KoStore: You must open before writing" << Qt::endl;
    return 0L;
  }
  if ( m_mode != Write  )
  {
    qCCritical(KOSTORE) << "KoStore: Can not write to store that is opened for reading" << Qt::endl;
    return 0L;
  }

  m_iSize += _len;
  if ( m_pZip->writeData( _data, _len ) ) // writeData returns a bool!
      return _len;
  return 0L;
}

bool KoZipStore::closeWrite()
{
    qCDebug(KOSTORE) <<"Wrote file" << m_sName <<" into ZIP archive. size"
                    << m_iSize << Qt::endl;
    return m_pZip->finishWriting( m_iSize );
#if 0
    if ( !m_pZip->writeFile( m_sName , "user", "group", m_iSize, m_byteArray.data() ) )
        qCWarning( KOSTORE ) << "Failed to write " << m_sName;
    m_byteArray.resize( 0 ); // save memory
    return true;
#endif
}

bool KoZipStore::enterRelativeDirectory( const QString& dirName )
{
    if ( m_mode == Read ) {
        if ( !m_currentDir ) {
            m_currentDir = m_pZip->directory(); // initialize
            Q_ASSERT( m_currentPath.isEmpty() );
        }
        const KArchiveEntry *entry = m_currentDir->entry( dirName );
        if ( entry && entry->isDirectory() ) {
            m_currentDir = dynamic_cast<const KArchiveDirectory*>( entry );
            return m_currentDir != 0;
        }
        return false;
    }
    else  // Write, no checking here
        return true;
}

bool KoZipStore::enterAbsoluteDirectory( const QString& path )
{
    if ( path.isEmpty() )
    {
        m_currentDir = 0;
        return true;
    }
    m_currentDir = dynamic_cast<const KArchiveDirectory*>( m_pZip->directory()->entry( path ) );
    Q_ASSERT( m_currentDir );
    return m_currentDir != 0;
}

bool KoZipStore::fileExists( const QString& absPath ) const
{
    const KArchiveEntry *entry = m_pZip->directory()->entry( absPath );
    return entry && entry->isFile();
}
