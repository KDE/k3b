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

#include <kconfig.h>

#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <unistd.h>


K3bSetup::K3bSetup()
{
}

K3bSetup::~K3bSetup()
{
}


bool K3bSetup::loadConfig( KConfig* c )
{
  m_cdwritingGroup = c->readEntry( "cdwriting_group", "cdrecording" );
  m_userList = c->readListEntry( "users" );
  return true;
}


bool K3bSetup::saveConfig( KConfig* c )
{
  c->writeEntry( "cdwriting_group", m_cdwritingGroup );
  c->writeEntry( "users", m_userList );
  return true;
}


uint K3bSetup::createCdWritingGroup()
{
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


void K3bSetup::applyDevicePermissions( K3bDeviceManager* deviceManager )
{
  uint groupId = createCdWritingGroup();

  // change owner for all devices and
  // change permissions for all devices

  K3bDevice* dev = deviceManager->allDevices().first();
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


    dev = deviceManager->allDevices().next();
  }
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
