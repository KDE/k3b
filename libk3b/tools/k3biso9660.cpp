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

#include <config-k3b.h>
#include "k3bglobals.h"

#include "k3biso9660.h"
#include "k3biso9660backend.h"

#include "k3bdevice.h"

#include "libisofs/isofs.h"

#include <qdir.h>
#include <QtCore/QFile>

#include <QtCore/QDebug>


/* callback function for libisofs */
int K3b::Iso9660::read_callback( char* buf, sector_t start, int len, void* udata )
{
    K3b::Iso9660* isoF = static_cast<K3b::Iso9660*>(udata);

    return isoF->read( start, buf, len );
}

/* callback function for libisofs */
int K3b::Iso9660::isofs_callback( struct iso_directory_record *idr, void *udata )
{
    K3b::Iso9660 *iso = static_cast<K3b::Iso9660*> (udata);
    QString path, isoPath,user,group,symlink;
    int i;
    int access;
    int time,cdate,adate;
    rr_entry rr;
    bool special=false;
    K3b::Iso9660Entry *entry=0;
    //K3b::Iso9660Entry *oldentry=0;
    char z_algo[2],z_params[2];
    int z_size=0;

    if (isonum_711(idr->name_len)==1) {
        switch (idr->name[0]) {
        case 0:
            path+=(".");
            special=true;
            break;
        case 1:
            path+=("..");
            special=true;
            break;
        }
    }
    //
    // First extract the raw iso9660 name
    //
    if( !special ) {
        for( i = 0; i < isonum_711( idr->name_len ); i++ ) {
            if( idr->name[i] )
                isoPath += idr->name[i];
        }
    }
    else
        isoPath = path;

    //
    // Now see if we have RockRidge
    //
    if( !iso->plainIso9660() && ParseRR(idr,&rr) > 0 ) {
        iso->m_rr = true;
        if (!special)
            path = QString::fromLocal8Bit( rr.name );
        symlink=rr.sl;
        access=rr.mode;
        time=0;//rr.st_mtime;
        adate=0;//rr.st_atime;
        cdate=0;//rr.st_ctime;
        user.setNum(rr.uid);
        group.setNum(rr.gid);
        z_algo[0]=rr.z_algo[0];z_algo[1]=rr.z_algo[1];
        z_params[0]=rr.z_params[0];z_params[1]=rr.z_params[1];
        z_size=rr.z_size;
    }
    else {
        access=iso->dirent->permissions() & ~S_IFMT;
        adate=cdate=time=isodate_915(idr->date,0);
        user=iso->dirent->user();
        group=iso->dirent->group();
        if (idr->flags[0] & 2) access |= S_IFDIR; else access |= S_IFREG;
        if (!special) {
            if( !iso->plainIso9660() && iso->jolietLevel() ) {
                for (i=0;i<(isonum_711(idr->name_len)-1);i+=2) {
                    QChar ch( be2me_16(*((ushort*)&(idr->name[i]))) );
                    if (ch==';') break;
                    path+=ch;
                }
            }
            else {
                // no RR, no Joliet, just plain iso9660
                path = isoPath;

                // remove the version field
                int pos = path.indexOf( ';' );
                if( pos > 0 )
                    path.truncate( pos );
            }
            if (path.endsWith('.')) path.truncate(path.length()-1);
        }
    }

    if( !iso->plainIso9660() )
        FreeRR(&rr);

    if (idr->flags[0] & 2) {
        entry = new K3b::Iso9660Directory( iso, isoPath, path, access | S_IFDIR, time, adate, cdate,
                                         user, group, symlink,
                                         special ? 0 : isonum_733(idr->extent),
                                         special ? 0 : isonum_733(idr->size) );
    }
    else {
        entry = new K3b::Iso9660File( iso, isoPath, path, access, time, adate, cdate,
                                    user, group, symlink, isonum_733(idr->extent), isonum_733(idr->size) );
        if (z_size)
            (static_cast<K3b::Iso9660File*>(entry))->setZF( z_algo, z_params, z_size );
    }
    iso->dirent->addEntry(entry);

    return 0;
}



K3b::Iso9660Entry::Iso9660Entry( K3b::Iso9660* archive,
                                  const QString& isoName,
                                  const QString& name,
                                  int access,
                                  int date,
                                  int adate,
                                  int cdate,
                                  const QString& user,
                                  const QString& group,
                                  const QString& symlink )
    : m_adate( adate ),
      m_cdate( cdate ),
      m_name( name ),
      m_isoName( isoName ),
      m_date( date ),
      m_access( access ),
      m_user( user ),
      m_group( group ),
      m_symlink( symlink ),
      m_archive( archive )
{
}


K3b::Iso9660Entry::~Iso9660Entry()
{
}






K3b::Iso9660File::Iso9660File( K3b::Iso9660* archive,
                                const QString& isoName,
                                const QString& name,
                                int access,
                                int date,
                                int adate,
                                int cdate,
                                const QString& user,
                                const QString& group,
                                const QString& symlink,
                                unsigned int pos,
                                unsigned int size )
    : K3b::Iso9660Entry( archive, isoName, name, access, date, adate, cdate, user, group, symlink ),
      m_startSector(pos),
      m_size(size)
{
    m_algo[0] = 0;
    m_algo[1] = 0;
    m_parms[0] = 0;
    m_parms[1] = 0;
    m_realsize = 0;
}

K3b::Iso9660File::~Iso9660File()
{
}

void K3b::Iso9660File::setZF(char algo[2],char parms[2],int realsize)
{
    m_algo[0]=algo[0];m_algo[1]=algo[1];
    m_parms[0]=parms[0];m_parms[1]=parms[1];
    m_realsize=realsize;
}


int K3b::Iso9660File::read( unsigned int pos, char* data, int maxlen ) const
{
    if( pos >= size() )
        return 0;

    unsigned long startSec = m_startSector + pos/2048;
    int startSecOffset = pos%2048;
    char* buffer = data;
    bool buffered = false;
    unsigned long bufferLen = maxlen+startSecOffset;

    // cut to size
    if( pos + maxlen > size() )
        bufferLen = size() - pos + startSecOffset;

    // pad to 2048
    if( bufferLen%2048 )
        bufferLen += (2048-(bufferLen%2048));

    // we need to buffer if we changed the startSec or need a bigger buffer
    if( startSecOffset || bufferLen > (unsigned int)maxlen ) {
        buffered = true;
        buffer = new char[bufferLen];
    }

    int read = archive()->read( startSec, buffer, bufferLen/2048 )*2048;

    if( buffered ) {
        if( read > 0 ) {
            // cut to requested data
            read -= startSecOffset;
            if( read + pos > size() )
                read = size() - pos;
            if( read > maxlen )
                read = maxlen;

            ::memcpy( data, buffer+startSecOffset, read );
        }
        delete [] buffer;

        return read;
    }
    else {
        // cut read data
        if( read + pos > size() )
            read = size() - pos;

        return read;
    }
}


bool K3b::Iso9660File::copyTo( const QString& url ) const
{
    QFile of( url );
    if( of.open( QIODevice::WriteOnly ) ) {
        char buffer[2048*10];
        unsigned int pos = 0;
        int r = 0;
        while( ( r = read( pos, buffer, 2048*10 ) ) > 0 ) {
            of.write( buffer, r );
            pos += r;
        }

        return !r;
    }
    else {
        qDebug() << "(K3b::Iso9660File) could not open " << url << " for writing.";
        return false;
    }
}


K3b::Iso9660Directory::Iso9660Directory( K3b::Iso9660* archive,
                                          const QString& isoName,
                                          const QString& name,
                                          int access,
                                          int date,
                                          int adate,
                                          int cdate,
                                          const QString& user,
                                          const QString& group,
                                          const QString& symlink,
                                          unsigned int pos,
                                          unsigned int size  )
    : K3b::Iso9660Entry( archive, isoName, name, access, date, adate, cdate, user, group, symlink ),
      m_bExpanded( size == 0 ), // we can only expand entries that represent an actual directory
      m_startSector(pos),
      m_size(size)
{
}

K3b::Iso9660Directory::~Iso9660Directory()
{
    qDeleteAll( m_entries );
}


void K3b::Iso9660Directory::expand()
{
    if( !m_bExpanded ) {
        archive()->dirent = this;
        if( ProcessDir( &K3b::Iso9660::read_callback, m_startSector, m_size, &K3b::Iso9660::isofs_callback, archive() ) )
            qDebug() << "(K3b::Iso9660) failed to expand dir: " << name() << " with size: " << m_size;

        m_bExpanded = true;
    }
}


QStringList K3b::Iso9660Directory::entries() const
{
    // create a fake const method to fool the user ;)
    const_cast<K3b::Iso9660Directory*>(this)->expand();

    QStringList l;

    QHashIterator<QString, K3b::Iso9660Entry*> it( m_entries );
    while ( it.hasNext() ) {
        it.next();
        l.append( it.key() );
    }

    return l;
}


QStringList K3b::Iso9660Directory::iso9660Entries() const
{
    // create a fake const method to fool the user ;)
    const_cast<K3b::Iso9660Directory*>(this)->expand();

    QStringList l;

    QHashIterator<QString, K3b::Iso9660Entry*> it( m_iso9660Entries );
    while ( it.hasNext() ) {
        it.next();
        l.append( it.key() );
    }

    return l;
}


K3b::Iso9660Entry* K3b::Iso9660Directory::entry( const QString& n )
{
    if( n.isEmpty() )
        return 0;

    expand();

    QString name(n);

    // trailing slash ? -> remove
    if( name.length() > 1 && name[name.length()-1] == '/' ) {
        name.truncate( name.length()-1 );
    }

    int pos = name.indexOf( '/' );
    while( pos == 0 ) {
        if( name.length() > 1 ) {
            name = name.mid( 1 ); // remove leading slash
            pos = name.indexOf( '/' ); // look again
        }
        else // "/"
            return this;
    }

    if ( pos != -1 ) {
        QString left = name.left( pos );
        QString right = name.mid( pos + 1 );

        K3b::Iso9660Entry* e = m_entries[ left ];
        if ( !e || !e->isDirectory() )
            return 0;
        return static_cast<K3b::Iso9660Directory*>(e)->entry( right );
    }

    return m_entries[ name ];
}


K3b::Iso9660Entry* K3b::Iso9660Directory::iso9660Entry( const QString& n )
{
    if( n.isEmpty() )
        return 0;

    expand();

    QString name(n);

    // trailing slash ? -> remove
    if( name.length() > 1 && name[name.length()-1] == '/' ) {
        name.truncate( name.length()-1 );
    }

    int pos = name.indexOf( '/' );
    while( pos == 0 ) {
        if( name.length() > 1 ) {
            name = name.mid( 1 ); // remove leading slash
            pos = name.indexOf( '/' ); // look again
        }
        else // "/"
            return this;
    }

    if ( pos != -1 ) {
        QString left = name.left( pos );
        QString right = name.mid( pos + 1 );

        K3b::Iso9660Entry* e = m_iso9660Entries[ left ];
        if ( !e || !e->isDirectory() )
            return 0;
        return static_cast<K3b::Iso9660Directory*>(e)->iso9660Entry( right );
    }

    return m_iso9660Entries[ name ];
}


const K3b::Iso9660Entry* K3b::Iso9660Directory::entry( const QString& name ) const
{
    return const_cast<K3b::Iso9660Directory*>(this)->entry( name );
}


const K3b::Iso9660Entry* K3b::Iso9660Directory::iso9660Entry( const QString& name ) const
{
    return const_cast<K3b::Iso9660Directory*>(this)->iso9660Entry( name );
}


void K3b::Iso9660Directory::addEntry( K3b::Iso9660Entry* entry )
{
    m_entries.insert( entry->name(), entry );
    m_iso9660Entries.insert( entry->isoName(), entry );
}





class K3b::Iso9660::Private
{
public:
    Private()
        : cdDevice(0),
          fd(-1),
          isOpen(false),
          startSector(0),
          plainIso9660(false),
          backend(0) {
    }

    QList<K3b::Iso9660Directory*> elToritoDirs;
    QList<K3b::Iso9660Directory*> jolietDirs;
    QList<K3b::Iso9660Directory*> isoDirs;
    QList<K3b::Iso9660Directory*> rrDirs; // RockRidge

    K3b::Iso9660SimplePrimaryDescriptor primaryDesc;

    K3b::Device::Device* cdDevice;
    int fd;

    bool isOpen;

    // only used for direkt K3b::Device::Device access
    unsigned int startSector;

    bool plainIso9660;

    K3b::Iso9660Backend* backend;
};


K3b::Iso9660::Iso9660( const QString& filename )
    :  m_filename( filename )
{
    d = new Private();
}


K3b::Iso9660::Iso9660( int fd )
{
    d = new Private();
    d->fd = fd;
}


K3b::Iso9660::Iso9660( K3b::Iso9660Backend* backend )
{
    d = new Private();
    d->backend = backend;
}


K3b::Iso9660::Iso9660( K3b::Device::Device* dev, unsigned int startSector )
{
    d = new Private();
    d->cdDevice = dev;
    d->startSector = startSector;
}


K3b::Iso9660::~Iso9660()
{
    close();
    delete d->backend;
    delete d;
}


void K3b::Iso9660::setStartSector( unsigned int startSector )
{
    d->startSector = startSector;
}


void K3b::Iso9660::setPlainIso9660( bool b )
{
    d->plainIso9660 = b;
}


bool K3b::Iso9660::plainIso9660() const
{
    return d->plainIso9660;
}


int K3b::Iso9660::read( unsigned int sector, char* data, int count )
{
    if( count == 0 )
        return 0;
    else
        return d->backend->read( sector, data, count );
}


void K3b::Iso9660::addBoot(struct el_torito_boot_descriptor* bootdesc)
{
    int i,size;
    boot_head boot;
    boot_entry *be;
    QString path;
    K3b::Iso9660File *entry;

    entry=new K3b::Iso9660File( this, "Catalog", "Catalog", dirent->permissions() & ~S_IFDIR,
                              dirent->date(), dirent->adate(), dirent->cdate(),
                              dirent->user(), dirent->group(), QString(),
                              isonum_731(bootdesc->boot_catalog), 2048 );
    dirent->addEntry(entry);
    if (!ReadBootTable(&K3b::Iso9660::read_callback,isonum_731(bootdesc->boot_catalog),&boot,this)) {
        i=1;
        be=boot.defentry;
        while (be) {
            size=BootImageSize(&K3b::Iso9660::read_callback,
                               isonum_711(((struct default_entry*) be->data)->media),
                               isonum_731(((struct default_entry*) be->data)->start),
                               isonum_721(((struct default_entry*) be->data)->seccount),
                               this);
            path="Default Image";
            if (i>1) path += " (" + QString::number(i) + ')';
            entry=new K3b::Iso9660File( this, path, path, dirent->permissions() & ~S_IFDIR,
                                      dirent->date(), dirent->adate(), dirent->cdate(),
                                      dirent->user(), dirent->group(), QString(),
                                      isonum_731(((struct default_entry*) be->data)->start), size<<9 );
            dirent->addEntry(entry);
            be=be->next;
            i++;
        }

        FreeBootTable(&boot);
    }
}


bool K3b::Iso9660::open()
{
    if( d->isOpen )
        return true;

    if( !d->backend ) {
        // create a backend

        if( !m_filename.isEmpty() )
            d->backend = new K3b::Iso9660FileBackend( m_filename );

        else if( d->fd > 0 )
            d->backend = new K3b::Iso9660FileBackend( d->fd );

        else if( d->cdDevice ) {
            // now check if we have a scrambled video dvd
            if( d->cdDevice->copyrightProtectionSystemType() == K3b::Device::COPYRIGHT_PROTECTION_CSS ) {

                qDebug() << "(K3b::Iso9660) found encrypted dvd. using libdvdcss.";

                // open the libdvdcss stuff
                d->backend = new K3b::Iso9660LibDvdCssBackend( d->cdDevice );
                if( !d->backend->open() ) {
                    // fallback to devicebackend
                    delete d->backend;
                    d->backend = new K3b::Iso9660DeviceBackend( d->cdDevice );
                }
            }
            else
                d->backend = new K3b::Iso9660DeviceBackend( d->cdDevice );
        }
        else
            return false;
    }

    d->isOpen = d->backend->open();
    if( !d->isOpen )
        return false;

    iso_vol_desc *desc;
    QString path,tmp,uid,gid;
    k3b_struct_stat buf;
    int access,c_i,c_j;
    struct el_torito_boot_descriptor* bootdesc;

    // TODO implement win32 support

    /* We'll use the permission and user/group of the 'host' file except
     * in Rock Ridge, where the permissions are stored on the file system
     */
    if ( k3b_stat( QFile::encodeName(m_filename), &buf ) < 0 ) {
        /* defaults, if stat fails */
        memset(&buf,0,sizeof(k3b_struct_stat));
        buf.st_mode=0777;
    }
    uid.setNum(buf.st_uid);
    gid.setNum(buf.st_gid);
    access = buf.st_mode & ~S_IFMT;


    int c_b=1;
    c_i=1;c_j=1;

    desc = ReadISO9660( &K3b::Iso9660::read_callback, d->startSector, this );

    if (!desc) {
        qDebug() << "K3b::Iso9660::openArchive no volume descriptors";
        close();
        return false;
    }

    while (desc) {

        m_rr = false;

        switch (isonum_711(desc->data.type)) {
        case ISO_VD_BOOT:

            bootdesc=(struct el_torito_boot_descriptor*) &(desc->data);
            if( !memcmp( EL_TORITO_ID, bootdesc->system_id, ISODCL(8,39) ) ) {
                path="El Torito Boot";
                if( c_b > 1 )
                    path += " (" + QString::number(c_b) + ')';

                dirent = new K3b::Iso9660Directory( this, path, path, access | S_IFDIR,
                                                  buf.st_mtime, buf.st_atime, buf.st_ctime, uid, gid, QString() );
                d->elToritoDirs.append( dirent );

                addBoot(bootdesc);
                c_b++;
            }
            break;

        case ISO_VD_PRIMARY:
            createSimplePrimaryDesc( (struct iso_primary_descriptor*)&desc->data );
            // fall through
        case ISO_VD_SUPPLEMENTARY:
        {
            struct iso_primary_descriptor* primaryDesc = (struct iso_primary_descriptor*)&desc->data;
            struct iso_directory_record* idr = (struct iso_directory_record*)&primaryDesc->root_directory_record;

            m_joliet = JolietLevel(&desc->data);

            // skip joliet in plain iso mode
            if( m_joliet && plainIso9660() )
                break;

            if (m_joliet) {
                path = "Joliet level " + QString::number(m_joliet);
                if( c_j > 1 )
                    path += " (" + QString::number(c_j) + ')';
            }
            else {
                path = QString::fromLocal8Bit( primaryDesc->volume_id, 32 );
                if( c_i > 1 )
                    path += " (" + QString::number(c_i) + ')';
            }

            dirent = new K3b::Iso9660Directory( this, path, path, access | S_IFDIR,
                                              buf.st_mtime, buf.st_atime, buf.st_ctime, uid, gid, QString() );

            // expand the root entry
            ProcessDir( &K3b::Iso9660::read_callback, isonum_733(idr->extent),isonum_733(idr->size),&K3b::Iso9660::isofs_callback,this);

            if (m_joliet)
                c_j++;
            else
                c_i++;

            if( m_joliet )
                d->jolietDirs.append( dirent );
            else {
                if( m_rr )
                    d->rrDirs.append( dirent );
                d->isoDirs.append( dirent );
            }

            break;
        }
        }
        desc = desc->next;
    }

    FreeISO9660(desc);

    return true;
}


bool K3b::Iso9660::isOpen() const
{
    return d->isOpen;
}


void K3b::Iso9660::createSimplePrimaryDesc( struct iso_primary_descriptor* desc )
{
    d->primaryDesc.volumeId = QString::fromLocal8Bit( desc->volume_id, 32 ).trimmed();
    d->primaryDesc.systemId = QString::fromLocal8Bit( desc->system_id, 32 ).trimmed();
    d->primaryDesc.volumeSetId = QString::fromLocal8Bit( desc->volume_set_id, 128 ).trimmed();
    d->primaryDesc.publisherId = QString::fromLocal8Bit( desc->publisher_id, 128 ).trimmed();
    d->primaryDesc.preparerId = QString::fromLocal8Bit( desc->preparer_id, 128 ).trimmed();
    d->primaryDesc.applicationId = QString::fromLocal8Bit( desc->application_id, 128 ).trimmed();
    d->primaryDesc.volumeSetSize = isonum_723(desc->volume_set_size);
    d->primaryDesc.volumeSetNumber = isonum_723(desc->volume_set_size);
    d->primaryDesc.logicalBlockSize = isonum_723(desc->logical_block_size);
    d->primaryDesc.volumeSpaceSize = isonum_733(desc->volume_space_size);
}


void K3b::Iso9660::close()
{
    if( d->isOpen ) {
        d->backend->close();

        // Since the first isoDir is the KArchive
        // root we must not delete it but all the
        // others.

        qDeleteAll( d->elToritoDirs );
        qDeleteAll( d->jolietDirs );
        qDeleteAll( d->isoDirs );
        d->elToritoDirs.clear();
        d->jolietDirs.clear();
        d->isoDirs.clear();

        d->isOpen = false;
    }
}


const K3b::Iso9660Directory* K3b::Iso9660::firstJolietDirEntry() const
{
    if ( !d->jolietDirs.isEmpty() )
        return d->jolietDirs.first();
    else
        return 0;
}


const K3b::Iso9660Directory* K3b::Iso9660::firstIsoDirEntry() const
{
    if ( !d->isoDirs.isEmpty() )
        return d->isoDirs.first();
    else
        return 0;
}


const K3b::Iso9660Directory* K3b::Iso9660::firstElToritoEntry() const
{
    if ( !d->elToritoDirs.isEmpty() )
        return d->elToritoDirs.first();
    else
        return 0;
}


const K3b::Iso9660Directory* K3b::Iso9660::firstRRDirEntry() const
{
    if ( !d->rrDirs.isEmpty() )
        return d->rrDirs.first();
    else
        return 0;
}


const K3b::Iso9660SimplePrimaryDescriptor& K3b::Iso9660::primaryDescriptor() const
{
    return d->primaryDesc;
}


void K3b::Iso9660::debug() const
{
    if( isOpen() ) {
        qDebug() << "System Id:         " << primaryDescriptor().systemId;
        qDebug() << "Volume Id:         " << primaryDescriptor().volumeId;
        qDebug() << "Volume Set Id:     " << primaryDescriptor().volumeSetId;
        qDebug() << "Preparer Id:       " << primaryDescriptor().preparerId;
        qDebug() << "Publisher Id:      " << primaryDescriptor().publisherId;
        qDebug() << "Application Id:    " << primaryDescriptor().applicationId;
        qDebug() << "Volume Set Size:   " << primaryDescriptor().volumeSetSize;
        qDebug() << "Volume Set Number: " << primaryDescriptor().volumeSetNumber;

        if( firstIsoDirEntry() ) {
            qDebug() << "First ISO Dir entry:";
            qDebug() << "----------------------------------------------";
            debugEntry( firstIsoDirEntry(), 0 );
            qDebug() << "----------------------------------------------";
        }
        if( firstRRDirEntry() ) {
            qDebug() << "First RR Dir entry:";
            qDebug() << "----------------------------------------------";
            debugEntry( firstRRDirEntry(), 0 );
            qDebug() << "----------------------------------------------";
        }
        if( firstJolietDirEntry() ) {
            qDebug() << "First Joliet Dir entry:";
            qDebug() << "----------------------------------------------";
            debugEntry( firstJolietDirEntry(), 0 );
            qDebug() << "----------------------------------------------";
        }
    }
}


void K3b::Iso9660::debugEntry( const K3b::Iso9660Entry* entry, int depth ) const
{
    if( !entry ) {
        qDebug() << "(K3b::Iso9660::debugEntry) null entry.";
        return;
    }

    QString spacer;
    spacer.fill( ' ', depth*3 );
    qDebug() << spacer << "- " << entry->name() << " (" << entry->isoName() << ")";
    if( entry->isDirectory() ) {
        const K3b::Iso9660Directory* dir = dynamic_cast<const K3b::Iso9660Directory*>(entry);
        const QStringList entries = dir->entries();
        for( QStringList::const_iterator it = entries.constBegin(); it != entries.constEnd(); ++it ) {
            debugEntry( dir->entry( *it ), depth+1 );
        }
    }
}


K3b::Iso9660SimplePrimaryDescriptor::Iso9660SimplePrimaryDescriptor()
    : volumeSetSize(0),
      volumeSetNumber(0),
      logicalBlockSize(0),
      volumeSpaceSize(0)
{
}


bool K3b::operator==( const K3b::Iso9660SimplePrimaryDescriptor& d1,
                      const K3b::Iso9660SimplePrimaryDescriptor& d2 )
{
    return( d1.volumeId == d2.volumeId &&
            d1.systemId == d2.systemId &&
            d1.volumeSetId == d2.volumeSetId &&
            d1.publisherId == d2.publisherId &&
            d1.preparerId == d2.preparerId &&
            d1.applicationId == d2.applicationId &&
            d1.volumeSetSize == d2.volumeSetSize &&
            d1.volumeSetNumber == d2.volumeSetNumber &&
            d1.logicalBlockSize == d2.logicalBlockSize &&
            d1.volumeSpaceSize == d2.volumeSpaceSize );
}


bool K3b::operator!=( const K3b::Iso9660SimplePrimaryDescriptor& d1,
                      const K3b::Iso9660SimplePrimaryDescriptor& d2 )
{
    return( d1.volumeId != d2.volumeId ||
            d1.systemId != d2.systemId ||
            d1.volumeSetId != d2.volumeSetId ||
            d1.publisherId != d2.publisherId ||
            d1.preparerId != d2.preparerId ||
            d1.applicationId != d2.applicationId ||
            d1.volumeSetSize != d2.volumeSetSize ||
            d1.volumeSetNumber != d2.volumeSetNumber ||
            d1.logicalBlockSize != d2.logicalBlockSize ||
            d1.volumeSpaceSize != d2.volumeSpaceSize );
}
