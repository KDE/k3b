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

#include "k3bnrgfilereader.h"

#include <k3biso9660.h>

#include <qfile.h>

#include <kdebug.h>



K3bNrgFileReader::K3bNrgFileReader( const QString& filename )
  : K3bImageFileReader()
{
  openFile( filename );
}


K3bNrgFileReader::~K3bNrgFileReader()
{
}


void K3bNrgFileReader::readFile()
{
  setValid(false);

  QFile f( filename() );
  if( f.open( IO_ReadOnly ) ) {
    // the NRG header is 300KB in length
    if( f.at( 300*1024 ) ) {
      K3bIso9660 isoF( f.handle() );
      setValid( isoF.open() );
    }
    else {
      kdDebug() << "(K3bNrgFileReader) could not seek to 300KB in " << filename() << endl;
    }
  }
  else {
    kdDebug() << "(K3bNrgFileReader) could not open file " << filename() << endl;
  }
}
