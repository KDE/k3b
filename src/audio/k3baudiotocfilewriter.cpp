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

#include "k3baudiotocfilewriter.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"

#include <device/k3bmsf.h>

#include <qstring.h>
#include <qtextstream.h>
#include <qfile.h>

#include <kdebug.h>


bool K3bAudioTocfileWriter::writeAudioToc( K3bAudioDoc* doc, QTextStream& t )
{
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // from the cdrdao manpage:
  // ------------------------
  // If one of the CD-TEXT items TITLE, PERFORMER, SONGWRITER, COMPOSER, ARRANGER, ISRC is defined for at least on  track  or
  // in  the  global section it must be defined for all tracks and in the global section. If a DISC_ID item is defined in the
  // global section, an ISRC entry must be defined for each track.
  //
  // Question: does it make any difference to specify empty cd-text fields?
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


  K3b::Msf stdinDataLength(0);

  // create the album CD-TEXT entries if needed
  // ---------------------------------------------------------------------------
  if( doc->cdText() ) {
    t << "CD_TEXT {" << "\n";
    t << "  LANGUAGE_MAP { 0: EN }\n";
    t << "  LANGUAGE 0 {\n";
    t << "    TITLE " << "\"" << prepareForTocFile(doc->title()) << "\"" << "\n";
    t << "    PERFORMER " << "\"" << prepareForTocFile(doc->artist()) << "\"" << "\n";
    t << "    DISC_ID " << "\"" << prepareForTocFile(doc->disc_id()) << "\"" << "\n";
    t << "    UPC_EAN " << "\"" << prepareForTocFile(doc->upc_ean()) << "\"" << "\n";
    t << "\n";
    t << "    ARRANGER " << "\"" << prepareForTocFile(doc->arranger()) << "\"" << "\n";
    t << "    SONGWRITER " << "\"" << prepareForTocFile(doc->songwriter()) << "\"" << "\n";
    t << "    COMPOSER " << "\"" << prepareForTocFile(doc->composer()) << "\"" << "\n";
    t << "    MESSAGE " << "\"" << prepareForTocFile(doc->cdTextMessage()) << "\"" << "\n";
    t << "  }" << "\n";
    t << "}" << "\n\n";
  }
  // ---------------------------------------------------------------------------




  // ===========================================================================
  // the tracks
  // ===========================================================================

  K3bAudioTrack* track = doc->first();

  // if we need to hide the first song in the first tracks' pregap
  // we process the first two songs at once

  if( doc->hideFirstTrack() ) {
    K3bAudioTrack* hiddenTrack = track;
    track = doc->next();
    if( track == 0 ) {
      // we cannot hide a lonely track
      track = doc->first();
    }
    else {
      t << "TRACK AUDIO" << "\n";


      // track is the "real" first track so it's copy and preemp information is used
      if( track->copyProtection() )
        t << "NO COPY" << "\n";
      else
	t << "COPY" << "\n";
      
      if( track->preEmp() )
        t << "PRE_EMPHASIS" << "\n";
//       else
//         t << "NO PRE_EMPHASIS" << "\n";

      if( doc->cdText() ) {
	writeCdTextEntries( track, t );
      }

      // the "hidden" file will be used as pregap for the "first" track
      t << "FILE ";
      if( doc->onTheFly() ) {
	t << "\"-\" ";   // read from stdin
	t << stdinDataLength.toString();        // where does the track start in stdin
	t << " " << hiddenTrack->length().toString();   // here we need the perfect length !!!!!
	t << "\n";
	
	stdinDataLength += hiddenTrack->length();
      }
      else {
	t << "\"" << track->bufferFile() << "\"" << " 0" << "\n";
      }
      t << "START" << "\n"; // use the whole hidden file as pregap


      // now comes the "real" first track
      t << "FILE ";
      if( doc->onTheFly() ) {
	t << "\"-\" ";   // read from stdin
	t << stdinDataLength.toString();        // where does the track start in stdin
	t << " " << track->length().toString();   // here we need the perfect length !!!!!
	t << "\n";
	
	stdinDataLength += track->length();
      }
      else {
	t << "\"" << track->bufferFile() << "\"" << " 0" << "\n";
      }
      t << "\n";
    }

    track = doc->next();
  }


  // now iterate over the rest of the tracks
  
  for( ; track != 0; track = doc->next() ) {
    t << "TRACK AUDIO" << "\n";

    if( track->copyProtection() )
      t << "NO COPY" << "\n";
    else
      t << "COPY" << "\n";

    if( track->preEmp() )
      t << "PRE_EMPHASIS" << "\n";
//     else
//       t << "NO PRE_EMPHASIS" << "\n";

    if( doc->cdText() ) {
      writeCdTextEntries( track, t );
    }

    K3b::Msf p = track->pregap();
    if( track->index() == 0 ) {
      // cdrdao seems to always create a pregap of 150 frames for the first track
      // so specifying a pregap of x for the first track results in an overall pregap
      // of 150 + x
      // so we do not allow pregaps below that in the gui
      // and cut the value here
      p -= 150;
    }
    if( p > 0 )
      t << "PREGAP " << p.toString() << "\n";

    t << "FILE ";
    if( doc->onTheFly() ) {
      t << "\"-\" ";   // read from stdin
      t << stdinDataLength.toString();        // where does the track start in stdin
      t << " " << track->length().toString();   // here we need the perfect length !!!!!
      t << "\n";
      
      stdinDataLength += track->length();
    }
    else {
      t << "\"" << track->bufferFile() << "\"" << " 0" << "\n";
    }
    
    t << "\n";
  }

  return true;
}


void K3bAudioTocfileWriter::writeCdTextEntries( K3bAudioTrack* track, QTextStream& t )
{
  t << "CD_TEXT {" << "\n";
  t << "  LANGUAGE 0 {" << "\n";
  t << "    TITLE " << "\"" << prepareForTocFile(track->title()) << "\"" << "\n";
  t << "    PERFORMER " << "\"" << prepareForTocFile(track->artist()) << "\"" << "\n";
  t << "    ISRC " << "\"" << prepareForTocFile(track->isrc()) << "\"" << "\n";
  t << "    ARRANGER " << "\"" << prepareForTocFile(track->arranger()) << "\"" << "\n";
  t << "    SONGWRITER " << "\"" << prepareForTocFile(track->songwriter()) << "\"" << "\n";
  t << "    COMPOSER " << "\"" << prepareForTocFile(track->composer()) << "\"" << "\n";
  t << "    MESSAGE " << "\"" << prepareForTocFile(track->cdTextMessage()) << "\"" << "\n";
  t << "  }" << "\n";
  t << "}" << "\n";
}


QString K3bAudioTocfileWriter::prepareForTocFile( const QString& str )
{
  // since "\" is the only special character I now of so far...
  QString newStr = str;
  int pos = str.find('\\');
  while( pos > -1 ) {
    newStr.insert( pos+1, "134" );
    pos = str.find( '\\', pos+3 );
  }

  return newStr;
}


bool K3bAudioTocfileWriter::writeAudioTocFile( K3bAudioDoc* doc, const QString& filename )
{
  QFile file( filename );
  if( !file.open( IO_WriteOnly ) ) {
    kdDebug() << "(K3bAudioDoc) Could not open toc-file " << filename << endl;
    return false;
  }

  QTextStream t(&file);

  // ===========================================================================
  // header
  // ===========================================================================

  // little comment
  t << "// TOC-file to use with cdrdao created by K3b" << "\n\n";

  // we create a CDDA tocfile
  t << "CD_DA\n\n";

  return writeAudioToc( doc, t );
}
