/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3biso9660.h"
#include "k3bdevicewrapperqiodevice.h"

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

#include <qcstring.h>
#include <qdir.h>
#include <qfile.h>
#include <qptrlist.h>

#include <kdebug.h>




class K3bFdFile : public QFile
{
public:
  K3bFdFile( int fd )
    : QFile(),
      m_fd(fd) {
    off_t pos = ::lseek( fd, 0, SEEK_CUR );
    if( pos < 0 )
      m_ourPosNull = 0;
    else
      m_ourPosNull = pos;
  }

  bool open( int m ) {
    return QFile::open( m, m_fd );
  }

  /**
   * reimplemented since we want the starting position
   * to be 0.
   */
  bool at( Offset pos ) {
    // QT seems to be unable to seek blcok devices.... :(
    return (::lseek( m_fd, m_ourPosNull + pos, SEEK_SET ) != -1);
    //    return QFile::at( m_ourPosNull + pos );
  }

private:
  int m_fd;
  Offset m_ourPosNull;
};






K3bIso9660File::K3bIso9660File( KArchive* archive, const QString& name, int access,
				int date, int adate,int cdate, const QString& user, const QString& group,
				const QString& symlink,int pos, int size)
  : KArchiveFile(archive, name, access, date, user, group, symlink, pos, size)
{
  m_adate=adate;
  m_cdate=cdate;
  m_algo[0]=0;m_algo[1]=0;m_parms[0]=0;m_parms[1]=0;m_realsize=0;
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

QByteArray K3bIso9660File::data(long long pos, int count) const
{
  QByteArray r;

  if( !archive()->device()->at( position() + pos ) ) {
    kdDebug() << "(K3bIso9660File) seek failed: " << strerror(errno) << " (" << errno << ")" << endl;
  }
  else if( r.resize( ((pos+count) < size()) ? count : size()-pos) ) {
    int rlen = archive()->device()->readBlock( r.data(), r.size() );
    if( rlen == -1 )
      r.resize(0);
    else if( rlen != (int)r.size() )
      r.resize(rlen);
  }
  
  return r;
}


int K3bIso9660File::read( long long pos, char* data, int len ) const
{
  if( !archive()->device()->at( position() + pos ) ) {
    kdDebug() << "(K3bIso9660File) seek failed: " << strerror(errno) << " (" << errno << ")" << endl;
    return -1;
  }
  else
    return archive()->device()->readBlock( data, ((pos+len) < size()) ? len : size()-pos );
}


QByteArray K3bIso9660File::data() const
{
  return data( 0, size() );
}


K3bIso9660Directory::K3bIso9660Directory( KArchive* archive, const QString& name, int access,
					  int date, int adate, int cdate, const QString& user, const QString& group,
					  const QString& symlink)
  : KArchiveDirectory(archive, name, access, date, user, group, symlink)
{
  m_adate=adate;
  m_cdate=cdate;
}

K3bIso9660Directory::~K3bIso9660Directory()
{
}





class K3bIso9660::Private
{
public:
  Private() 
    : rootDir(0),
      cdDevice(0),
      startSector(0) {
  }

  bool deleteDev;

  QPtrList<K3bIso9660Directory> elToritoDirs;
  QPtrList<K3bIso9660Directory> jolietDirs;
  QPtrList<K3bIso9660Directory> isoDirs; // RockRidge

  K3bIso9660Directory* rootDir;
  K3bIso9660SimplePrimaryDescriptor primaryDesc;

  K3bCdDevice::CdDevice* cdDevice;

  // only used for direkt K3bDevice access
  unsigned long startSector;
};


K3bIso9660::K3bIso9660( const QString& filename )
  : KArchive( new QFile( filename ) ),
    m_filename( filename )
{
  d = new Private();
  d->deleteDev = true;
}


K3bIso9660::K3bIso9660( int fd )
  : KArchive( new K3bFdFile( fd ) )
{
  d = new Private();
  d->deleteDev = true;
}


K3bIso9660::K3bIso9660( QIODevice * dev )
  : KArchive( dev )
{
  d = new Private();
  d->deleteDev = false;
}


K3bIso9660::K3bIso9660( K3bCdDevice::CdDevice* dev, unsigned long startSector )
  : KArchive( new K3bDeviceWrapperQIODevice(dev) )
{
  d = new Private();
  d->deleteDev = true;
  d->cdDevice = dev;
  d->startSector = startSector;
}


K3bIso9660::~K3bIso9660()
{
  // KArchive takes care of deleting the entries

  // mjarrett: Closes to prevent ~KArchive from aborting w/o device
  if( isOpened() )
    close();
  if ( d->deleteDev )
    delete device(); // we created it ourselves
  delete d;
}

/* callback function for libisofs */
static int readf(char *buf, int start, int len,void *udata) 
{
  QIODevice* dev = ( static_cast<K3bIso9660*> (udata) )->device();
  
  if( dev->at(start<<11) ) {
    if( dev->readBlock(buf, len<<11) != -1 )
      return len;
    else
      kdDebug() << "(K3bIso9660::ReadRequest) read with size " << (len<<11) << " failed." << endl;
  }
  else
    kdDebug() << "(K3bIso9660::ReadRequest) seek to " << (start<<11) << " failed." << endl;
  kdDebug() << "(K3bIso9660::ReadRequest) failed start: " << start << " len: " << len << endl;

  return -1;
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
  KArchiveEntry *entry=0,*oldentry=0;
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
      if (!special) path=rr.name;
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
      entry = new K3bIso9660File( iso, path, access, time, adate, cdate,
				  user, group, symlink, isonum_733(idr->extent)<<11,isonum_733(idr->size) );
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
			    isonum_731(bootdesc->boot_catalog)<<11, 2048 );
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
				isonum_731(((struct default_entry*) be->data)->start)<<11, size<<9 );
      dirent->addEntry(entry);
      be=be->next;
      i++;
    }

    FreeBootTable(&boot);
  }
}


bool K3bIso9660::openArchive( int mode )
{
  iso_vol_desc *desc;
  QString path,tmp,uid,gid;
  struct stat buf;
  int access,c_i,c_j;
  struct el_torito_boot_descriptor* bootdesc;

  if ( mode == IO_WriteOnly )
    return false;


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
	  d->isoDirs.append( dirent );
	 
	  // we use the first Iso/RR desc as our root
	  // if the user needs anything else there are the firstXXXEntry() methods
	  if( !d->rootDir ) {
	    d->rootDir = dirent;
	    setRootDir( dirent );
	  }
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
}


bool K3bIso9660::closeArchive()
{
  // KArchive's destructor calls this
  // so this is where we clean up
  // Since the first isoDir is the KArchive
  // root we must not delete it but all the
  // others.

  d->elToritoDirs.setAutoDelete(true);
  d->jolietDirs.setAutoDelete(true);
  d->elToritoDirs.clear();
  d->jolietDirs.clear();

  d->isoDirs.setAutoDelete(false);
  d->isoDirs.removeRef( d->rootDir );
  d->isoDirs.setAutoDelete(true);
  d->isoDirs.clear();

  d->rootDir = 0;

  return true;
}

bool K3bIso9660::writeDir( const QString&, const QString&, const QString& )
{
  return false;
}

bool K3bIso9660::prepareWriting( const QString&, const QString&, const QString&, uint )
{
  return false;
}

bool K3bIso9660::doneWriting( uint )
{
  return false;
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


const K3bIso9660SimplePrimaryDescriptor& K3bIso9660::primaryDescriptor() const
{
  return d->primaryDesc;
}


void K3bIso9660::virtual_hook( int id, void* data )
{
  KArchive::virtual_hook( id, data );
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

  debugEntry( directory(), 0 );
}


void K3bIso9660::debugEntry( const KArchiveEntry* entry, int depth ) const
{
  QString spacer;
  spacer.fill( ' ', depth*3 );

  kdDebug() << spacer << "- " << entry->name() << endl;
  if( entry->isDirectory() ) {
    const KArchiveDirectory* dir = dynamic_cast<const KArchiveDirectory*>(entry);
    QStringList entries = dir->entries();
    for( QStringList::const_iterator it = entries.begin(); it != entries.end(); ++it )
      debugEntry( dir->entry( *it ), depth+1 );
  }
}
