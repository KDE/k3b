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

#include "k3bisooptions.h"
#include <k3bcore.h>
#include <k3bversion.h>
#include <k3bglobals.h>

#include <kconfig.h>
#include <klocale.h>
#include <qstring.h>


K3bIsoOptions::K3bIsoOptions()
  : m_volumeID( "K3b data project" ),
    m_applicationID( QString("K3B THE CD KREATOR (C) 1998-2005 SEBASTIAN TRUEG AND THE K3B TEAM") ),
    m_systemId( K3b::systemName().upper() ),
    m_inputCharset( "iso8859-1" ),
    m_whiteSpaceTreatmentReplaceString( "_" )
{
  m_bForceInputCharset = false;

  m_createRockRidge = true;
  m_createJoliet = true;
  m_createUdf = false;
  m_ISOallowLowercase = false;
  m_ISOallowPeriodAtBegin = false;
  m_ISOallow31charFilenames = true;
  m_ISOomitVersionNumbers = false;
  m_ISOomitTrailingPeriod = false;
  m_ISOmaxFilenameLength = false;
  m_ISOrelaxedFilenames = false;
  m_ISOnoIsoTranslate = false;
  m_ISOallowMultiDot = false;
  m_ISOuntranslatedFilenames = false;
  m_followSymbolicLinks = false;
  m_createTRANS_TBL = false;
  m_hideTRANS_TBL = false;
  m_jolietLong = false;

  m_doNotCacheInodes = false;

  m_isoLevel = 2;

  m_discardSymlinks = false;
  m_discardBrokenSymlinks = false;

  m_preserveFilePermissions = false;

  m_whiteSpaceTreatment = noChange;

  m_volumeSetSize = 1;
  m_volumeSetNumber = 1;
}


void K3bIsoOptions::save( KConfigBase* c )
{
  c->writeEntry( "volume id", m_volumeID );
  c->writeEntry( "application id", m_applicationID );
  c->writeEntry( "preparer", m_preparer );
  c->writeEntry( "publisher", m_publisher );
  c->writeEntry( "system id", m_systemId );
  c->writeEntry( "volume set id", m_volumeSetId );
  c->writeEntry( "volume set size", m_volumeSetSize );
  c->writeEntry( "volume set number", m_volumeSetNumber );

  c->writeEntry( "rock_ridge", m_createRockRidge );
  c->writeEntry( "joliet", m_createJoliet );
  c->writeEntry( "udf", m_createUdf );

  // save iso-level
  c->writeEntry( "iso_level", m_isoLevel );

  c->writeEntry( "create TRANS_TBL", m_createTRANS_TBL );
  c->writeEntry( "hide TRANS_TBL", m_hideTRANS_TBL );
  c->writeEntry( "untranslated filenames", m_ISOuntranslatedFilenames );
  c->writeEntry( "allow 31 character filenames", m_ISOallow31charFilenames );
  c->writeEntry( "max ISO filenames", m_ISOmaxFilenameLength );
  c->writeEntry( "allow beginning period", m_ISOallowPeriodAtBegin );
  c->writeEntry( "relaxed filenames", m_ISOrelaxedFilenames );
  c->writeEntry( "omit version numbers", m_ISOomitVersionNumbers );
  c->writeEntry( "omit trailing period", m_ISOomitTrailingPeriod );
  c->writeEntry( "no iSO translation", m_ISOnoIsoTranslate );
  c->writeEntry( "allow multible dots", m_ISOallowMultiDot );
  c->writeEntry( "allow lowercase filenames", m_ISOallowLowercase );
  //  c->writeEntry( "follow symbolic links", m_followSymbolicLinks );

  c->writeEntry( "joliet long", m_jolietLong );

  c->writeEntry( "force input charset", m_bForceInputCharset );
  c->writeEntry( "input charset", m_inputCharset );

  c->writeEntry( "do not cache inodes", m_doNotCacheInodes );

  // save whitespace-treatment
  switch( m_whiteSpaceTreatment ) {
  case strip:
    c->writeEntry( "white_space_treatment", "strip" );
    break;
  case extended:
    c->writeEntry( "white_space_treatment", "extended" );
    break;
  case replace:
    c->writeEntry( "white_space_treatment", "replace" );
    break;
  default:
    c->writeEntry( "white_space_treatment", "noChange" );
  }

  c->writeEntry( "whitespace replace string", m_whiteSpaceTreatmentReplaceString );

  c->writeEntry( "discard symlinks", discardSymlinks() );
  c->writeEntry( "discard broken symlinks", discardBrokenSymlinks() );

  c->writeEntry( "preserve file permissions", m_preserveFilePermissions );
}


K3bIsoOptions K3bIsoOptions::load( KConfigBase* c )
{
  K3bIsoOptions options;

  options.setVolumeID( c->readEntry( "volume id", options.volumeID() ) );
  options.setApplicationID( c->readEntry( "application id", options.applicationID() ) );
  options.setPreparer( c->readEntry( "preparer", options.preparer() ) );
  options.setPublisher( c->readEntry( "publisher", options.publisher() ) );
  options.setSystemId( c->readEntry( "system id", options.systemId() ) );
  options.setVolumeSetId( c->readEntry( "volume set id", options.volumeSetId() ) );
  options.setVolumeSetSize( c->readNumEntry( "volume set size", options.volumeSetSize() ) );
  options.setVolumeSetNumber( c->readNumEntry( "volume set number", options.volumeSetNumber() ) );

  options.setForceInputCharset( c->readBoolEntry( "force input charset", options.forceInputCharset() ) );
  if( options.forceInputCharset() )
    options.setInputCharset( c->readEntry( "input charset", options.inputCharset() ) );

  options.setCreateRockRidge( c->readBoolEntry( "rock_ridge", options.createRockRidge() ) );
  options.setCreateJoliet( c->readBoolEntry( "joliet", options.createJoliet() ) );
  options.setCreateUdf( c->readBoolEntry( "udf", options.createUdf() ) );

  options.setISOLevel( c->readNumEntry( "iso_level", options.ISOLevel() ) );

  options.setCreateTRANS_TBL( c->readBoolEntry( "create TRANS_TBL", options.createTRANS_TBL() ) );
  options.setHideTRANS_TBL( c->readBoolEntry( "hide TRANS_TBL", options.hideTRANS_TBL() ) );

  //
  // We need to use the memeber variables here instead of the access methods
  // which do not return the actual value of the member variables but the value
  // representing the use in mkisofs (i.e. ISOomitVersionNumbers is also enabled
  // if ISOmaxFilenameLength is enabled.
  //
  options.setISOuntranslatedFilenames( c->readBoolEntry( "untranslated filenames", options.m_ISOuntranslatedFilenames ) );
  options.setISOallow31charFilenames( c->readBoolEntry( "allow 31 character filenames", options.m_ISOallow31charFilenames ) );
  options.setISOmaxFilenameLength( c->readBoolEntry( "max ISO filenames", options.m_ISOmaxFilenameLength ) );
  options.setISOallowPeriodAtBegin( c->readBoolEntry( "allow beginning period", options.m_ISOallowPeriodAtBegin ) );
  options.setISOrelaxedFilenames( c->readBoolEntry( "relaxed filenames", options.m_ISOrelaxedFilenames ) );
  options.setISOomitVersionNumbers( c->readBoolEntry( "omit version numbers", options.m_ISOomitVersionNumbers ) );
  options.setISOnoIsoTranslate( c->readBoolEntry( "no iSO translation", options.m_ISOnoIsoTranslate ) );
  options.setISOallowMultiDot( c->readBoolEntry( "allow multible dots", options.m_ISOallowMultiDot ) );
  options.setISOallowLowercase( c->readBoolEntry( "allow lowercase filenames", options.m_ISOallowLowercase ) );
  options.setISOomitTrailingPeriod( c->readBoolEntry( "omit trailing period", options.m_ISOomitTrailingPeriod ) );

  //  options.setFollowSymbolicLinks( c->readBoolEntry( "follow symbolic links", options.m_followSymbolicLinks ) );

  options.setJolietLong( c->readBoolEntry( "joliet long", options.jolietLong() ) );

  options.setDoNotCacheInodes( c->readBoolEntry( "do not cache inodes", options.doNotCacheInodes() ) );

  QString w = c->readEntry( "white_space_treatment", "noChange" );
  if( w == "replace" )
    options.setWhiteSpaceTreatment( replace );
  else if( w == "strip" )
    options.setWhiteSpaceTreatment( strip );
  else if( w == "extended" )
    options.setWhiteSpaceTreatment( extended );
  else
    options.setWhiteSpaceTreatment( noChange );

  options.setWhiteSpaceTreatmentReplaceString( c->readEntry( "whitespace replace string", options.whiteSpaceTreatmentReplaceString() ) );

  options.setDiscardSymlinks( c->readBoolEntry("discard symlinks", options.discardSymlinks() ) );
  options.setDiscardBrokenSymlinks( c->readBoolEntry("discard broken symlinks", options.discardBrokenSymlinks() ) );

  options.setPreserveFilePermissions( c->readBoolEntry( "preserve file permissions", options.preserveFilePermissions() ) );

  return options;
}


K3bIsoOptions K3bIsoOptions::defaults()
{
  // let the constructor create defaults
  return K3bIsoOptions();
}


int K3bIsoOptions::maxIso9660FilenameLength() const
{
  int iso9660_namelen = LEN_ISONAME;
  if( ISOLevel() == 4 )
    iso9660_namelen = MAX_ISONAME_V2;
  if( ISOmaxFilenameLength() )
    iso9660_namelen = MAX_ISONAME_V1;
  if( createRockRidge() && (iso9660_namelen > MAX_ISONAME_V2_RR) )
    iso9660_namelen = MAX_ISONAME_V2_RR;
  return iso9660_namelen;
}
