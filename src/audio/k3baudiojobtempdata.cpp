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

#include "k3baudiojobtempdata.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include <tools/k3bglobals.h>
#include <tools/k3bversion.h>
#include <device/k3bmsf.h>
#include <k3bcore.h>

#include <qfile.h>
#include <qtextstream.h>
#include <qvaluevector.h>

#include <kdebug.h>


class K3bAudioJobTempData::Private
{
public:
  Private( K3bAudioDoc* _doc ) 
    : doc(_doc) {
  }

  QValueVector<QString> bufferFiles;
  QValueVector<QString> infFiles;
  QString tocFile;

  K3bAudioDoc* doc;
};


K3bAudioJobTempData::K3bAudioJobTempData( K3bAudioDoc* doc, QObject* parent, const char* name )
  : QObject( parent, name )
{
  d = new Private( doc );
}


K3bAudioJobTempData::~K3bAudioJobTempData()
{
  cleanup();
  delete d;
}


const QString& K3bAudioJobTempData::bufferFileName( int track )
{
  if( (int)d->bufferFiles.count() < track )
    prepareTempFileNames();
  return d->bufferFiles.at(track-1);
}

const QString& K3bAudioJobTempData::bufferFileName( K3bAudioTrack* track )
{
  return bufferFileName( track->index() + 1 );
}


const QString& K3bAudioJobTempData::tocFileName()
{
  if( d->tocFile.isEmpty() )
    prepareTempFileNames();
  return d->tocFile;
}


const QString& K3bAudioJobTempData::infFileName( int track )
{
  if( (int)d->infFiles.count() < track )
    prepareTempFileNames();
  return d->infFiles.at( track - 1 );
}

const QString& K3bAudioJobTempData::infFileName( K3bAudioTrack* track )
{
  return infFileName( track->index() + 1 );
}


// cdrecord only uses the Trackstart field to determine the pregap for the first
// track.
// modifying the pregapsize only works in DAO mode.
// WE ONLY HAVE PERFORMER AND TITLE IN THE GLOBAL CDTEXT SECTION
bool K3bAudioJobTempData::writeInfFiles()
{
  if( (int)d->infFiles.count() < d->doc->numberOfTracks() )
    prepareTempFileNames();

  QPtrListIterator<K3bAudioTrack> it( *d->doc->tracks() );

  // the data starts after the first pregap
  // we have no influence on the first 150 sectors. They will always be written
  // as null data (that is ok since the red book defines it)
  // but since the K3bAudioTrack pregap is the complete pregap we need to substract
  // the 150 sectors (at least for the first track, cdrecord seems to ignore the 
  // Trackstart field for all other tracks anyway!)
  K3b::Msf currentPos = (*it)->pregap() - 150;

  for( ; *it; ++it ) {
    K3bAudioTrack* track = *it;
    ++it;
    K3bAudioTrack* nextTrack = *it;
    --it;

    QFile f( infFileName( track ) );

    if( !f.open( IO_WriteOnly ) ) {
      kdDebug() << "(K3bAudioJobTempData) could not open file " << f.name() << endl;
      return false;
    }

    QTextStream s( &f );


    // now write the inf data
    // ----------------------
    // header
    s << "# Cdrecord-Inf-File written by K3b " << k3bcore->version() 
      << ", " << QDateTime::currentDateTime().toString() << endl
      << "#" << endl;

    s << "ISRC=\t" << track->isrc() << endl;

    // CD-Text
    if( d->doc->cdText() ) {
      // The following two fields do only make sense once
      s << "Albumperformer=\t" << "'" << d->doc->artist() << "'" << endl;
      s << "Albumtitle=\t" << "'" << d->doc->title() << "'" << endl;
      
      s << "Performer=\t" << "'" << track->artist() << "'" << endl;
      s << "Songwriter=\t" << "'" << track->songwriter() << "'" << endl;
      s << "Composer=\t" << "'" << track->composer() << "'" << endl;
      s << "Arranger=\t" << "'" << track->arranger() << "'" << endl;
      s << "Message=\t" << "'" << track->cdTextMessage() << "'" << endl;

      s << "Tracktitle=\t" << "'" << track->title() << "'" << endl;
    }

    s << "Tracknumber=\t" << track->index()+1 << endl;

    // track start
    s << "Trackstart=\t" << currentPos.totalFrames() << endl;

    // track length
    K3b::Msf length = track->length();
    if( nextTrack )
      length += nextTrack->pregap();
    s << "# Tracklength: " << length.toString() << endl;
    s << "Tracklength=\t" << length.totalFrames() << ", 0" << endl;

    // pre-emphasis
    s << "Pre-emphasis=\t";
    if( track->preEmp() )
      s << "yes";
    else
      s << "no";
    s << endl;

    // channels (always 2)
    s << "Channels=\t2" << endl;

    // copy-permitted
    // TODO: not sure about this!
    //       there are three options: yes, no, once
    //       but using "once" gives the same result as with cdrdao
    //       and that's important.
    s << "Copy_permitted=\t";
    if( track->copyProtection() )
      s << "once";
    else
      s << "yes";
    s << endl;

    // endianess - wav is little -> onthefly: big, with images: little
    s << "Endianess=\t";
    if( d->doc->onTheFly() )
      s << "big";
    else
      s << "little";
    s << endl;

    // write indices
    // the current tracks' data contains the pregap of the next track
    // if the pregap has length 0 we need no index 0
    s << "Index=\t\t0" << endl;
    s << "Index0=\t\t";
    if( !nextTrack || nextTrack->pregap() == 0 )
      s << "-1";
    else
      s << (length - nextTrack->pregap()).totalFrames();
    s << endl;

    currentPos += length;

    f.close();
  }

  return true;
}


bool K3bAudioJobTempData::writeTocFile()
{
  QFile file( tocFileName() );
  if( !file.open( IO_WriteOnly ) ) {
    kdDebug() << "(K3bAudioJobTempData) Could not open toc-file " << tocFileName() << endl;
    return false;
  }

  QTextStream t(&file);

  // ===========================================================================
  // header
  // ===========================================================================

  // little comment
  t << "// TOC-file to use with cdrdao created by K3b " << k3bcore->version()
    << ", " << QDateTime::currentDateTime().toString() << endl << endl;

  // we create a CDDA tocfile
  t << "CD_DA\n\n";

  writeAudioTocCdTextHeader( t );

  return writeAudioTocFilePart( t );
}


void K3bAudioJobTempData::writeAudioTocCdTextHeader( QTextStream& t )
{
  if( d->doc->cdText() ) {
    t << "CD_TEXT {" << "\n";
    t << "  LANGUAGE_MAP { 0: EN }\n";
    t << "  LANGUAGE 0 {\n";
    t << "    TITLE " << "\"" << encodeForTocFile(d->doc->title()) << "\"" << "\n";
    t << "    PERFORMER " << "\"" << encodeForTocFile(d->doc->artist()) << "\"" << "\n";
    t << "    DISC_ID " << "\"" << encodeForTocFile(d->doc->disc_id()) << "\"" << "\n";
    t << "    UPC_EAN " << "\"" << encodeForTocFile(d->doc->upc_ean()) << "\"" << "\n";
    t << "\n";
    t << "    ARRANGER " << "\"" << encodeForTocFile(d->doc->arranger()) << "\"" << "\n";
    t << "    SONGWRITER " << "\"" << encodeForTocFile(d->doc->songwriter()) << "\"" << "\n";
    t << "    COMPOSER " << "\"" << encodeForTocFile(d->doc->composer()) << "\"" << "\n";
    t << "    MESSAGE " << "\"" << encodeForTocFile(d->doc->cdTextMessage()) << "\"" << "\n";
    t << "  }" << "\n";
    t << "}" << "\n\n";
  }
}


bool K3bAudioJobTempData::writeAudioTocFilePart( QTextStream& t, const K3b::Msf& startMsf )
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


  K3b::Msf stdinDataLength(startMsf);

  // ===========================================================================
  // the tracks
  // ===========================================================================

  QPtrListIterator<K3bAudioTrack> it( *d->doc->tracks() );

  // if we need to hide the first song in the first tracks' pregap
  // we process the first two songs at once

  if( d->doc->hideFirstTrack() ) {
    // we cannot hide a lonely track
    if( d->doc->numberOfTracks() > 1 ) {
      K3bAudioTrack* hiddenTrack = *it;
      ++it;
      K3bAudioTrack* track = *it;

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

      if( d->doc->cdText() ) {
	writeCdTextEntries( track, t );
      }

      // the "hidden" file will be used as pregap for the "first" track
      t << "FILE ";
      if( d->doc->onTheFly() ) {
	t << "\"-\" ";   // read from stdin
	t << stdinDataLength.toString();        // where does the track start in stdin
	t << " " << hiddenTrack->length().toString();   // here we need the perfect length !!!!!
	t << "\n";
	
	stdinDataLength += hiddenTrack->length();
      }
      else {
	t << "\"" << bufferFileName(hiddenTrack) << "\"" << " 0" << "\n";
      }
      t << "START" << "\n"; // use the whole hidden file as pregap


      // now comes the "real" first track
      t << "FILE ";
      if( d->doc->onTheFly() ) {
	t << "\"-\" ";   // read from stdin
	t << stdinDataLength.toString();        // where does the track start in stdin
	t << " " << track->length().toString();   // here we need the perfect length !!!!!
	t << "\n";
	
	stdinDataLength += track->length();
      }
      else {
	t << "\"" << bufferFileName(track) << "\"" << " 0" << "\n";
      }
      t << "\n";
    }

    ++it;
  }


  // now iterate over the rest of the tracks
  
  for( ; *it; ++it ) {
    K3bAudioTrack* track = *it;

    t << "TRACK AUDIO" << "\n";

    if( track->copyProtection() )
      t << "NO COPY" << "\n";
    else
      t << "COPY" << "\n";

    if( track->preEmp() )
      t << "PRE_EMPHASIS" << "\n";
//     else
//       t << "NO PRE_EMPHASIS" << "\n";

    if( d->doc->cdText() ) {
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

      // the first track is the only track K3b does not generate null-pregap data for
      // since cdrecord does not allow this. So We just do it here the same way and tell
      // cdrdao to create the first pregap for us
      if( p > 0 )
	t << "PREGAP " << p.toString() << "\n";
      t << "FILE ";
      if( d->doc->onTheFly() ) {
	t << "\"-\" "   // read from stdin
	  << stdinDataLength.toString()
	  << " "
	  << track->length().toString()
	  << endl;

	stdinDataLength += track->length();
      }
      else {
	t << "\"" << bufferFileName(track) << "\"" << " 0" << endl;
      }
    }
    else {
      // here we need to specify the last track's buffer file or "-" to create the pregap
      // in the case of the file it's "FILE "filename" (lastTrack->length() - track->pregap()).toString() 0"
      // in the case of stdin it's "FILE "-" currentPos.toString() track->pregap().toString()"

      --it;
      K3bAudioTrack* lastTrack = *it;
      ++it;

      t << "FILE ";
      if( d->doc->onTheFly() ) {
	t << "\"-\" "   // read from stdin
	  << stdinDataLength.toString()
	  << " "
	  << (track->pregap() + track->length()).toString()
	  << endl
	  << "START " 
	  << track->pregap().toString()
	  << endl;

	stdinDataLength += track->pregap();
	stdinDataLength += track->length();
      }
      else {
	if( track->pregap() > 0 ) {
	  t << "\"" << bufferFileName(lastTrack) << "\" "
	    << lastTrack->length().toString()
	    << endl;
	  t << "START" << endl;
	  t << "FILE ";
	}
	t << "\"" << bufferFileName(track) << "\"" 
	  << " 0" 
	  << endl;
      }
    }
    
    t << endl;
  }

  return true;
}


void K3bAudioJobTempData::writeCdTextEntries( K3bAudioTrack* track, QTextStream& t )
{
  t << "CD_TEXT {" << "\n";
  t << "  LANGUAGE 0 {" << "\n";
  t << "    TITLE " << "\"" << encodeForTocFile(track->title()) << "\"" << "\n";
  t << "    PERFORMER " << "\"" << encodeForTocFile(track->artist()) << "\"" << "\n";
  t << "    ISRC " << "\"" << encodeForTocFile(track->isrc()) << "\"" << "\n";
  t << "    ARRANGER " << "\"" << encodeForTocFile(track->arranger()) << "\"" << "\n";
  t << "    SONGWRITER " << "\"" << encodeForTocFile(track->songwriter()) << "\"" << "\n";
  t << "    COMPOSER " << "\"" << encodeForTocFile(track->composer()) << "\"" << "\n";
  t << "    MESSAGE " << "\"" << encodeForTocFile(track->cdTextMessage()) << "\"" << "\n";
  t << "  }" << "\n";
  t << "}" << "\n";
}


K3bAudioDoc* K3bAudioJobTempData::doc() const
{
  return d->doc;
}


void K3bAudioJobTempData::prepareTempFileNames( const QString& path ) 
{
  d->bufferFiles.clear();
  d->infFiles.clear();

  QString prefix = K3b::findUniqueFilePrefix( "k3b_audio_" + d->doc->title(), path ) + "_";

  for( int i = 0; i < d->doc->numberOfTracks(); i++ ) {
    d->bufferFiles.append( prefix + QString::number( i+1 ).rightJustify( 2, '0' ) + ".wav" );
    d->infFiles.append( prefix + QString::number( i+1 ).rightJustify( 2, '0' ) + ".inf" );
  }

  d->tocFile = prefix + ".toc";
}


void K3bAudioJobTempData::cleanup()
{
  for( uint i = 0; i < d->infFiles.count(); ++i ) {
    if( QFile::exists( d->infFiles[i] ) )
      QFile::remove(  d->infFiles[i] );
  }

  if( QFile::exists( d->tocFile ) )
    QFile::remove(  d->tocFile );
}


QString K3bAudioJobTempData::encodeForTocFile( const QString& str )
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

#include "k3baudiojobtempdata.moc"
