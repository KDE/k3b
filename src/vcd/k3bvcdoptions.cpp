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
    m_volumeCount(1 ),
    m_volumeNumber(1 )
{

}


void K3bVcdOptions::save( KConfig* c )
{
  c->writeEntry( "volume id", m_volumeID );
  c->writeEntry( "album id", m_albumID );
  c->writeEntry( "preparer", m_preparer );
  c->writeEntry( "publisher", m_publisher );
  c->writeEntry( "system id", m_systemId );
  c->writeEntry( "volume set id", m_volumeSetId );
  // c->writeEntry( "volume count", m_volumeCount );  
  // c->writeEntry( "volume number", m_volumeNumber );  
}


K3bVcdOptions K3bVcdOptions::load( KConfig* c )
{
  K3bVcdOptions options;

  options.setVolumeId( c->readEntry( "volume id", options.volumeId() ) );
  options.setAlbumId( c->readEntry( "album id", options.albumId() ) );
  options.setPreparer( c->readEntry( "preparer", options.preparer() ) );
  options.setPublisher( c->readEntry( "publisher", options.publisher() ) );
  options.setSystemId( c->readEntry( "system id", options.systemId() ) );
  options.setVolumeSetId( c->readEntry( "volume set id", options.volumeSetId() ) );
  // options.setVolumeCount( c->readEntry( "volume count", options.volumeCount() ) );
  // options.setVolumeNumber( c->readEntry( "volume number", options.volumeNumber() ) );
  
  return options;
}


K3bVcdOptions K3bVcdOptions::defaults()
{
  // let the constructor create defaults
  return K3bVcdOptions();
}
