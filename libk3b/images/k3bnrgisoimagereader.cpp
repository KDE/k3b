/* 
 *
 * $Id: k3bnrgfilereader.cpp 412090 2005-05-10 18:28:07Z trueg $
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

#include "k3bnrgisoimagereader.h"
#include "k3bplainimagesource.h"

#include <k3biso9660.h>

#include <klocale.h>



K3bNrgIsoImageReader::K3bNrgIsoImageReader()
  : K3bIso9660ImageReader(),
    m_iso( 0 )
{
}


K3bNrgIsoImageReader::K3bNrgIsoImageReader( const QString& filename )
  : K3bIso9660ImageReader(),
    m_iso( 0 )
{
  open( filename );
}


K3bNrgIsoImageReader::~K3bNrgIsoImageReader()
{
}


bool K3bNrgIsoImageReader::open( const QString& file )
{
  close();

  m_file.setName( file );
  if( m_file.open( IO_ReadOnly ) ) {
    // the NRG header is 300KB in length
    if( m_file.at( 300*1024 ) ) {
      m_iso = new K3bIso9660( m_file.handle() );
      if( m_iso->open() ) {
	setImageFileName( file );
	return true;
      }
    }
  }

  close();
  return false;
}


void K3bNrgIsoImageReader::close()
{
  delete m_iso;
  m_iso = 0;
  m_file.close();
}


bool K3bNrgIsoImageReader::isOpen() const
{
  return( m_iso != 0 );
}


QString K3bNrgIsoImageReader::imageType() const
{
  return "NRG_ISO9660";
}


QString K3bNrgIsoImageReader::imageTypeComment() const
{
  return i18n("Nero ISO9660 Image");
}


QString K3bNrgIsoImageReader::metaInformation() const
{
  QString s = K3bImageReaderBase::metaInformation();
  s.append( createIso9660MetaInformation( iso9660() ) );
  return s;
}


K3bImageSource* K3bNrgIsoImageReader::createImageSource( K3bJobHandler* hdl, QObject* parent ) const
{
  K3bPlainImageSource* s = new K3bPlainImageSource( imageFileName(), hdl, parent );
  s->skip( 300*1024 );
  return s;
}
