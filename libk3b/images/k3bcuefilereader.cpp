/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bcuefilereader.h"
#include "k3baudiocueimagesource.h"
#include <k3bcdtext.h>
#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bmsf.h>

#include <klocale.h>

K3bCueFileReader::K3bCueFileReader()
  : K3bImageReaderBase(),
    m_bOpen(false)
{
}


K3bCueFileReader::K3bCueFileReader( const QString& file )
  : K3bImageReaderBase()
{
  open( file );
}


K3bCueFileReader::~K3bCueFileReader()
{
}


bool K3bCueFileReader::open( const QString& file )
{
  // FIXME: make sure the filenames all fit:
  //        rename both to <nameincuefile>.bin and <nameincuefile>.cue using symlinks
  //        to work around the differences in cdrdao and cdrecord. the latter uses the
  //        filename from the cue while the first one just replaces the ening of the cue 
  //        file.

  // FIXME: setImageFile & setTocFile
  m_parser.openFile( file );
  m_bOpen = m_parser.isValid();
  return isOpen();
}


void K3bCueFileReader::close()
{
  // since the reader does not keep any structures open we use a fake open
  // which just tells if the parsing succeeded
  m_bOpen = false; 
}


bool K3bCueFileReader::isOpen() const
{
  return m_bOpen;
}


QString K3bCueFileReader::imageType() const
{
  if( toc().contentType() == K3bDevice::AUDIO )
    return "audiocue";
  else
    return "cuebin";
}


QString K3bCueFileReader::imageTypeComment() const
{
  if( toc().contentType() == K3bDevice::AUDIO )
    return i18n("Audio Cue Image");
  else
    return i18n("Cue/Bin Image");
}


bool K3bCueFileReader::needsSpecialHandling() const
{
  return ( toc().contentType() != K3bDevice::AUDIO );
}


QString K3bCueFileReader::metaInformation() const
{
  if( toc().contentType() == K3bDevice::AUDIO ) {
    QString s( "<p><b>" + i18n("Table of contents") + "</b><br>" );
    s.append( "<i>" + i18n("%n track", "%n tracks", toc().count() ) + " - "
	      + toc().length().toString() + " " + i18n("min") + "</i>" );
    s.append( "<p>" );

    unsigned int i = 1;
    for( K3bDevice::Toc::const_iterator it = toc().begin();
	 it != toc().end(); ++it ) {
      
      // create track info
      s.append( i18n("Track") + " " + QString::number(i).rightJustify( 2, '0' ) );
      
      // append CD-Text
      if( !cdText().isEmpty() && !cdText()[i-1].isEmpty() )
	s.append( QString(" <i>(%2 - %3)</i>")
		  .arg(cdText()[i-1].performer())
		  .arg(cdText()[i-1].title()) );
      else
	s.append( " -" );

      s.append( "  " + ( i < toc().count() 
			 ? (*it).length().toString() 
			 : QString("??:??:??") ) );
      s.append( "<br>" );
      
      ++i;
    }

    return s;
  }
  else
    return K3bImageReaderBase::metaInformation();
}


const K3bDevice::Toc& K3bCueFileReader::toc() const
{
  return m_parser.toc();
}


const K3bDevice::CdText& K3bCueFileReader::cdText() const
{
  return m_parser.cdText();
}


K3bImageSource* K3bCueFileReader::createImageSource( K3bJobHandler* hdl, QObject* parent ) const
{
  kdDebug() << k_funcinfo << endl;

  if( toc().contentType() == K3bDevice::AUDIO )
    return new K3bAudioCueImageSource( m_parser.filename(), hdl, parent );
  else
    return 0;
}
