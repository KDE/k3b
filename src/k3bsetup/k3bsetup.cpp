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


K3bSetup::K3bSetup( QObject* parent )
  : QObject( parent )
{
  // create a K3bDeviceManager
  m_deviceManager = new K3bDeviceManager( this );
  m_externalBinManager = new K3bExternalBinManager( this );

  // this is a little not to hard hack to ensure that we get the "global" k3b appdir
  // k3bui.rc should always be in $KDEDIR/share/apps/k3b/
  m_configPath = KGlobal::dirs()->findResourceDir( "data", "k3b/k3bui.rc" ) + "k3b/k3bsetup";
  m_config = new KSimpleConfig( m_configPath );


  // initialize devices
  // ================================================
  m_deviceManager->scanbus();

  if( m_config->hasGroup("Devices") ) {
    m_config->setGroup( "Devices" );
    m_deviceManager->readConfig( m_config );
  }
  // ================================================


  // initialize external programs
  // ================================================
  m_externalBinManager->search();

  if( m_config->hasGroup("External Programs") ) {
    m_config->setGroup( "External Programs" );
    m_externalBinManager->readConfig( m_config );
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
  // save devices
  // -----------------------------------------------------------------------
  if( m_config->hasGroup( "Devices" ) )
    m_config->deleteGroup( "Devices" );
  m_config->setGroup( "Devices" );
  m_deviceManager->saveConfig( m_config );
  // -----------------------------------------------------------------------

  // save external programs
  // -----------------------------------------------------------------------
  if( m_config->hasGroup( "External Programs" ) )
    m_config->deleteGroup( "External Programs" );
  m_config->setGroup( "External Programs" );
  m_externalBinManager->saveConfig( m_config );
  // -----------------------------------------------------------------------

  if( m_config->hasGroup( "Permissions" ) )
    m_config->deleteGroup( "Permissions" );
  m_config->setGroup( "Permissions" );
  m_config->writeEntry( "cdwriting_group", m_cdwritingGroup );
  m_config->writeEntry( "users", m_userList );


  if( m_applyDevicePermissions )
    applyDevicePermissions();
  if( m_applyExternalBinPermission )
    applyExternalProgramPermissions();
  if( m_createFstabEntries )
    createFstabEntries();

  m_config->sync();

  // let everybody read the global K3b config file
  chmod( m_configPath.latin1(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );


  return true;
}


uint K3bSetup::createCdWritingGroup()
{
  if( m_cdwritingGroup.isEmpty() ) {
    qDebug("(K3bSetup) setting cd writing group to 'cdrecording'.");
    m_cdwritingGroup = "cdrecording";
  }

  // search group and create new if not found
  struct group* oldGroup = getgrnam( m_cdwritingGroup.latin1() );
  uint groupId;
  if( oldGroup == 0 ) {

    qDebug("(K3bSetup) Could not find group " + m_cdwritingGroup );

    // find new group id
    uint newId = 100;
    while( struct group* g = getgrent() ) {
      if( g->gr_gid == newId )
	newId = g->gr_gid + 1;
    }
    groupId = newId;
  }
  else {

    qDebug("(K3bSetup) found group " + m_cdwritingGroup );

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

  qDebug("(K3bSetup) created textstreams");

  QString line = oldGroupStream.readLine();
  while( !line.isNull() ) {
    if( !line.startsWith( QString("%1:").arg(m_cdwritingGroup) ) )
      newGroupStream << line << "\n";
    line = oldGroupStream.readLine();
  }

  qDebug("(K3bSetup) copied all groups except " + m_cdwritingGroup );

  // add cdwriting group
  QStringList members;
  // save old members of the group
  if( oldGroup != 0 ) {

    qDebug( "(K3bSetup) importing group members..." );

    int i = 0;
    while( oldGroup->gr_mem[i] != 0 ) {
      members.append( oldGroup->gr_mem[i] );
      i++;
    }

    qDebug( "(K3bSetup) imported group members" );

  }
  members += m_userList;

  // remove double entries
  qDebug("(K3bSetup) removing double entries");

  QStringList::Iterator i, j;
  for( i = members.begin(); i != members.end(); ++i )
    for( j = i; j != members.end(); ++j )
      if( i != j && *i == *j )
	j = members.remove( j );

  qDebug("(K3bSetup) creating new entry");

  // write the new entry to the new group file
  QString entry = QString("%1::%2:").arg(m_cdwritingGroup).arg(groupId);
  i = members.begin();
  entry.append( *i );
  i++;
  for( ; i != members.end(); ++i )
    entry.append( QString(",%1").arg(*i) );

  qDebug("(K3bSetup) writing entry to file");

  newGroupStream << entry << "\n";

  oldGroupFile.close();
  newGroupFile.close();

  // set the correct permissions (although they seem to be correct. Just to be sure!)
  chmod( "/etc/group", S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );

  return groupId;
}


void K3bSetup::applyDevicePermissions()
{
  uint groupId = createCdWritingGroup();

  // change owner for all devices and
  // change permissions for all devices

  K3bDevice* dev = m_deviceManager->allDevices().first();
  while( dev != 0 ) {

    if( QFile::exists( dev->genericDevice() ) ) {
      chown( QFile::encodeName(dev->genericDevice()), 0, groupId );
      chmod( QFile::encodeName(dev->genericDevice()), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP );
    }
    else {
      qDebug("(K3bSetup) Could not find generic device: " + dev->genericDevice() );
    }

    if( QFile::exists( dev->ioctlDevice() ) ) {
      chown( QFile::encodeName(dev->ioctlDevice()), 0, groupId );
      chmod( QFile::encodeName(dev->ioctlDevice()), S_IRUSR|S_IWUSR|S_IRGRP );
    }
    else {
      qDebug("(K3bSetup) Could not find ioctl device: " + dev->ioctlDevice() );
    }


    dev = m_deviceManager->allDevices().next();
  }
}


void K3bSetup::applyExternalProgramPermissions()
{
  uint groupId = createCdWritingGroup();

  static const char* programs[] = { "cdrecord",
				    "mkisofs",
				    "cdrdao" };
  static const int NUM_PROGRAMS = 3;

  for( int i = 0; i < NUM_PROGRAMS; ++i ) {
    K3bExternalBin* binObject = m_externalBinManager->binObject( programs[i] );
    if( QFile::exists(binObject->path) ) {
      if( !binObject->version.isEmpty() ) {
	qDebug("(K3bSetup) setting permissions for %s.", programs[i] );
	chown( QFile::encodeName(binObject->path), 0, groupId );
	chmod( QFile::encodeName(binObject->path), S_ISUID|S_IRUSR|S_IWUSR|S_IXUSR|S_IXGRP );
      }
      else
	qDebug("(K3bSetup) %s is no not %s.", binObject->path.latin1(), programs[i] );
    }
    else
      qDebug("(K3bSetup) could not find %s.", programs[i] );
  }
}


void K3bSetup::createFstabEntries()
{
  qDebug("(K3bSetup) creating new /etc/fstab");
  qDebug("(K3bSetup) saving backup to /etc/fstab.k3bsetup");
  
  // move /etc/fstab to /etc/fstab.k3bsetup
  rename( "/etc/fstab", "/etc/fstab.k3bsetup" );


  // create fstab entries or update fstab entries
  QFile oldFstabFile( "/etc/fstab.k3bsetup" );
  oldFstabFile.open( IO_ReadOnly );
  QTextStream fstabStream( &oldFstabFile );
  
  QFile newFstabFile( "/etc/fstab" );
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

    bool createMountPoint = true;

    // TODO: check if mountpoint is empty
    // TODO: check if device mounted (unmount by default) KIO::findDeviceMountPoint

    // create mountpoint if it does not exist
    if( !QFile::exists( dev->mountPoint() ) ) {
      if( mkdir( dev->mountPoint().latin1(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH ) != 0 ) {
	KMessageBox::error( 0, i18n("Could not create mount point '%1'\nNo fstab entry will be created for device %2").arg(dev->mountPoint()).arg(dev->ioctlDevice()) );
	createMountPoint = false;
      }
    }

    if( createMountPoint ) {
      // set the correct permissions for the mountpoint
      chmod( dev->mountPoint().latin1(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH );
	
      newFstabStream << dev->ioctlDevice() << "\t" 
		     << dev->mountPoint() << "\t"
		     << "auto" << "\t"
		     << "ro,noauto,user,exec" << "\t"
		     << "0 0" << "\n";
    }

    dev = m_deviceManager->allDevices().next();
  }
    
    
  newFstabFile.close();
  newFstabFile.close();

  // set the correct permissions (although they seem to be correct. Just to be sure!)
  chmod( "/etc/fstab", S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );
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
