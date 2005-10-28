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

#include "k3bimage.h"
#include "k3bsimpletar.h"

#include <k3biodevicewrapper.h>
#include <k3btrack.h>
#include <k3btoc.h>
#include <k3bglobals.h>

#include <qdom.h>
#include <qvaluevector.h>
#include <qbuffer.h>
#include <qtextstream.h>

#include <math.h>


static bool isTrue( const QString& s )
{
  return ( s.lower() != "false" && s.toInt() != 0 );
}

static QString trackTypeString( const K3bTrack& track )
{
  if( track.type() == K3bDevice::Track::AUDIO )
    return "Audio";
  else {
    switch( track.mode() ) {
    case K3bDevice::Track::MODE2:
      return "Data Mode2";
    case K3bDevice::Track::XA_FORM1:
      return "Data XA Form1";
    case K3bDevice::Track::XA_FORM2:
      return "Data XA Form2";
    default:
      return "Data Mode1";
    }
  }
}


class K3bImageTrackData
{
public:
  K3bImageTrackData()
    : completelyWritten( false ) {}

  QValueVector<QString> filenames;
  QValueVector<KIO::filesize_t> startOffsets;
  QValueVector<KIO::filesize_t> lengths;

  K3b::SectorSize sectorSize;

  bool completelyWritten;
};


class K3bImage::Private
{
public:
  Private()
    : bOpen( false ),
      tar( 0 ),
      trackOpenForWriting( 0 ) {
  }

  bool bOpen;

  K3bSimpleTar* tar;
  int openMode;

  QValueVector<K3bImageTrackData> trackData;

  // the track we are currently writing in IO_WriteOnly mode (starting at 1)
  unsigned int trackOpenForWriting;

  // the track chunk we are currently writing in case the track data is bigger
  // than 1GB (starting at 0)
  unsigned int trackChunk;

  // the number of bytes already written for the current track in IO_WriteOnly mode
  unsigned long long trackBytesWritten;

  // the number of bytes already written for the current track chunk in IO_WriteOnly mode
  unsigned long long trackChunkBytesWritten;
};


K3bImage::K3bImage()
{
  d = new Private;
}


K3bImage::K3bImage( const QString& filename, int mode )
{
  d = new Private;
  open( filename, mode );
}


K3bImage::~K3bImage()
{
  delete d->tar;
  close();
}


bool K3bImage::open( const QString& filename, int mode )
{
  close();

  d->openMode = mode;
  d->trackOpenForWriting = 0;
  d->trackBytesWritten = 0ULL;
  m_filename = filename;

  if( !d->tar )
    d->tar = new K3bSimpleTar();

  if( d->tar->open( filename, mode ) ) {
    if( mode == IO_ReadOnly ) {
      if( readToc() ) {
	d->bOpen = true;
	return true;
      }
    }
    else {
      // we are open for writing
      d->bOpen = true;
      return true;
    }
  }

//   delete d->tar;
//   d->tar = 0;

  return false;
}


bool K3bImage::isOpen() const
{
  return d->bOpen;
}


void K3bImage::close()
{
  if( d->bOpen ) {
    if( d->openMode == IO_WriteOnly ) {
      if( !m_cdText.isEmpty() )
	saveCdText();
      saveToc();
    }

    d->tar->close();

    d->trackData.clear();
    m_cdText.clear();
    m_toc.clear();

    m_filename.truncate( 0 );
    d->bOpen = false;

//     delete d->tar;
//     d->tar = 0;
  }
}


K3b::SectorSize K3bImage::sectorSize( unsigned int track ) const
{
  return d->trackData[track-1].sectorSize;
}


QIODevice* K3bImage::readTrack( unsigned int track )
{
  if( !isOpen() || track > m_toc.count() )
    return false;

  // first we find us the corresponding files
  K3bImageTrackData& data = d->trackData[track-1];
  
  K3bIODeviceWrapper* dw = new K3bIODeviceWrapper( true );

  // FIXME
//   for( unsigned int i = 0; i < data.filenames.count(); ++i )
//     dw->addWrappedIODevice( static_cast<const KArchiveFile*>( d->tar->directory()->entry( data.filenames[i] ) )->device(), 
// 			    data.startOffsets[i], 
// 			    data.lengths[i] );
  
  return dw;
}


void K3bImage::clear()
{
  if( isOpen() && d->openMode == IO_WriteOnly ) {
    open( filename(), IO_WriteOnly );
  }
}


bool K3bImage::writeTrack( unsigned int track, K3b::SectorSize size )
{
  if( d->trackOpenForWriting ) {
    kdDebug() << "(K3bImage) last track has not been completely written." << endl;
    return false;
  }

  if( isOpen() && d->openMode == IO_WriteOnly ) {
    if( track > m_toc.count() )
      return false;

    K3bImageTrackData& data = d->trackData[track-1];

    data.sectorSize = size;

    //
    // We use 1GB files, let's see how may files we need
    // Although tar supports files bigger than 2GB, KTar does not (or to be more exact: KArchive does not
    // because KArchiveFile::size() returns an int.) We could of course use a different
    // backend but using file sizes that stay below the 32 bit barrier keeps us on the safe side anyway.
    //
    static const KIO::filesize_t oneGB = 1024ULL*1024ULL*1024ULL;
    KIO::filesize_t trackDataSize = (KIO::filesize_t)size * (KIO::filesize_t)m_toc[track-1].length().lba();
    int numFiles = (int)::ceil( (double)trackDataSize / (double)oneGB );

    kdDebug() << "(K3bImage) splitting track " << track << " into " << numFiles << " chunks." << endl;

    data.filenames.clear();
    data.startOffsets.clear();
    data.lengths.clear();
    data.completelyWritten = false;

    for( int i = 0; i < numFiles; ++i ) {
      data.filenames.append( QString( "track%1_%2").arg(track).arg(i) );
      data.startOffsets.append( 0 );
      if( i < numFiles-1 )
	data.lengths.append( oneGB );
      else
	data.lengths.append( trackDataSize - oneGB*(KIO::filesize_t)i );
    }

    if( d->tar->prepareWriting( data.filenames[0], data.lengths[0] ) ) {
      d->trackOpenForWriting = track;
      d->trackBytesWritten = 0ULL;
      d->trackChunk = 0;
      d->trackChunkBytesWritten = 0ULL;
      return true;
    }
  }

  return false;
}


bool K3bImage::writeTrackData( const char* buf, unsigned int len )
{
  if( d->trackOpenForWriting ) {

    K3bImageTrackData& data = d->trackData[d->trackOpenForWriting-1];

    unsigned int bufRest = 0;

    //
    // Since we split the track data into 1GB chunks we have to check if we filled up the
    // current chunk.
    //
    if( d->trackChunkBytesWritten + (KIO::filesize_t)len > data.lengths[d->trackChunk] ) {
      unsigned int newLen = (unsigned int)(data.lengths[d->trackChunk] - d->trackChunkBytesWritten);
      bufRest = len - newLen;
      len = newLen;
    }

    if( d->tar->writeData( buf, len ) ) {
      d->trackBytesWritten += (KIO::filesize_t)len;
      d->trackChunkBytesWritten += (KIO::filesize_t)len;
      KIO::filesize_t trackSize = m_toc[d->trackOpenForWriting-1].length().lba() * (KIO::filesize_t)data.sectorSize;

      // did we finish this track completely?
      if( d->trackBytesWritten >= trackSize ) {

	kdDebug() << "(K3bImage) " << d->trackBytesWritten << " bytes written for track " 
		  << d->trackOpenForWriting << endl;

	d->trackOpenForWriting = 0;
	d->tar->doneWriting( d->trackChunkBytesWritten );

	data.completelyWritten = true;
      }

      // or do we need to open the next chunk?
      else if( d->trackChunkBytesWritten == data.lengths[d->trackChunk] ) {

	d->tar->doneWriting( d->trackChunkBytesWritten );

	++d->trackChunk;
	d->trackChunkBytesWritten = 0ULL;

	kdDebug() << "(K3bImage) writing chunk " << d->trackChunk << " of track " << d->trackOpenForWriting << endl;

	if( !d->tar->prepareWriting( data.filenames[d->trackChunk], 
				     data.lengths[d->trackChunk] ) ) {
	  d->trackOpenForWriting = 0;
	  return false;
	}

	//
	// We did not use up the whole data for the last chunk
	//
	if( bufRest > 0 ) {
	  if( d->tar->writeData( buf+len, bufRest ) ) {
	    d->trackChunkBytesWritten += (KIO::filesize_t)bufRest;
	    d->trackBytesWritten += (KIO::filesize_t)bufRest;
	  }
	  else
	    return false;
	}
      }

      return true;
    }
  }
  else
    kdDebug() << "(K3bImage) call to writeTrackData() without an open track." << endl;

  return false;
}


QIODevice* K3bImage::writingDevice()
{
  if( d->trackOpenForWriting ) {
    // FIXME: create some writing device class
    return 0;
  }
  else
    return 0;
}


void K3bImage::setToc( const K3bDevice::Toc& toc )
{
  if( isOpen() && d->openMode == IO_WriteOnly ) {
    m_toc = toc;
    d->trackData.resize( m_toc.count() );
  }
}


void K3bImage::setCdText( const K3bDevice::CdText& t )
{
  if( isOpen() && d->openMode == IO_WriteOnly ) {
    m_cdText = t;
  }
}


bool K3bImage::readToc()
{
  clear();

  K3bSimpleTarEntry* archiveEntry = d->tar->fileEntry( "toc.xml" );
  if( archiveEntry ) {

    QDomDocument xmlDoc;
    QString error;
    int line, col;
    if( !xmlDoc.setContent( archiveEntry->readAll(), &error, &line, &col ) ) {
      kdDebug() << "(K3bImage) unable to read toc.xml file." << endl;
      kdDebug() << "(K3bImage) " << error << " (at " << line << "," << col << ")" << endl;
      return false;
    }

    //
    // Start the parsing of the toc
    //
    QDomElement rootElem = xmlDoc.documentElement();

    if( rootElem.tagName() != "image" ) {
      kdDebug() << "(K3bImage) unknown root tag: " << rootElem.tagName() << endl;
      return false;
    }

    QDomNode n = rootElem.firstChild();
    if( !n.isElement() || n.nodeName() != "numsessions" ) {
      kdDebug() << "(K3bImage) expecting tag numsessions." << endl;
      return false;
    }
    unsigned int numsessions = n.toElement().text().toInt();

    n = n.nextSibling();
    if( !n.isElement() || n.nodeName() != "numtracks" ) {
      kdDebug() << "(K3bImage) expecting tag numtracks." << endl;
      return false;
    }
    //    unsigned int numtracks = n.toElement().text().toInt();

    n = n.nextSibling();
    if( n.isNull() || !n.isElement() ) {
      kdDebug() << "(K3bImage) expecting tags cdtext, catalog or session." << endl;
      return false;
    }
    QDomElement e = n.toElement();
    if( e.tagName() == "cdtext" ) {

      // read the filename and see if the file is there
      QString filename;
      KIO::filesize_t startOffset = 0;
      KIO::filesize_t fileSize = 0;
      if( !readFilename( e, filename, startOffset, fileSize ) )
	return false;

      K3bSimpleTarEntry* archiveEntry = d->tar->fileEntry( filename );
      if( archiveEntry ) {

	// for the cdtext we directly read the data
	m_cdText.setRawPackData( archiveEntry->readAll() );
      }

      n = n.nextSibling();
      if( n.isNull() || !n.isElement() ) {
	kdDebug() << "(K3bImage) expecting tags catalog or session." << endl;
	return false;
      }
      e = n.toElement();
    }

    if( e.tagName() == "catalog" ) {
      m_toc.setMcn( e.text().ascii() );

      n = n.nextSibling();
      if( n.isNull() || !n.isElement() ) {
	kdDebug() << "(K3bImage) expecting tag session." << endl;
	return false;
      }
      e = n.toElement();
    }

    K3b::Msf pos;

    for( unsigned int i = 1; i <= numsessions; ++i ) {
      if( e.isNull() || e.tagName() != "session" ) {
	kdDebug() << "(K3bImage) expecting tag session." << endl;
	return false;
      }

      //
      // read a session
      //
      // session number
      if( !e.hasAttribute( "number" ) ) {
	kdDebug() << "(K3bImage) expecting attribute number in tag session." << endl;
	return false;
      }

      unsigned int sessionNum = e.attributeNode( "number" ).value().toInt();

      QDomNode sn = n.firstChild();
      if( sn.isNull() || sn.nodeName() != "numtracks" ) {
	kdDebug() << "(K3bImage) expecting tag numtracks." << endl;
	return false;
      }

      unsigned int numTracksInSession = sn.toElement().text().toInt();

      sn = sn.nextSibling();
      QDomElement se = sn.toElement();

      for( unsigned int j = 1; j <= numTracksInSession; ++j ) {
	if( se.isNull() || se.tagName() != "track" ) {
	  kdDebug() << "(K3bImage) expecting tag track." << endl;
	  return false;
	}

	if( !se.hasAttribute( "type" ) ) {
	  kdDebug() << "(K3bImage) expecting attribute type." << endl;
	  return false;
	}

	int trackType = 0;
	int trackMode = 0;

	QString trackTypeString = se.attributeNode( "type" ).value();
	if( trackTypeString == "Audio" ) {
	  trackType = K3bDevice::Track::AUDIO;
	  trackMode = K3bDevice::Track::UNKNOWN;
	}
	else if( trackTypeString == "Data Mode1" ) {
	  trackType = K3bDevice::Track::DATA;
	  trackMode = K3bDevice::Track::MODE1;
	}
	else if( trackTypeString == "Data XA Form1" ) {
	  trackType = K3bDevice::Track::DATA;
	  trackMode = K3bDevice::Track::XA_FORM1;
	}
	else if( trackTypeString == "Data XA Form2" ) {
	  trackType = K3bDevice::Track::DATA;
	  trackMode = K3bDevice::Track::XA_FORM2;
	}
	else  {
	  kdDebug() << "(K3bImage) unknown track type: " << trackTypeString << endl;
	  return false;
	}


	//
	// read a track
	//
	QDomNode tn = sn.firstChild();
	QDomElement te = tn.toElement();

	if( te.isNull() || te.tagName() != "data" ) {
	  kdDebug() << "(K3bImage) expecting tag data." << endl;
	  return false;
	}
	if( !te.hasAttribute( "sectorsize" ) ) {
	  kdDebug() << "(K3bImage) expecting attribute sectorsize." << endl;
	  return false;
	}

	int sectorsize = te.attributeNode( "sectorsize" ).value().toInt();
	switch( sectorsize ) {
	case K3b::SECTORSIZE_AUDIO:
	case K3b::SECTORSIZE_DATA_2048:
	case K3b::SECTORSIZE_DATA_2048_SUBHEADER:
	case K3b::SECTORSIZE_DATA_2324:
	case K3b::SECTORSIZE_DATA_2324_SUBHEADER:
	case K3b::SECTORSIZE_RAW:
	  // valid
	  break;
	default:
	  kdDebug() << "(K3bImage) invalid sector size: " << sectorsize << endl;
	  return false;
	}

	// read the filename and see if the file is there
	K3bImageTrackData trackData;
	trackData.sectorSize = (K3b::SectorSize)sectorsize;
	K3b::Msf trackLen;

	QDomElement dataE = te.firstChild().toElement();
	while( !dataE.isNull() && dataE.tagName() == "file" ) {
	  QString f;
	  KIO::filesize_t so, le;
	  if( !readFilename( dataE, f, so, le ) )
	    return false;

	  trackData.startOffsets.append( so );
	  trackData.lengths.append( le );
	  trackData.filenames.append( f );

	  trackLen += le/sectorsize;

	  dataE = te.nextSibling().toElement();
	}

	// create the new track
	K3bDevice::Track track( pos,
				pos + trackLen - 1,
				trackType,
				trackMode );


	tn = tn.nextSibling();
	te = tn.toElement();

	while( !te.isNull() && te.tagName() == "index" ) {
	  int indexNum = te.attributeNode( "number" ).value().toInt();
	  K3b::Msf indexVal( te.text().toInt() );

	  if( indexNum == 0 )
	    track.setIndex0( indexVal );
	  else if( indexNum > 1 ) {
	    // FIXME: add index (there is no possibility in K3bTrack yet!
	    kdDebug() << "(K3bImage) IMPLEMENT INDEX > 1 PARSING!" << endl;
	  }
	  else {
	    kdDebug() << "(K3bImage) invalid index number " << indexNum << endl;
	    return false;
	  }

	  tn = tn.nextSibling();
	  te = tn.toElement();
	}

	// read preemph
	if( !te.isNull() && te.tagName() == "preemp" ) {
	  track.setPreEmphasis( isTrue( te.text() ) );

	  tn = tn.nextSibling();
	  te = tn.toElement();
	}

	// read copy
	if( !te.isNull() && te.tagName() == "copy" ) {
	  track.setCopyPermitted( isTrue( te.text() ) );

	  tn = tn.nextSibling();
	  te = tn.toElement();
	}

	// read scms
	if( !te.isNull() && te.tagName() == "scms" ) {
	  // FIXME: isTrue( te.text() ) );

	  tn = tn.nextSibling();
	  te = tn.toElement();
	}

	// read isrc
	if( !te.isNull() && te.tagName() == "isrc" ) {
	  track.setIsrc( te.text().ascii() );

	  tn = tn.nextSibling();
	  te = tn.toElement();
	}

	track.setSession( sessionNum );
	m_toc.append( track );
	d->trackData.append( trackData );

	pos += trackLen;


	sn = sn.nextSibling();
	se = sn.toElement();
      }      

      n = n.nextSibling();
      e = n.toElement();
    }

    return true;
  }
  else {
    kdDebug() << "(K3bImage) No toc.xml file found." << endl;
    return false;
  }
}


bool K3bImage::readFilename( QDomElement& e, 
			     QString& filename, 
			     KIO::filesize_t& startOffset, 
			     KIO::filesize_t& fileSize )
{
  filename = e.text();

  K3bSimpleTarEntry* fileEntry = d->tar->fileEntry( filename );
  if( fileEntry == 0 ) {
    kdDebug() << "(K3bImage) invalid data filename: " << filename << endl;
    return false;
  }
  
  startOffset = 0;
  // FIXME: int is way too small! Maybe we have to split big tracks into several files. :(
  fileSize = fileEntry->size();
  
  if( e.hasAttribute( "start_offset" ) )
    startOffset = e.attributeNode( "start_offset" ).value().toULongLong();

  if( e.hasAttribute( "length" ) )
    fileSize = e.attributeNode( "length" ).value().toULongLong();

  if( fileSize + startOffset > (KIO::filesize_t)fileEntry->size() ) {
    kdDebug() << "(K3bImage) invalid size values for image file " << filename << endl;
    return false;
  }

  return true;
}


bool K3bImage::saveToc()
{
  if( isOpen() && 
      d->openMode == IO_WriteOnly ) {

    //
    // now let's see if all track data has been stored
    //
    for( unsigned int i = 0; i < d->trackData.count(); ++i ) {
      if( !d->trackData[i].completelyWritten ) {
	kdDebug() << "(K3bImage) data for track " << (i+1) << " not complete." << endl;
      }
    }


    //
    // We have all files. Let's write the toc
    //
    QDomDocument doc;
    QDomElement imageE = doc.createElement( "image" );


    // NUMSESSIONS
    QDomElement e = doc.createElement( "numsessions" );
    e.appendChild( doc.createTextNode( QString::number( m_toc.sessions() ) ) );
    imageE.appendChild( e );

    // NUMTRACKS
    e = doc.createElement( "numtracks" );
    e.appendChild( doc.createTextNode( QString::number( m_toc.count() ) ) );
    imageE.appendChild( e );

    // CD-TEXT
    if( !m_cdText.isEmpty() ) {

      // the cdtext file will be written in saveCdText()

      QDomElement cdTextE = doc.createElement( "cdtext" );
      cdTextE.appendChild( doc.createTextNode( "cdtext" ) );

      imageE.appendChild( cdTextE );
    }

    // CATALOG
    if( !m_toc.mcn().isEmpty() ) {
      e = doc.createElement( "catalog" );
      e.appendChild( doc.createTextNode( m_toc.mcn() ) );
      imageE.appendChild( e );
    }

    // SESSIONS
    unsigned int trackNum = 1;
    int firstSession = 1;

    // there is one special case where the first session's number is two. It is used
    // to append sessions to multisession CD/DVD.
    if( m_toc[0].session() > 1 )
      firstSession = m_toc[0].session();

    for( int sessionNum = firstSession; sessionNum < m_toc.sessions() + firstSession; ++sessionNum ) {

      // create the session tag
      QDomElement sessionE = doc.createElement( "session" );
      sessionE.setAttribute( "number", sessionNum );

      // add the numtracks tag here to honor the tag ordering
      sessionE.appendChild( doc.createElement( "numtracks" ) );

      // TRACKS
      unsigned int numTracks = 0;
      while( trackNum <= m_toc.count() && 
	     ( m_toc[trackNum-1].session() == 0 ||
	       m_toc[trackNum-1].session() == sessionNum ) ) {

	// count the tracks in this session to save the numtracks value later on
	numTracks++;

	// create a track element
	QDomElement trackE = doc.createElement( "track" );
	trackE.setAttribute( "number", trackNum );
	trackE.setAttribute( "type", trackTypeString( m_toc[trackNum-1] ) );

	// now save the file (and see if is actually there)
	K3bImageTrackData& trackData = d->trackData[trackNum-1];

	QDomElement dataE = doc.createElement( "data" );
	dataE.setAttribute( "sectorsize", trackData.sectorSize );

	//
	// Save all the files used for the track data. In case the track data is bigger than 1 GB we have
	// more than one.
	//
	// We do not need the start_offset and length fields though since one file always belongs entirely
	// to one track.
	//
	for( unsigned int i = 0; i < trackData.filenames.count(); ++i ) {
	  QDomElement fileE = doc.createElement( "file" );
	  fileE.appendChild( doc.createTextNode( trackData.filenames[i] ) );
	  dataE.appendChild( fileE );
	}

	trackE.appendChild( dataE );

	if( m_toc[trackNum-1].type() == K3bDevice::Track::AUDIO ) {
	  
	  // save index 0
	  if( m_toc[trackNum-1].index0() > 0 ) {
	    QDomElement indexE = doc.createElement( "index" );
	    indexE.setAttribute( "number", 0 );
	    indexE.appendChild( doc.createTextNode( QString::number( m_toc[trackNum-1].index0().lba() ) ) );

	    trackE.appendChild( indexE );
	  }

	  // save all other indices (starting with index number 2)
	  int indexNum = 2;
	  for( QValueVector<K3b::Msf>::const_iterator it = m_toc[trackNum-1].indices().begin();
	       it != m_toc[trackNum-1].indices().end(); ++it, ++indexNum ) {
	    QDomElement indexE = doc.createElement( "index" );
	    indexE.setAttribute( "number", indexNum );
	    indexE.appendChild( doc.createTextNode( QString::number( (*it).lba() ) ) );

	    trackE.appendChild( indexE );
	  }

	  e = doc.createElement( "preemp" );
	  e.appendChild( doc.createTextNode( m_toc[trackNum-1].preEmphasis() ? "true" : "false" ) );
	  trackE.appendChild( e );

	  e = doc.createElement( "copy" );
	  e.appendChild( doc.createTextNode( m_toc[trackNum-1].copyPermitted() ? "true" : "false" ) );
	  trackE.appendChild( e );

	  // FIXME: add SCMS (once it's in K3bTrack)
	}

	if( !m_toc[trackNum-1].isrc().isEmpty() ) {
	  e = doc.createElement( "isrc" );
	  e.appendChild( doc.createTextNode( m_toc[trackNum-1].isrc() ) );
	  trackE.appendChild( e );
	}

	++trackNum;

	sessionE.appendChild( trackE );
      }

      // now update the numtracks element
      sessionE.firstChild().appendChild( doc.createTextNode( QString::number( numTracks ) ) );

      // and add the session element to the image tree
      imageE.appendChild( sessionE );
    }

    doc.appendChild( imageE );

    // save the toc inside the store
    QByteArray data;
    QBuffer buf( data );
    buf.open( IO_WriteOnly );
    QTextStream xmlStream( &buf );
    doc.save( xmlStream, 0 );
    buf.close();
    return d->tar->writeFile( "toc.xml", data.size(), data.data() );
  }

  return false;
}


bool K3bImage::saveCdText()
{
  if( isOpen() && 
      d->openMode == IO_WriteOnly ) {

    QByteArray data = m_cdText.rawPackData();  
    
    return d->tar->writeFile( "cdtext", data.size(), data.data() );
  }

  return false;
}
