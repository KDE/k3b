/*
 *
 * $Id$
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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

#include "k3bvcdoptions.h"

#include <kconfig.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <qstring.h>
#include <qfile.h>


K3bVcdOptions::K3bVcdOptions()
  : m_volumeID( i18n("Project name", "VIDEOCD") ),
    m_volumeSetId( "" ),
    m_applicationId( "CDI/CDI_VCD.APP;1" ),
    m_systemId( "CD-RTOS CD-BRIDGE" ),
    m_vcdclass( "vcd" ),
    m_vcdversion( "2.0" ),
    m_volumeCount( 1 ),
    m_volumeNumber( 1 ),
    m_autodetect( true ),
    m_cdisupport( false ),
    m_brokensvcdmode( false ),
    m_sector2336( false )
{
}

bool K3bVcdOptions::checkCdiFiles()
{
  m_cdisize = 0;
  if( !QFile::exists( locate("data","k3b/cdi/cdi_imag.rtf") )) return false;
  if( !QFile::exists( locate("data","k3b/cdi/cdi_text.fnt") )) return false;
  if( !QFile::exists( locate("data","k3b/cdi/cdi_vcd.app") )) return false;  
  if( !QFile::exists( locate("data","k3b/cdi/cdi_vcd.cfg") )) return false;

  m_cdisize += QFile( locate("data","k3b/cdi/cdi_imag.rtf") ).size();
  m_cdisize += QFile( locate("data","k3b/cdi/cdi_text.fnt") ).size();
  m_cdisize += QFile( locate("data","k3b/cdi/cdi_vcd.app") ).size();
  m_cdisize += QFile( locate("data","k3b/cdi/cdi_vcd.cfg") ).size();

  return true;
}

void K3bVcdOptions::save( KConfig* c )
{
  c->writeEntry( "volume_id", m_volumeID );
  c->writeEntry( "album_id", m_albumID );
  c->writeEntry( "volume_set_id", m_volumeSetId );
  c->writeEntry( "preparer", m_preparer );
  c->writeEntry( "publisher", m_publisher );
  c->writeEntry( "volume_count", m_volumeCount );
  c->writeEntry( "volume_number", m_volumeNumber );
  c->writeEntry( "autodetect", m_autodetect );
  c->writeEntry( "cdi_support", m_cdisupport );
  c->writeEntry( "broken_svcd_mode", m_brokensvcdmode );
  c->writeEntry( "2336_sectors", m_sector2336 );
}


K3bVcdOptions K3bVcdOptions::load( KConfig* c )
{
  K3bVcdOptions options;

  options.setVolumeId( c->readEntry( "volume_id", options.volumeId() ) );
  options.setAlbumId( c->readEntry( "album_id", options.albumId() ) );
  options.setVolumeSetId( c->readEntry( "volume_set_id", options.volumeSetId() ) );
  options.setPreparer( c->readEntry( "preparer", options.preparer() ) );
  options.setPublisher( c->readEntry( "publisher", options.publisher() ) );
  options.setVolumeCount( ( c->readEntry( "volume_count", QString("%1").arg(options.volumeCount()) )).toInt() );
  options.setVolumeNumber( ( c->readEntry( "volume_number", QString("%1").arg(options.volumeNumber()) )).toInt() );
  options.setAutoDetect( c->readBoolEntry( "autodetect", options.AutoDetect() ) );
  options.setCdiSupport( c->readBoolEntry( "cdi_support", options.CdiSupport() ) );
  options.setBrokenSVcdMode( c->readBoolEntry( "broken_svcd_mode", options.BrokenSVcdMode() ) );
  options.setSector2336( c->readBoolEntry( "2336_sectors", options.Sector2336() ) );
  
  return options;
}


K3bVcdOptions K3bVcdOptions::defaults()
{
  // let the constructor create defaults
  return K3bVcdOptions();
}
