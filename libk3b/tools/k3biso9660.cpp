/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3biso9660.h"

#include <k3bdevice.h>
#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bmsf.h>

#include "libisofs/isofs.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <qcstring.h>
#include <qdir.h>
#include <qfile.h>
#include <qptrlist.h>

#include <kdebug.h>

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif



K3bIso9660Entry::K3bIso9660Entry( K3bIso9660* archive,
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
    m_date( date ),
    m_access( access ),
    m_user( user ),
    m_group( group ),
    m_symlink( symlink ),
    m_archive( archive )
{
}


K3bIso9660Entry::~K3bIso9660Entry()
{
}






K3bIso9660File::K3bIso9660File( K3bIso9660* archive, 
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
  : K3bIso9660Entry( archive, name, access, date, adate, cdate, user, group, symlink ),
    m_startSector(pos),
    m_size(size)
{
  m_algo[0] = 0;
  m_algo[1] = 0;
  m_parms[0] = 0;
  m_parms[1] = 0;
  m_realsize = 0;
}

K3bIso9660File::~K3bIso9660File()
{
}

void K3bIso9660File::setZF(char algo[2],char parms[2],int realsize)
{
  m_algo[0]=algo[0];m_algo[1]=algo[1];
  m_parms[0]=parms[0];m_parms[1]=parms[1];
  m_realsize=realsize;
}


int K3bIso9660File::read( unsigned int pos, char* data, int maxlen ) const
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


K3bIso9660Directory::K3bIso9660Directory( K3bIso9660* archive, 
					  const QString& name, 
					  int access,
					  int date, 
					  int adate, 
					  int cdate, 
					  const QString& user, 
					  const QString& group,
					  const QString& symlink )
  : K3bIso9660Entry( archive, name, access, date, adate, cdate, user, group, symlink )
{
  m_entries.setAutoDelete( true );
}

K3bIso9660Directory::~K3bIso9660Directory()
{
}


QStringList K3bIso9660Directory::entries() const
{
  QStringList l;
  
  QDictIterator<K3bIso9660Entry> it( m_entries );
  for( ; it.current(); ++it )
    l.append( it.currentKey() );
  
  return l;
}


K3bIso9660Entry* K3bIso9660Directory::entry( const QString& n )
{
  QString name(n);
  int pos = name.find( '/' );
  if( pos == 0 ) {
    if( name.length() > 1 ) {
      name = name.mid( 1 ); // remove leading slash
      pos = name.find( '/' ); // look again
    }
    else // "/"
      return this;
  }

  // trailing slash ? -> remove
  if( pos != -1 && pos == (int)name.length()-1 ) {
    name = name.left( pos );
    pos = name.find( '/' ); // look again
  }

  if ( pos != -1 ) {
    QString left = name.left( pos );
    QString right = name.mid( pos + 1 );

    K3bIso9660Entry* e = m_entries[ left ];
    if ( !e || !e->isDirectory() )
      return 0;
    return static_cast<K3bIso9660Directory*>(e)->entry( right );
  }

  return m_entries[ name ];
}


const K3bIso9660Entry* K3bIso9660Directory::entry( const QString& name ) const
{
  return const_cast<K3bIso9660Directory*>(this)->entry( name );
}


void K3bIso9660Directory::addEntry( K3bIso9660Entry* entry )
{
  m_entries.insert( entry->name(), entry );
}





class K3bIso9660::Private
{
public:
  Private() 
    : cdDevice(0),
      fd(-1),
      closeFd(false),
      startSector(0) {
  }

  bool deleteDev;

  QPtrList<K3bIso9660Directory> elToritoDirs;
  QPtrList<K3bIso9660Directory> jolietDirs;
  QPtrList<K3bIso9660Directory> isoDirs;
  QPtrList<K3bIso9660Directory> rrDirs; // RockRidge

  K3bIso9660SimplePrimaryDescriptor primaryDesc;

  K3bCdDevice::CdDevice* cdDevice;
  int fd;
  bool closeFd;

  // only used for direkt K3bDevice access
  unsigned int startSector;
};


K3bIso9660::K3bIso9660( const QString& filename )
  :  m_filename( filename )
{
  d = new Private();
}


K3bIso9660::K3bIso9660( int fd )
{
  d = new Private();
  d->fd = fd;
}



K3bIso9660::K3bIso9660( K3bCdDevice::CdDevice* dev, unsigned int startSector )
{
  d = new Private();
  d->cdDevice = dev;
  d->startSector = startSector;
}


K3bIso9660::~K3bIso9660()
{
  close();
  delete d;
}


int K3bIso9660::read( unsigned int sector, char* data, int count )
{
  if( count == 0 )
    return 0;

  int read = -1;

  if( d->cdDevice != 0 ) {

    int retries = 10;  // TODO: no fixed value
    while( retries && !d->cdDevice->read10( (unsigned char*)data,
					    count*2048,
					    sector,
					    count ) )
      retries--;

    if( retries > 0 )
      read = count;

  
    // fallback
    if( read < 0 ) {
      kdDebug() << "(K3bIso9660) falling back to stdlib read" << endl;
      if( ::lseek( d->cdDevice->open(), static_cast<unsigned long long>(sector)*2048, SEEK_SET ) == -1 )
	kdDebug() << "(K3bIso9660) seek failed." << endl;
      else {
	read = ::read( d->cdDevice->open(), data, count*2048 );
	if( read < 0 )
	  kdDebug() << "(K3bIso9660) stdlib read failed." << endl;
	else
	  read /= 2048;
      }
    }

    return read;
  }
  else {
    if( ::lseek( d->fd, static_cast<unsigned long long>(sector)*2048, SEEK_SET ) != -1 )
      if( (read = ::read( d->fd, data, count*2048 )) != -1 )
	return read / 2048;

    return -1;
  }
}


/* callback function for libisofs */
static int readf( char* buf, sector_t start, int len, void* udata ) 
{
  K3bIso9660* isoF = static_cast<K3bIso9660*>(udata);
  
  return isoF->read( start, buf, len );
}

/* callback function for libisofs */
static int mycallb(struct iso_directory_record *idr,void *udata) 
{
  K3bIso9660 *iso = static_cast<K3bIso9660*> (udata);
  QString path,user,group,symlink;
  int i;
  int access;
  int time,cdate,adate;
  rr_entry rr;
  bool special=false;
  K3bIso9660Entry *entry=0,*oldentry=0;
  char z_algo[2],z_params[2];
  int z_size=0;

  if (iso->level) {
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
    if(ParseRR(idr,&rr)>0) {
      iso->m_rr = true;
      if (!special)
	path=rr.name;
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
    } else {
      access=iso->dirent->permissions() & ~S_IFMT;
      adate=cdate=time=isodate_915(idr->date,0);
      user=iso->dirent->user();
      group=iso->dirent->group();
      if (idr->flags[0] & 2) access |= S_IFDIR; else access |= S_IFREG;
      if (!special) {
	if (iso->jolietLevel()) {
	  for (i=0;i<(isonum_711(idr->name_len)-1);i+=2) {
	    QChar ch( be2me_16(*((ushort*)&(idr->name[i]))) );
	    if (ch==';') break;
	    path+=ch;
	  }
	} else {
	  for (i=0;i<isonum_711(idr->name_len);i++) {
	    if (idr->name[i]==';') break;
	    if (idr->name[i]) path+=(idr->name[i]);
	  }
	}
	if (path.endsWith(".")) path.setLength(path.length()-1);
      }
    }
    FreeRR(&rr);
    if (idr->flags[0] & 2) {
      entry = new K3bIso9660Directory( iso, path, access | S_IFDIR, time, adate, cdate,
				       user, group, symlink );
    } else {
//       kdDebug() << "(K3bIso9660) creating file: " << path << " start: " << isonum_733(idr->extent)
// 		<< " size: " << isonum_733(idr->size) << endl;
      entry = new K3bIso9660File( iso, path, access, time, adate, cdate,
				  user, group, symlink, isonum_733(idr->extent), isonum_733(idr->size) );
      if (z_size)
	(static_cast<K3bIso9660File*>(entry))->setZF( z_algo, z_params, z_size );
    }
    iso->dirent->addEntry(entry);
  }

  if ( (idr->flags[0] & 2) && (iso->level==0 || !special) ) {
    if (iso->level) {
      oldentry=iso->dirent;
      iso->dirent=static_cast<K3bIso9660Directory*> (entry);
    }
    iso->level++;
    ProcessDir( &readf, isonum_733(idr->extent),isonum_733(idr->size),&mycallb,udata);
    iso->level--;
    if(iso->level)
      iso->dirent = static_cast<K3bIso9660Directory*>(oldentry);
  }
  return 0;
}


void K3bIso9660::addBoot(struct el_torito_boot_descriptor* bootdesc)
{
  int i,size;
  boot_head boot;
  boot_entry *be;
  QString path;
  K3bIso9660File *entry;
    
  entry=new K3bIso9660File( this, "Catalog", dirent->permissions() & ~S_IFDIR,
			    dirent->date(), dirent->adate(), dirent->cdate(),
			    dirent->user(), dirent->group(), QString::null,
			    isonum_731(bootdesc->boot_catalog), 2048 );
  dirent->addEntry(entry);
  if (!ReadBootTable(&readf,isonum_731(bootdesc->boot_catalog),&boot,this)) {
    i=1;
    be=boot.defentry;
    while (be) {
      size=BootImageSize(&readf,
			 isonum_711(((struct default_entry*) be->data)->media),
			 isonum_731(((struct default_entry*) be->data)->start),
			 isonum_721(((struct default_entry*) be->data)->seccount),
			 this);
      path="Default Image";
      if (i>1) path += " (" + QString::number(i) + ")";
      entry=new K3bIso9660File( this, path, dirent->permissions() & ~S_IFDIR,
				dirent->date(), dirent->adate(), dirent->cdate(),
				dirent->user(), dirent->group(), QString::null,
				isonum_731(((struct default_entry*) be->data)->start), size<<9 );
      dirent->addEntry(entry);
      be=be->next;
      i++;
    }

    FreeBootTable(&boot);
  }
}


bool K3bIso9660::open()
{
  // open the file
  if( !m_filename.isEmpty() ) {
    d->fd = ::open( QFile::encodeName( m_filename ), O_RDONLY|O_LARGEFILE );
    if( d->fd < 0 )
      return false;
    d->closeFd = true;
  }
  else if( d->cdDevice ) {
    if( d->cdDevice->open() < 0 )
      return false;
  }
  else if( d->fd < 0 )
    return false;

  iso_vol_desc *desc;
  QString path,tmp,uid,gid;
  struct stat buf;
  int access,c_i,c_j;
  struct el_torito_boot_descriptor* bootdesc;


  /* We'll use the permission and user/group of the 'host' file except
   * in Rock Ridge, where the permissions are stored on the file system
   */
  if (::stat( m_filename.local8Bit(), &buf )<0) {
    /* defaults, if stat fails */
    memset(&buf,0,sizeof(struct stat));
    buf.st_mode=0777;
  }
  uid.setNum(buf.st_uid);
  gid.setNum(buf.st_gid);
  access = buf.st_mode & ~S_IFMT;


  int c_b=1;
  c_i=1;c_j=1;

  desc = ReadISO9660( &readf, d->startSector, this );

  if (!desc) {
    kdDebug() << "K3bIso9660::openArchive no volume descriptors" << endl;
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
	  path += " (" + QString::number(c_b) + ")";
                        
	dirent = new K3bIso9660Directory( this, path, access | S_IFDIR,
					  buf.st_mtime, buf.st_atime, buf.st_ctime, uid, gid, QString::null );
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
	if (m_joliet) {
	  path = "Joliet level " + QString::number(m_joliet);
	  if( c_j > 1 )
	    path += " (" + QString::number(c_j) + ")";
	} 
	else {
	  path = QString::fromLocal8Bit( primaryDesc->volume_id, 32 ); 
	  if( c_i > 1 )
	    path += " (" + QString::number(c_i) + ")";
	}
	
	dirent = new K3bIso9660Directory( this, path, access | S_IFDIR,
					  buf.st_mtime, buf.st_atime, buf.st_ctime, uid, gid, QString::null );

	if( m_joliet )
	  d->jolietDirs.append( dirent );
	else {
	  if( m_rr )
	    d->rrDirs.append( dirent );
	  d->isoDirs.append( dirent );
	}

	level=0;
	mycallb( idr, this );
	if (m_joliet)
	  c_j++;
	else
	  c_i++;
	break;
      }
    }
    desc = desc->next;
  }

  FreeISO9660(desc);
  
  return true;
}


void K3bIso9660::createSimplePrimaryDesc( struct iso_primary_descriptor* desc )
{
  d->primaryDesc.volumeId = QString::fromLocal8Bit( desc->volume_id, 32 ).stripWhiteSpace();
  d->primaryDesc.systemId = QString::fromLocal8Bit( desc->system_id, 32 ).stripWhiteSpace();
  d->primaryDesc.volumeSetId = QString::fromLocal8Bit( desc->volume_set_id, 128 ).stripWhiteSpace();
  d->primaryDesc.publisherId = QString::fromLocal8Bit( desc->publisher_id, 128 ).stripWhiteSpace();
  d->primaryDesc.preparerId = QString::fromLocal8Bit( desc->preparer_id, 128 ).stripWhiteSpace();
  d->primaryDesc.applicationId = QString::fromLocal8Bit( desc->application_id, 128 ).stripWhiteSpace();
  d->primaryDesc.volumeSetSize = isonum_723(desc->volume_set_size);
  d->primaryDesc.volumeSetNumber = isonum_723(desc->volume_set_size);
  d->primaryDesc.logicalBlockSize = isonum_723(desc->logical_block_size);
  d->primaryDesc.volumeSpaceSize = isonum_733(desc->volume_space_size);
}


void K3bIso9660::close()
{
  if( d->closeFd ) {
    ::close( d->fd );
    d->closeFd = false;
    d->fd = -1;
  }
  else if( d->cdDevice )
    d->cdDevice->close();

  // Since the first isoDir is the KArchive
  // root we must not delete it but all the
  // others.

  d->elToritoDirs.setAutoDelete(true);
  d->jolietDirs.setAutoDelete(true);
  d->isoDirs.setAutoDelete(true);
  d->elToritoDirs.clear();
  d->jolietDirs.clear();
  d->isoDirs.clear();
}


const K3bIso9660Directory* K3bIso9660::firstJolietDirEntry() const
{
  return d->jolietDirs.first();
}


const K3bIso9660Directory* K3bIso9660::firstIsoDirEntry() const
{
  return d->isoDirs.first();
}


const K3bIso9660Directory* K3bIso9660::firstElToritoEntry() const
{
  return d->elToritoDirs.first();
}


const K3bIso9660Directory* K3bIso9660::firstRRDirEntry() const
{
  return d->rrDirs.first();
}


const K3bIso9660SimplePrimaryDescriptor& K3bIso9660::primaryDescriptor() const
{
  return d->primaryDesc;
}


void K3bIso9660::debug() const
{
  kdDebug() << "System Id:         " << primaryDescriptor().systemId << endl;
  kdDebug() << "Volume Id:         " << primaryDescriptor().volumeId << endl;
  kdDebug() << "Volume Set Id:     " << primaryDescriptor().volumeSetId << endl;
  kdDebug() << "Preparer Id:       " << primaryDescriptor().preparerId << endl;
  kdDebug() << "Publisher Id:      " << primaryDescriptor().publisherId << endl;
  kdDebug() << "Application Id:    " << primaryDescriptor().applicationId << endl;
  kdDebug() << "Volume Set Size:   " << primaryDescriptor().volumeSetSize << endl;
  kdDebug() << "Volume Set Number: " << primaryDescriptor().volumeSetNumber << endl;

  debugEntry( firstIsoDirEntry(), 0 );
}


void K3bIso9660::debugEntry( const K3bIso9660Entry* entry, int depth ) const
{
  QString spacer;
  spacer.fill( ' ', depth*3 );

  kdDebug() << spacer << "- " << entry->name() << endl;
  if( entry->isDirectory() ) {
    const K3bIso9660Directory* dir = dynamic_cast<const K3bIso9660Directory*>(entry);
    QStringList entries = dir->entries();
    for( QStringList::const_iterator it = entries.begin(); it != entries.end(); ++it )
      debugEntry( dir->entry( *it ), depth+1 );
  }
}
