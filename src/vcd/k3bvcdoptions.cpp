/***************************************************************************
                          k3bvcdoptions.cpp  -  description
                             -------------------
    begin                : Sam Nov 23 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#include "k3bvcdoptions.h"

#include <kconfig.h>
#include <klocale.h>
#include <qstring.h>


K3bVcdOptions::K3bVcdOptions()
  : m_volumeID( i18n("Project name", "VIDEOCD") ),
    m_applicationID( "K3B" ),
    m_volumeSetId( "" ),
    m_brokensvcdmode( false ),
    m_sector2336( false ),
    m_volumeCount( 1 ),
    m_volumeNumber( 1 )
{

}


void K3bVcdOptions::save( KConfig* c )
{
  c->writeEntry( "volume_id", m_volumeID );
  c->writeEntry( "album_id", m_albumID );
  c->writeEntry( "preparer", m_preparer );
  c->writeEntry( "publisher", m_publisher );
  c->writeEntry( "system_id", m_systemId );
  c->writeEntry( "volume_set_id", m_volumeSetId );
  c->writeEntry( "broken_svcd_mode", m_brokensvcdmode );
  c->writeEntry( "2336_sectors", m_sector2336 );
  c->writeEntry( "volume_count", m_volumeCount );  
  c->writeEntry( "volume_number", m_volumeNumber );  
}


K3bVcdOptions K3bVcdOptions::load( KConfig* c )
{
  K3bVcdOptions options;

  options.setVolumeId( c->readEntry( "volume_id", options.volumeId() ) );
  options.setAlbumId( c->readEntry( "album_id", options.albumId() ) );
  options.setPreparer( c->readEntry( "preparer", options.preparer() ) );
  options.setPublisher( c->readEntry( "publisher", options.publisher() ) );
  options.setSystemId( c->readEntry( "system_id", options.systemId() ) );
  options.setVolumeSetId( c->readEntry( "volume_set_id", options.volumeSetId() ) );
  options.setBrokenSVcdMode( c->readBoolEntry( "broken_svcd_mode", options.BrokenSVcdMode() ) );
  options.setSector2336( c->readBoolEntry( "2336_sectors", options.Sector2336() ) );
  options.setVolumeCount( ( c->readEntry( "volume_count", QString("%1").arg(options.volumeCount()) )).toInt() );
  options.setVolumeNumber( ( c->readEntry( "volume_number", QString("%1").arg(options.volumeNumber()) )).toInt() );
  
  return options;
}


K3bVcdOptions K3bVcdOptions::defaults()
{
  // let the constructor create defaults
  return K3bVcdOptions();
}
