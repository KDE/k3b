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


#include "k3bimagereaderbase.h"

#include <k3bglobals.h>

#include <klocale.h>
#include <kio/global.h>


K3bImageReaderBase::K3bImageReaderBase()
{
}


K3bImageReaderBase::~K3bImageReaderBase()
{
}


void K3bImageReaderBase::close()
{
}


QString K3bImageReaderBase::metaInformation() const
{
  //
  // We simply create a little document containing the size of the image
  // and the tocfile if it's not empty
  //
  QString s( "<p>" + i18n("Image size") + ": <i>" + KIO::convertSize( K3b::filesize( imageFileName() ) ) + "</i>" );

  if( !tocFile().isEmpty() )
    s.append( QString( "<p>%1: <i>%2</i><br>"
		       "%3: <i>%4</i>" )
	      .arg( i18n("Image file") )
	      .arg( imageFileName().section( '/', -1 ) )
	      .arg( i18n("Toc file") )
	      .arg( tocFile().section( '/', -1 ) ) );
  
  return s;
}


K3bImageSource* K3bImageReaderBase::createImageSource( K3bJobHandler*, QObject* ) const
{
  return 0;
}
