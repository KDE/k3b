/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bcuefileparser.h"

#include <qfile.h>
#include <qtextstream.h>

#include <kdebug.h>


K3bCueFileParser::K3bCueFileParser( const QString& filename )
  : K3bImageFileReader()
{
  openFile( filename );
}


K3bCueFileParser::~K3bCueFileParser()
{
}


void K3bCueFileParser::readFile()
{
  QFile f( filename() );
  if( f.open( IO_ReadOnly ) ) {
    //
    // for now we just search for a "FILE" statement and the stated file
    // FIXME: do some real parsing
    //
    QTextStream s( &f );
    QString line = s.readLine();
    while( !line.isNull() ) {
      if( line.startsWith( "FILE" ) ) {
	QString dataFile = line.mid(5);
	// strip "
	// FIXME: Does the cue syntax allow " or '??
	if( dataFile[0] == '"' || dataFile[0] == '\'' ) {
	  dataFile = dataFile.mid(1);
	  dataFile.truncate(dataFile.length()-1);
	}

	// find data file
	if( dataFile[0] == '/' ) {
	  setImageFilename( dataFile );
	}
	else {
	  // relative path
	  setImageFilename( filename().mid( 0, filename().findRev('/') + 1 ) + dataFile );
	}

	setValid( QFile::exists( imageFilename() ) );
      }

      line = s.readLine();
    }
  }
  else {
    kdDebug() << "(K3bCueFileParser) could not open file " << filename() << endl;
  }
}
