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
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qregexp.h>

#include <kdebug.h>


K3bCueFileParser::K3bCueFileParser( const QString& filename )
  : K3bImageFileReader()
{
  openFile( filename );
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

	kdDebug() << "(K3bCueFileParser) found FILE statement: " << line << endl;

	line.stripWhiteSpace();

	// extract the filename from the FILE statement
	// cdrecord 2.01a19 does only support binary, motorola, au, and wave
	QRegExp rx( "(FILE)(\\s*)([\"\']?[^\"\']*[\"\']?)(\\s*)(BINARY|MOTOROLA|AU|WAVE)" );
	int pos = rx.search( line );
	if( pos > -1 ) {
	  QString dataFile = rx.cap(3);
	  if( dataFile[0] == '\"' || dataFile[0] == '\'' ) {
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

	  //
	  // CDRDAO does not use this image filename but replaces the extension from the cue file
	  // with "bin" to get the image filename, we should take this into account
	  //
	  kdDebug() << "(K3bCueFileParser) trying bin file: " << dataFile << endl;
	  if( QFileInfo( imageFilename() ).isFile() ) {
	    setValid( true );
	    m_imageFilenameInCue = true;
	  }
	  else {
	    setImageFilename( filename().mid( filename().length() - 3 ) + "bin" );
	    setValid( QFileInfo( imageFilename() ).isFile() );
	    m_imageFilenameInCue = false;
	  }
	}
      }

      line = s.readLine();
    }
  }
  else {
    kdDebug() << "(K3bCueFileParser) could not open file " << filename() << endl;
  }
}
