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

// Kde Includes
#include <kapplication.h>
#include <kconfig.h>
#include <k3bcore.h>
#include <klocale.h>
#include <kstandarddirs.h>

// Qt Includes
#include <qstring.h>
#include <qfile.h>

// K3b Includes
#include "k3bvcdoptions.h"
#include <k3bversion.h>

K3bVcdOptions::K3bVcdOptions()
        : m_restriction( 0 ),
        m_pbcenabled( PbcEnabled() ),
        m_pbcnumkeys( PbcNumKeys() ),
        m_volumeID( i18n( "VIDEOCD" ) ),
        m_albumID( "" ),
        m_volumeSetId( "" ),
        m_publisher( QString( "K3b - Version %1" ).arg( k3bcore->version() ) ),
        m_applicationId( "CDI/CDI_VCD.APP;1" ),
        m_systemId( "CD-RTOS CD-BRIDGE" ),
        m_vcdclass( "vcd" ),
        m_vcdversion( "2.0" ),
        m_pregapleadout( 150 ),
        m_pregaptrack( 150 ),
        m_frontmargintrack( 30 ),
        m_rearmargintrack( 45 ),
        m_frontmargintrackSVCD( 0 ),
        m_rearmargintrackSVCD( 0 ),
        m_mpegversion( 1 ),
        m_volumeCount( 1 ),
        m_volumeNumber( 1 ),
        m_autodetect( true ),
        m_cdisupport( false ),
        m_brokensvcdmode( false ),
        m_sector2336( false ),
        m_updatescanoffsets( false ),
        m_relaxedaps( false ),
        m_segmentfolder( true ),
        m_usegaps( false )
{
}

bool K3bVcdOptions::checkCdiFiles()
{
    m_cdisize = 0;
    if ( !QFile::exists( locate( "data", "k3b/cdi/cdi_imag.rtf" ) ) )
        return false;
    if ( !QFile::exists( locate( "data", "k3b/cdi/cdi_text.fnt" ) ) )
        return false;
    if ( !QFile::exists( locate( "data", "k3b/cdi/cdi_vcd.app" ) ) )
        return false;
    if ( !QFile::exists( locate( "data", "k3b/cdi/cdi_vcd.cfg" ) ) )
        return false;

    m_cdisize += QFile( locate( "data", "k3b/cdi/cdi_imag.rtf" ) ).size();
    m_cdisize += QFile( locate( "data", "k3b/cdi/cdi_text.fnt" ) ).size();
    m_cdisize += QFile( locate( "data", "k3b/cdi/cdi_vcd.app" ) ).size();
    m_cdisize += QFile( locate( "data", "k3b/cdi/cdi_vcd.cfg" ) ).size();

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
    c->writeEntry( "UpdateScanOffsets", m_updatescanoffsets );
    c->writeEntry( "RelaxedAps", m_relaxedaps );
    c->writeEntry( "PbcEnabled", m_pbcenabled );
    c->writeEntry( "SegmentFolder", m_segmentfolder );
    c->writeEntry( "Restriction", m_restriction );
    c->writeEntry( "PreGapLeadout", m_pregapleadout );
    c->writeEntry( "PreGapTrack", m_pregaptrack );
    c->writeEntry( "FrontMarginTrack", m_frontmargintrack );
    c->writeEntry( "RearMarginTrack", m_rearmargintrack );
    c->writeEntry( "UseGaps", m_usegaps );
}


K3bVcdOptions K3bVcdOptions::load( KConfig* c )
{
    K3bVcdOptions options;

    options.setVolumeId( c->readEntry( "volume_id", options.volumeId() ) );
    options.setAlbumId( c->readEntry( "album_id", options.albumId() ) );
    options.setVolumeSetId( c->readEntry( "volume_set_id", options.volumeSetId() ) );
    options.setPreparer( c->readEntry( "preparer", options.preparer() ) );
    options.setPublisher( c->readEntry( "publisher", options.publisher() ) );
    options.setVolumeCount( c->readNumEntry( "volume_count",  options.volumeCount() ) );
    options.setVolumeNumber( c->readNumEntry( "volume_number", options.volumeNumber() ) );
    options.setAutoDetect( c->readBoolEntry( "autodetect", options.AutoDetect() ) );
    options.setCdiSupport( c->readBoolEntry( "cdi_support", options.CdiSupport() ) );
    options.setNonCompliantMode( c->readBoolEntry( "broken_svcd_mode", options.NonCompliantMode() ) );
    options.setSector2336( c->readBoolEntry( "2336_sectors", options.Sector2336() ) );
    options.setUpdateScanOffsets( c->readBoolEntry( "UpdateScanOffsets", options.UpdateScanOffsets() ) );
    options.setRelaxedAps( c->readBoolEntry( "RelaxedAps", options.RelaxedAps() ) );
    options.setPbcEnabled( c->readBoolEntry( "PbcEnabled", options.PbcEnabled() ) );
    options.setSegmentFolder( c->readBoolEntry( "SegmentFolder", options.SegmentFolder() ) );
    options.setRestriction( c->readNumEntry( "Restriction", options.Restriction() ) );
    options.setPreGapLeadout( c->readNumEntry( "PreGapLeadout", options.PreGapLeadout() ) );
    options.setPreGapTrack( c->readNumEntry( "PreGapTrack", options.PreGapTrack() ) );
    options.setFrontMarginTrack( c->readNumEntry( "FrontMarginTrack", options.FrontMarginTrack() ) );
    options.setRearMarginTrack( c->readNumEntry( "RearMarginTrack", options.RearMarginTrack() ) );
    options.setUseGaps( c->readBoolEntry( "UseGaps", options.UseGaps() ) );

    return options;
}

K3bVcdOptions K3bVcdOptions::defaults()
{
    // let the constructor create defaults
    return K3bVcdOptions();
}
