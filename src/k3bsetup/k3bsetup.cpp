/***************************************************************************
                          k3bsetup.cpp  -  description
                             -------------------
    begin                : Sat Dec  1 16:18:59 CET 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bsetup.h"

#include "../device/k3bdevicemanager.h"
#include "../device/k3bdevice.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../tools/k3bglobals.h"

#include <kconfig.h>
#include <ksimpleconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>

#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <unistd.h>
#include <fstab.h>


K3bSetup::K3bSetup( QObject* parent )
  : QObject( parent )
{
  // create a K3bDeviceManager
  m_externalBinManager = K3bExternalBinManager::self();
  m_deviceManager = K3bDeviceManager::self();


  m_config = new KSimpleConfig( K3b::globalConfig() );


  // initialize external programs
  // ================================================
  m_externalBinManager->search();

  if( m_config->hasGroup("External Programs") ) {
    m_config->setGroup( "External Programs" );
    m_externalBinManager->readConfig( m_config );
  }
  // ================================================


  // initialize devices
  // ================================================
  m_deviceManager->scanbus();

  if( m_config->hasGroup("Devices") ) {
    m_config->setGroup( "Devices" );
    m_deviceManager->readConfig( m_config );
  }
  // ================================================


  if( m_config->hasGroup( "Permissions" ) ) {
    m_config->setGroup( "Permissions" );
    m_cdwritingGroup = m_config->readEntry( "cdwriting_group", "cdrecording" );
    m_userList = m_config->readListEntry( "users" );
  }
}


K3bSetup::~K3bSetup()
{
}


bool K3bSetup::saveConfig()
{
  emit writingSettings();

  // save devices
  // -----------------------------------------------------------------------
  emit writingSetting( i18n("Saving CD devices to global configuration.") );

  if( m_config->hasGroup( "Devices" ) )
    m_config->deleteGroup( "Devices" );
  m_config->setGroup( "Devices" );
  m_deviceManager->saveConfig( m_config );

  emit settingWritten( true, i18n("Success") );
  // -----------------------------------------------------------------------

  // save external programs
  // -----------------------------------------------------------------------
  emit writingSetting( i18n("Writing external program paths to global configuration.") );

  if( m_config->hasGroup( "External Programs" ) )
    m_config->deleteGroup( "External Programs" );
  m_config->setGroup( "External Programs" );
  m_externalBinManager->saveConfig( m_config );

  emit settingWritten( true, i18n("Success") );
  // -----------------------------------------------------------------------


  emit writingSetting( i18n("Writing permission settings to global configuration.") );

  if( m_config->hasGroup( "Permissions" ) )
    m_config->deleteGroup( "Permissions" );
  m_config->setGroup( "Permissions" );
  m_config->writeEntry( "cdwriting_group", m_cdwritingGroup );
  m_config->writeEntry( "users", m_userList );

  emit settingWritten( true, i18n("Success") );

  if( m_applyDevicePermissions || m_applyExternalBinPermission )
  {
    uint groupid = createCdWritingGroup();

    if( m_applyDevicePermissions )
      doApplyDevicePermissions(groupid);
    if( m_applyExternalBinPermission )
      doApplyExternalProgramPermissions(groupid);
  }
  if( m_createFstabEntries )
    doCreateFstabEntries();

  m_config->sync();

  // let everybody read the global K3b config file
  chmod( QFile::encodeName(m_configPath), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );


  return true;
}


uint K3bSetup::createCdWritingGroup()
{
  if( m_cdwritingGroup.isEmpty() ) {
    kdDebug() << "(K3bSetup) setting cd writing group to 'cdrecording'." << endl;
    m_cdwritingGroup = "cdrecording";
  }

  // search group and create new if not found
  struct group* oldGroup = getgrnam( m_cdwritingGroup.local8Bit() );
  uint groupId;
  if( oldGroup == 0 ) {

    kdDebug() << "(K3bSetup) Could not find group " << m_cdwritingGroup << endl;

    // find new group id
    uint newId = 100;
    while( struct group* g = getgrent() ) {
      if( g->gr_gid == newId )
	newId = g->gr_gid + 1;
    }
    groupId = newId;
  }
  else {

    kdDebug() << "(K3bSetup) found group " << m_cdwritingGroup << endl;

    groupId = oldGroup->gr_gid;
  }

  endgrent();

  rename( "/etc/group", "/etc/group.k3bsetup" );

  QFile oldGroupFile( "/etc/group.k3bsetup" );
  QFile newGroupFile( "/etc/group" );
  oldGroupFile.open( IO_ReadOnly );
  newGroupFile.open( IO_WriteOnly );
  QTextStream oldGroupStream( &oldGroupFile );
  QTextStream newGroupStream( &newGroupFile );

  kdDebug() << "(K3bSetup) created textstreams" << endl;

  QString line = oldGroupStream.readLine();
  while( !line.isNull() ) {
    if( !line.startsWith( QString("%1:").arg(m_cdwritingGroup) ) )
      newGroupStream << line << "\n";
    line = oldGroupStream.readLine();
  }

  kdDebug() << "(K3bSetup) copied all groups except " << m_cdwritingGroup << endl;

  // add cdwriting group
  QStringList members;
  // save old members of the group
  if( oldGroup != 0 ) {

    kdDebug() << "(K3bSetup) importing group members..." << endl;

    int i = 0;
    while( oldGroup->gr_mem[i] != 0 ) {
      members.append( oldGroup->gr_mem[i] );
      i++;
    }

    kdDebug() << "(K3bSetup) imported group members" << endl;

  }
  members += m_userList;

  // remove double entries
  kdDebug() << "(K3bSetup) removing double entries" << endl;

  QStringList::Iterator i, j;
  for( i = members.begin(); i != members.end(); ++i )
    for( j = i; j != members.end(); ++j )
      if( i != j && *i == *j )
	j = members.remove( j );

  kdDebug() << "(K3bSetup) creating new entry" << endl;

  // write the new entry to the new group file
  QString entry = QString("%1::%2:").arg(m_cdwritingGroup).arg(groupId);
  i = members.begin();
  entry.append( *i );
  i++;
  for( ; i != members.end(); ++i )
    entry.append( QString(",%1").arg(*i) );

  kdDebug() << "(K3bSetup) writing entry to file" << endl;

  newGroupStream << entry << "\n";

  oldGroupFile.close();
  newGroupFile.close();

  // set the correct permissions (although they seem to be correct. Just to be sure!)
  chmod( "/etc/group", S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );

  return groupId;
}


void K3bSetup::doApplyDevicePermissions( uint groupId )
{
  emit writingSetting( i18n("Changing CD device permissions.") );

  // change owner for all devices and
  // change permissions for all devices

  K3bDevice* dev = m_deviceManager->allDevices().first();
  while( dev != 0 ) {

    if( dev->interfaceType() == K3bDevice::SCSI ) {
      if( QFile::exists( dev->genericDevice() ) ) {
	chown( QFile::encodeName(dev->genericDevice()), 0, groupId );
	chmod( QFile::encodeName(dev->genericDevice()), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP );
      }
      else {
	kdDebug() << "(K3bSetup) Could not find generic device: " << dev->genericDevice() << endl;
	emit error( i18n("Could not find generic device (%1)").arg(dev->genericDevice()) );
      }

      // TODO: serach for additionell devices like scdX, srX
    }

    if( QFile::exists( dev->ioctlDevice() ) ) {
      chown( QFile::encodeName(dev->ioctlDevice()), 0, groupId );
      chmod( QFile::encodeName(dev->ioctlDevice()), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP );
    }
    else {
      kdDebug() << "(K3bSetup) Could not find ioctl device: " << dev->ioctlDevice() << endl;
      emit error( i18n("Could not find ioctl device (%1)").arg(dev->ioctlDevice()) );
    }



    dev = m_deviceManager->allDevices().next();
  }

  emit settingWritten( true, i18n("Success") );
}


void K3bSetup::doApplyExternalProgramPermissions( uint groupId )
{
  static const char* programs[] = { "cdrecord",
				    "cdrdao" };
  static const int NUM_PROGRAMS = 2;

  for( int i = 0; i < NUM_PROGRAMS; ++i ) {
    const K3bExternalProgram* p = m_externalBinManager->program( programs[i] );

    emit writingSetting( i18n("Changing permissions for %1.").arg( programs[i] ) );

    for( QPtrListIterator<K3bExternalBin> it( p->bins() ); it.current(); ++it ) {
      const K3bExternalBin* binObject = *it;

      if( QFile::exists(binObject->path) ) {
	if( !binObject->version.isEmpty() ) {
	  kdDebug() << "(K3bSetup) setting permissions for " << programs[i] << "." << endl;
	  chown( QFile::encodeName(binObject->path), 0, groupId );
	  chmod( QFile::encodeName(binObject->path), S_ISUID|S_IRUSR|S_IWUSR|S_IXUSR|S_IXGRP );
	  emit settingWritten( true, i18n("Success") );
	}
	else {
	  emit settingWritten( false, i18n("%1 is not a %2 executable.").arg(binObject->path).arg(programs[i]) );
	  kdDebug() << "(K3bSetup) " << binObject->path << " is not " << programs[i] << "." << endl;
	}
      }
      else {
	emit settingWritten( false, i18n("Could not find %1.").arg(programs[i]) );
	kdDebug() << "(K3bSetup) could not find " << programs[i] << "." << endl;
      }
    }
  }
}


void K3bSetup::doCreateFstabEntries()
{
  // What this method really should do:
  // create a fstab entry for every device that
  // has an empty mountDevice or mountPoint


  QString fstabPath = QString::fromLatin1( _PATH_FSTAB );
  QString backupFstabPath = fstabPath + ".k3bsetup";
  emit writingSetting( i18n("Saving old %1 to %2").arg(fstabPath).arg(backupFstabPath) );

  kdDebug() << "(K3bSetup) creating new " << fstabPath << endl;
  kdDebug() << "(K3bSetup) saving backup to " << backupFstabPath << endl;
  
  // move /etc/fstab to /etc/fstab.k3bsetup
  rename( fstabPath.latin1(), backupFstabPath.latin1() );

  emit settingWritten( true, i18n("Success") );


  // create fstab entries or update fstab entries
  QFile oldFstabFile( backupFstabPath );
  oldFstabFile.open( IO_ReadOnly );
  QTextStream fstabStream( &oldFstabFile );
  
  QFile newFstabFile( fstabPath );
  newFstabFile.open( IO_WriteOnly );
  QTextStream newFstabStream( &newFstabFile );
  
  QString line = fstabStream.readLine();
  while( !line.isNull() ) {
    bool write = true;

    K3bDevice* dev = m_deviceManager->allDevices().first();
    while( dev != 0 ) {
      if( line.startsWith( dev->ioctlDevice() ) )
	write = false;
      dev = m_deviceManager->allDevices().next();
    }

    if( write ) {
      newFstabStream << line << "\n";
    }

    line = fstabStream.readLine();
  }

  // create entries for the devices
  K3bDevice* dev = m_deviceManager->allDevices().first();
  while( dev != 0 ) {

    emit writingSetting( i18n("Creating fstab entry for %1").arg(dev->ioctlDevice()) );

    bool createMountPoint = true;

    // TODO: check if mountpoint is empty
    // TODO: check if device mounted (unmount by default) KIO::findDeviceMountPoint

    // create mountpoint if it does not exist
    if( !QFile::exists( dev->mountPoint() ) ) {
      if( mkdir( QFile::encodeName(dev->mountPoint()), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH ) != 0 ) {
	emit error( i18n("Could not create mount point '%1'").arg(dev->mountPoint()) );
	createMountPoint = false;
      }
    }

    if( createMountPoint ) {
      // set the correct permissions for the mountpoint
      chmod( QFile::encodeName(dev->mountPoint()), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH );
	
      newFstabStream << dev->ioctlDevice() << "\t" 
		     << dev->mountPoint() << "\t"
		     << "auto" << "\t"
		     << "ro,noauto,user,exec" << "\t"
		     << "0 0" << "\n";

      emit settingWritten( true, i18n("Success") );
    }
    else
      emit settingWritten( false, QString::null );

    dev = m_deviceManager->allDevices().next();
  }
    
    
  newFstabFile.close();
  newFstabFile.close();

  // set the correct permissions (although they seem to be correct. Just to be sure!)
  chmod( fstabPath.latin1(), S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );
}


const QString& K3bSetup::cdWritingGroup() const
{
  return m_cdwritingGroup;
}


const QStringList& K3bSetup::users() const
{
  return m_userList; 
}


void K3bSetup::setCdWritingGroup( const QString& group )
{
  m_cdwritingGroup = group;
}


void K3bSetup::addUser( const QString& user )
{
  m_userList.append( user );
}


void K3bSetup::clearUsers()
{
  m_userList.clear();
}


#include "k3bsetup.moc"
