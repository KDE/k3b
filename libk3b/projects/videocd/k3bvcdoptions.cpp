/*

    SPDX-FileCopyrightText: 2003-2004 Christian Kvasny <chris@k3b.org>
    SPDX-FileCopyrightText: 2008-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bvcdoptions.h"
#include "k3bversion.h"
#include "k3bcore.h"
#include "k3b_i18n.h"

#include <KConfig>
#include <QFile>
#include <QString>
#include <QStandardPaths>

K3b::VcdOptions::VcdOptions()
    : m_restriction( 0 ),
      m_segment( 0 ),
      m_sequence( 0 ),
      m_pbcenabled(false),
      m_pbcnumkeysenabled(false),
      m_volumeID( "VIDEOCD" ),
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
      m_mpegversion( VCD20 ),
      m_volumeCount( 1 ),
      m_volumeNumber( 1 ),
      m_autodetect( true ),
      m_cdisupport( false ),
      m_brokensvcdmode( false ),
      m_VCD30interpretation( false ),
      m_sector2336( false ),
      m_updatescanoffsets( false ),
      m_relaxedaps( false ),
      m_segmentfolder( true ),
      m_usegaps( false )
{}

bool K3b::VcdOptions::checkCdiFiles()
{
    m_cdisize = 0;
    if ( !QFile::exists( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "k3b/cdi/cdi_imag.rtf" ) ) )
        return false;
    if ( !QFile::exists( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "k3b/cdi/cdi_text.fnt" ) ) )
        return false;
    if ( !QFile::exists( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "k3b/cdi/cdi_vcd.app" ) ) )
        return false;
    if ( !QFile::exists( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "k3b/cdi/cdi_vcd.cfg" ) ) )
        return false;

    m_cdisize += QFile( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "k3b/cdi/cdi_imag.rtf" ) ).size();
    m_cdisize += QFile( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "k3b/cdi/cdi_text.fnt" ) ).size();
    m_cdisize += QFile( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "k3b/cdi/cdi_vcd.app" ) ).size();
    m_cdisize += QFile( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "k3b/cdi/cdi_vcd.cfg" ) ).size();

    return true;
}

void K3b::VcdOptions::save( KConfigGroup c )
{
    c.writeEntry( "volume_id", m_volumeID );
    c.writeEntry( "album_id", m_albumID );
    c.writeEntry( "volume_set_id", m_volumeSetId );
    c.writeEntry( "preparer", m_preparer );
    c.writeEntry( "publisher", m_publisher );
    c.writeEntry( "volume_count", m_volumeCount );
    c.writeEntry( "volume_number", m_volumeNumber );
    c.writeEntry( "autodetect", m_autodetect );
    c.writeEntry( "cdi_support", m_cdisupport );
    c.writeEntry( "broken_svcd_mode", m_brokensvcdmode );
    c.writeEntry( "VCD30interpretation", m_VCD30interpretation );
    c.writeEntry( "2336_sectors", m_sector2336 );
    c.writeEntry( "UpdateScanOffsets", m_updatescanoffsets );
    c.writeEntry( "RelaxedAps", m_relaxedaps );
    c.writeEntry( "PbcEnabled", m_pbcenabled );
    c.writeEntry( "SegmentFolder", m_segmentfolder );
    c.writeEntry( "Restriction", m_restriction );
    c.writeEntry( "PreGapLeadout", m_pregapleadout );
    c.writeEntry( "PreGapTrack", m_pregaptrack );
    c.writeEntry( "FrontMarginTrack", m_frontmargintrack );
    c.writeEntry( "RearMarginTrack", m_rearmargintrack );
    c.writeEntry( "UseGaps", m_usegaps );
    c.writeEntry( "MPEG Version", ( int )m_mpegversion );
}


K3b::VcdOptions K3b::VcdOptions::load( const KConfigGroup& c )
{
    K3b::VcdOptions options;

    options.setVolumeId( c.readEntry( "volume_id", options.volumeId() ) );
    options.setAlbumId( c.readEntry( "album_id", options.albumId() ) );
    options.setVolumeSetId( c.readEntry( "volume_set_id", options.volumeSetId() ) );
    options.setPreparer( c.readEntry( "preparer", options.preparer() ) );
    options.setPublisher( c.readEntry( "publisher", options.publisher() ) );
    options.setVolumeCount( c.readEntry( "volume_count", options.volumeCount() ) );
    options.setVolumeNumber( c.readEntry( "volume_number", options.volumeNumber() ) );
    options.setAutoDetect( c.readEntry( "autodetect", options.AutoDetect() ) );
    options.setCdiSupport( c.readEntry( "cdi_support", options.CdiSupport() ) );
    options.setNonCompliantMode( c.readEntry( "broken_svcd_mode", options.NonCompliantMode() ) );
    options.setVCD30interpretation( c.readEntry( "VCD30interpretation", options.VCD30interpretation() ) );
    options.setSector2336( c.readEntry( "2336_sectors", options.Sector2336() ) );
    options.setUpdateScanOffsets( c.readEntry( "UpdateScanOffsets", options.UpdateScanOffsets() ) );
    options.setRelaxedAps( c.readEntry( "RelaxedAps", options.RelaxedAps() ) );
    options.setPbcEnabled( c.readEntry( "PbcEnabled", options.PbcEnabled() ) );
    options.setSegmentFolder( c.readEntry( "SegmentFolder", options.SegmentFolder() ) );
    options.setRestriction( c.readEntry( "Restriction", options.Restriction() ) );
    options.setPreGapLeadout( c.readEntry( "PreGapLeadout", options.PreGapLeadout() ) );
    options.setPreGapTrack( c.readEntry( "PreGapTrack", options.PreGapTrack() ) );
    options.setFrontMarginTrack( c.readEntry( "FrontMarginTrack", options.FrontMarginTrack() ) );
    options.setRearMarginTrack( c.readEntry( "RearMarginTrack", options.RearMarginTrack() ) );
    options.setUseGaps( c.readEntry( "UseGaps", options.UseGaps() ) );
    options.setMpegVersion( ( MPEGVersion )c.readEntry( "MPEG Version", ( int )options.mpegVersion() ) );
    return options;
}

K3b::VcdOptions K3b::VcdOptions::defaults()
{
    // let the constructor create defaults
    return K3b::VcdOptions();
}
