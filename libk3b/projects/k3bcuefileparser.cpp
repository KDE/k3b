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

#include "k3bcuefileparser.h"

#include <k3bmsf.h>
#include <k3bglobals.h>
#include <k3btrack.h>
#include <k3bcdtext.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qdir.h>

#include <kdebug.h>


// avoid usage of QTextStream since K3b often
// tries to open big files (iso images) in a 
// cue file parser to test it.
static QString readLine( QFile* f )
{
  QString s;
  Q_LONG r = f->readLine( s, 1024 );
  if( r >= 0 ) {
    // remove the trailing newline
    return s.stripWhiteSpace();
  }
  else {
    // end of file or error
    return QString::null;
  }
}


// TODO: add method: usableByCdrecordDirectly()
// TODO: add Toc with sector sizes

class K3bCueFileParser::Private
{
public:
  bool inFile;
  bool inTrack;
  int trackType;
  int trackMode;
  bool rawData;
  bool haveIndex1;
  K3b::Msf currentDataPos;
  K3b::Msf index0;

  K3bDevice::Toc toc;
  int currentParsedTrack;

  K3bDevice::CdText cdText;
};



K3bCueFileParser::K3bCueFileParser( const QString& filename )
  : K3bImageFileReader()
{
  d = new Private;
  openFile( filename );
}


K3bCueFileParser::~K3bCueFileParser()
{
  delete d;
}


void K3bCueFileParser::readFile()
{
  setValid(true);

  d->inFile = d->inTrack = d->haveIndex1 = false;
  d->trackMode = K3bDevice::Track::UNKNOWN;
  d->toc.clear();
  d->cdText.clear();
  d->currentParsedTrack = 0;

  QFile f( filename() );
  if( f.open( IO_ReadOnly ) ) {
    QString line = readLine( &f );
    while( !line.isNull() ) {
      
      if( !parseLine(line) ) {
	setValid(false);
	break;
      }

      line = readLine( &f );
    }

    if( isValid() ) {
      // save last parsed track for which we do not have the proper length :(
      if( d->currentParsedTrack > 0 ) {
	d->toc.append( K3bDevice::Track( d->currentDataPos, 
					   d->currentDataPos,
					   d->trackType,
					   d->trackMode ) );
      }
      
      // debug the toc
      kdDebug() << "(K3bCueFileParser) successfully parsed cue file." << endl
		<< "------------------------------------------------" << endl;
      for( unsigned int i = 0; i < d->toc.count(); ++i ) {
	K3bDevice::Track& track = d->toc[i];
	kdDebug() << "Track " << (i+1) 
		  << " (" << ( track.type() == K3bDevice::Track::AUDIO ? "audio" : "data" ) << ") "
		  << track.firstSector().toString() << " - " << track.lastSector().toString() << endl;
      }
      
      kdDebug() << "------------------------------------------------" << endl;
    }
  }
  else {
    kdDebug() << "(K3bCueFileParser) could not open file " << filename() << endl;
    setValid(false);
  }
}


bool K3bCueFileParser::parseLine( QString& line )
{
  // use cap(1) for the filename
  static QRegExp fileRx( "FILE\\s\"?([^\"]*)\"?\\s[^\"\\s]*" );

  // use cap(1) for the flags
  static QRegExp flagsRx( "FLAGS(\\s(DCP|4CH|PRE|SCMS)){1,4}" );

  // use cap(1) for the tracknumber and cap(2) for the datatype
  static QRegExp trackRx( "TRACK\\s(\\d{1,2})\\s(AUDIO|CDG|MODE1/2048|MODE1/2352|MODE2/2336|MODE2/2352|CDI/2336|CDI/2352)" );

  // use cap(1) for the index number, cap(3) for the minutes, cap(4) for the seconds, cap(5) for the frames,
  // and cap(2) for the MSF value string
  static QRegExp indexRx( "INDEX\\s(\\d{1,2})\\s((\\d+):([0-5]\\d):((?:[0-6]\\d)|(?:7[0-4])))" );

  // use cap(1) for the MCN
  static QRegExp catalogRx( "CATALOG\\s(\\w{13,13})" );

  // use cap(1) for the ISRC
  static QRegExp isrcRx( "ISRC\\s(\\w{5,5}\\d{7,7})" );

  static QString cdTextRxStr = "\"?([^\"]{0,80})\"?";

  // use cap(1) for the string
  static QRegExp titleRx( "TITLE\\s" + cdTextRxStr );
  static QRegExp performerRx( "PERFORMER\\s" + cdTextRxStr );
  static QRegExp songwriterRx( "SONGWRITER\\s" + cdTextRxStr );


  // simplify all white spaces except those in filenames and CD-TEXT
  simplifyWhiteSpace( line );

  // skip comments and empty lines
  if( line.startsWith("REM") || line.startsWith("#") || line.isEmpty() )
    return true;


  //
  // FILE
  //
  if( fileRx.exactMatch( line ) ) {

    setValid( findImageFileName( fileRx.cap(1) ) );
    
    if( d->inFile ) {
      kdDebug() << "(K3bCueFileParser) only one FILE statement allowed." << endl;
      return false;
    }
    d->inFile = true;
    d->inTrack = false;
    d->haveIndex1 = false;
    return true;
  }


  //
  // TRACK
  //
  else if( trackRx.exactMatch( line ) ) {
    if( !d->inFile ) {
      kdDebug() << "(K3bCueFileParser) TRACK statement before FILE." << endl;
      return false;
    }

    // check if we had index1 for the last track
    if( d->inTrack && !d->haveIndex1 ) {
      kdDebug() << "(K3bCueFileParser) TRACK without INDEX 1." << endl;
      return false;
    }

    // save last track
    // TODO: use d->rawData in some way
    if( d->currentParsedTrack > 0 ) {
      d->toc.append( K3bDevice::Track( d->currentDataPos, 
				       d->currentDataPos,
				       d->trackType,
				       d->trackMode ) );
    }

    d->currentParsedTrack++;

    d->cdText.resize( d->currentParsedTrack );

    // parse the tracktype
    if( trackRx.cap(2) == "AUDIO" ) {
      d->trackType = K3bDevice::Track::AUDIO;
      d->trackMode = K3bDevice::Track::UNKNOWN;
    }
    else {
      d->trackType = K3bDevice::Track::DATA;
      if( trackRx.cap(2).startsWith("MODE1") ) {
	d->trackMode = K3bDevice::Track::MODE1;
	d->rawData = (trackRx.cap(2) == "MODE1/2352");
      }
      else if( trackRx.cap(2).startsWith("MODE2") ) {
	d->trackMode = K3bDevice::Track::MODE2;
	d->rawData = (trackRx.cap(2) == "MODE2/2352");
      }
      else {
	kdDebug() << "(K3bCueFileParser) unsupported track type: " << trackRx.cap(2) << endl;
	return false;
      }
    }

    d->haveIndex1 = false;
    d->inTrack = true;
    d->index0 = 0;

    return true;
  }


  //
  // FLAGS
  //
  else if( flagsRx.exactMatch( line ) ) {
    if( !d->inTrack ) {
      kdDebug() << "(K3bCueFileParser) FLAGS statement without TRACK." << endl;
      return false;
    }

    // TODO: save the flags
    return true;
  }


  //
  // INDEX
  //
  else if( indexRx.exactMatch( line ) ) {
    if( !d->inTrack ) {
      kdDebug() << "(K3bCueFileParser) INDEX statement without TRACK." << endl;
      return false;
    }

    unsigned int indexNumber = indexRx.cap(1).toInt();

    K3b::Msf indexStart = K3b::Msf::fromString( indexRx.cap(2) );

    if( indexNumber == 0 ) {
      d->index0 = indexStart;

      if( d->currentParsedTrack < 2 && indexStart > 0 ) {
	kdDebug() << "(K3bCueFileParser) first track is not allowed to have a pregap > 0." << endl;
	return false;
      }
    }
    else if( indexNumber == 1 ) {
      d->haveIndex1 = true;
      d->currentDataPos = indexStart;
      if( d->currentParsedTrack > 1 ) {
	d->toc[d->currentParsedTrack-2].setLastSector( indexStart-1 );
	if( d->index0 > 0 && d->index0 < indexStart ) {
	  d->toc[d->currentParsedTrack-2].setIndex0( d->index0 - d->toc[d->currentParsedTrack-2].firstSector() );
	}
      }
    }
    else {
      // TODO: add index > 0
    }

    return true;
  }


  //
  // CATALOG
  //
  if( catalogRx.exactMatch( line ) ) {
    // TODO: set the toc's mcn
    return true;
  }


  //
  // ISRC
  //
  if( isrcRx.exactMatch( line ) ) {
    if( d->inTrack ) {
      // TODO: set the track's ISRC
      return true;
    }
    else {
      kdDebug() << "(K3bCueFileParser) ISRC without TRACK." << endl;
      return false;
    }
  }


  //
  // CD-TEXT
  // TODO: create K3bDevice::TrackCdText entries
  //
  else if( titleRx.exactMatch( line ) ) {
    if( d->inTrack )
      d->cdText[d->currentParsedTrack-1].setTitle( titleRx.cap(1) );
    else
      d->cdText.setTitle( titleRx.cap(1) );
    return true;
  }

  else if( performerRx.exactMatch( line ) ) {
    if( d->inTrack )
      d->cdText[d->currentParsedTrack-1].setPerformer( performerRx.cap(1) );
    else
      d->cdText.setPerformer( performerRx.cap(1) );
    return true;
  }

  else if( songwriterRx.exactMatch( line ) ) {
    if( d->inTrack )
      d->cdText[d->currentParsedTrack-1].setSongwriter( songwriterRx.cap(1) );
    else
      d->cdText.setSongwriter( songwriterRx.cap(1) );
    return true;
  }

  else {
    kdDebug() << "(K3bCueFileParser) unknown Cue line: '" << line << "'" << endl;
    return false;
  }
}


void K3bCueFileParser::simplifyWhiteSpace( QString& s )
{
  s = s.stripWhiteSpace();

  unsigned int i = 0;
  bool insideQuote = false;
  while( i < s.length() ) {
    if( !insideQuote ) {
      if( s[i].isSpace() && s[i+1].isSpace() )
	s.remove( i, 1 );
    }

    if( s[i] == '"' )
      insideQuote = !insideQuote;

    ++i;
  }
}


const K3bDevice::Toc& K3bCueFileParser::toc() const
{
  return d->toc;
}


const K3bDevice::CdText& K3bCueFileParser::cdText() const
{
  return d->cdText;
}


bool K3bCueFileParser::findImageFileName( const QString& dataFile )
{
  //
  // CDRDAO does not use this image filename but replaces the extension from the cue file
  // with "bin" to get the image filename, we should take this into account
  //

  m_imageFilenameInCue = true;

  // first try filename as a hole (absolut)
  if( QFile::exists( dataFile ) ) {
    setImageFilename( QFileInfo(dataFile).absFilePath() );
    return true;
  }

  // try the filename in the cue's directory
  if( QFileInfo( K3b::parentDir(filename()) + dataFile.section( '/', -1 ) ).isFile() ) {
    setImageFilename( K3b::parentDir(filename()) + dataFile.section( '/', -1 ) );
    kdDebug() << "(K3bCueFileParser) found image file: " << imageFilename() << endl;
    return true;
  }

  // try the filename ignoring case
  if( QFileInfo( K3b::parentDir(filename()) + dataFile.section( '/', -1 ).lower() ).isFile() ) {
    setImageFilename( K3b::parentDir(filename()) + dataFile.section( '/', -1 ).lower() );
    kdDebug() << "(K3bCueFileParser) found image file: " << imageFilename() << endl;
    return true;
  }

  m_imageFilenameInCue = false;

  // try removing the ending from the cue file (image.bin.cue and image.bin)
  if( QFileInfo( filename().left( filename().length()-4 ) ).isFile() ) {
    setImageFilename( filename().left( filename().length()-4 ) );
    kdDebug() << "(K3bCueFileParser) found image file: " << imageFilename() << endl;
    return true;
  }

  //
  // we did not find the image specified in the cue.
  // Search for another one having the same filename as the cue but a different extension
  //

  QDir parentDir( K3b::parentDir(filename()) );
  QString filenamePrefix = filename().section( '/', -1 );
  filenamePrefix.truncate( filenamePrefix.length() - 3 ); // remove cue extension
  kdDebug() << "(K3bCueFileParser) checking folder " << parentDir.path() << " for files: " << filenamePrefix << "*" << endl;

  //
  // we cannot use the nameFilter in QDir because of the spaces that may occure in filenames
  //
  QStringList possibleImageFiles = parentDir.entryList( QDir::Files );
  int cnt = 0;
  for( QStringList::const_iterator it = possibleImageFiles.constBegin(); it != possibleImageFiles.constEnd(); ++it ) {
    if( (*it).lower() == dataFile.section( '/', -1 ).lower() ||
	(*it).startsWith( filenamePrefix ) && !(*it).endsWith( "cue" ) ) {
      ++cnt;
      setImageFilename( K3b::parentDir(filename()) + *it );
    }
  }

  //
  // we only do this if there is one unique file which fits the requirements. 
  // Otherwise we cannot be certain to have the right file.
  //
  return ( cnt == 1 && QFileInfo( imageFilename() ).isFile() );
}
