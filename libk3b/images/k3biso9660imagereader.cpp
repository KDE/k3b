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


#include "k3biso9660imagereader.h"
#include "k3bplainimagesource.h"

#include <k3biso9660.h>
#include <k3bglobals.h>

#include <klocale.h>

#include <kdebug.h>

K3bIso9660ImageReader::K3bIso9660ImageReader()
  : K3bImageReaderBase(),
    m_iso( 0 )
{
}


K3bIso9660ImageReader::K3bIso9660ImageReader( const QString& file )
  : K3bImageReaderBase(),
    m_iso( 0 )
{
  open( file );
}


K3bIso9660ImageReader::~K3bIso9660ImageReader()
{
  close();
}


bool K3bIso9660ImageReader::open( const QString& file )
{
  close();
  m_iso = new K3bIso9660( file );
  if( m_iso->open() ) {
    setImageFileName( file );
    return true;
  }
  else {
    close();
    return false;
  }
}


void K3bIso9660ImageReader::close()
{
  delete m_iso;
  m_iso = 0;
}


bool K3bIso9660ImageReader::isOpen() const
{
  return( m_iso != 0 );
}


QString K3bIso9660ImageReader::imageType() const
{
  return "ISO9660";
}


QString K3bIso9660ImageReader::imageTypeComment() const
{
  return i18n("ISO9660 Image");
}


int K3bIso9660ImageReader::mediaType() const
{
  //
  // an image with size < 703 is definitely a CD image.
  // size between 703 and 880 may also be written to DVD
  //
  int mb = K3b::filesize( imageFileName() )/1024/1024;
  if( mb <= 703 )
    return CD_IMAGE;
  else if( mb < 880 )
    return CD_IMAGE|UNSURE;
  else if( mb < 1000 )
    return DVD_IMAGE|UNSURE;
  else
    return DVD_IMAGE;
}


QString K3bIso9660ImageReader::metaInformation() const
{
  QString s = K3bImageReaderBase::metaInformation();
  s.append( createIso9660MetaInformation( iso9660() ) );
  return s;
}


QString K3bIso9660ImageReader::createIso9660MetaInformation( K3bIso9660* iso ) const
{
  if( iso ) {
    QString s( "<p><b>" + i18n("ISO9660 Filesystem Info") + "</b><p>" );

    s.append( i18n("System Id:") );
    s.append( QString( " <i>%1</i><br>" ).arg( iso->primaryDescriptor().systemId.isEmpty() 
					       ? QString("-")
					       : iso->primaryDescriptor().systemId ) );

    s.append( i18n("Volume Id:") );
    s.append( QString( " <i>%1</i><br>" ).arg( iso->primaryDescriptor().volumeId.isEmpty() 
					       ? QString("-")
					       : iso->primaryDescriptor().volumeId ) );

    s.append( i18n("Volume Set Id:") );
    s.append( QString( " <i>%1</i><br>" ).arg( iso->primaryDescriptor().volumeSetId.isEmpty()
					       ? QString("-")
					       : iso->primaryDescriptor().volumeSetId ) );

    s.append( i18n("Publisher Id:") );
    s.append( QString( " <i>%1</i><br>" ).arg( iso->primaryDescriptor().publisherId.isEmpty()
					       ? QString("-") 
					       : iso->primaryDescriptor().publisherId ) );

    s.append( i18n("Preparer Id:") );
    s.append( QString( " <i>%1</i><br>" ).arg( iso->primaryDescriptor().preparerId.isEmpty()
					       ? QString("-") 
					       : iso->primaryDescriptor().preparerId ) );

    s.append( i18n("Application Id:") );
    s.append( QString( " <i>%1</i><br>" ).arg( iso->primaryDescriptor().applicationId.isEmpty()
					       ? QString("-") 
					       : iso->primaryDescriptor().applicationId ) );
    return s;
  }
  else
    return QString::null;
}


K3bImageSource* K3bIso9660ImageReader::createImageSource( K3bJobHandler* hdl, QObject* parent ) const
{
  return new K3bPlainImageSource( imageFileName(), hdl, parent );
}
