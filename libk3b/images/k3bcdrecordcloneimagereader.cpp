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


#include "k3bcdrecordcloneimagereader.h"

#include <k3btrack.h>
#include <k3bdeviceglobals.h>

#include <qfile.h>

#include <klocale.h>


K3bCdrecordCloneImageReader::K3bCdrecordCloneImageReader()
  : K3bImageReaderBase(),
    m_bOpen(false)
{
}


K3bCdrecordCloneImageReader::K3bCdrecordCloneImageReader( const QString& file )
  : K3bImageReaderBase()
{
  open( file );
}


K3bCdrecordCloneImageReader::~K3bCdrecordCloneImageReader()
{
}


bool K3bCdrecordCloneImageReader::open( const QString& file )
{
  close();

  // first of all we check if we find the image file which contains the data for this toc
  // cdrecord always uses this strange file naming:
  //   somedata
  //   somedata.toc

  // now get rid of the ".toc" extension
  QString imageFileName = file.left( file.length()-4 );
  if( !QFile::exists( imageFileName ) ) {
    kdDebug() << "(K3bCdrecordCloneImageReader) could not find image file " << imageFileName << endl;
    return false;
  }

  setImageFileName( imageFileName );
  setTocFile( file );

  QFile f( file );
  if( f.open( IO_ReadOnly ) ) {
    //
    // Inspired by clone.c from the cdrecord sources
    //
    char buffer[2048];
    int read = f.readBlock( buffer, 2048 );
    f.close();

    if( read == 2048 ) {
      kdDebug() << "(K3bCdrecordCloneImageReader) TOC too large." << endl;
      close();
      return false;
    }

    // the toc starts with a tocheader
    struct tocheader {
      unsigned char len[2];
      unsigned char first; // first session
      unsigned char last; // last session
    };

    struct tocheader* th = (struct tocheader*)buffer;
    int dataLen = K3bDevice::from2Byte( th->len ) + 2;  // the len field does not include it's own length

    if( th->first != 1 ) {
      kdDebug() << "(K3bCdrecordCloneImageReader) first session != 1" << endl;
      close();
      return false;
    }

    // the following bytes are multible instances of
    struct ftrackdesc {
      unsigned char sess_number;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
      unsigned char adr      : 4;
      unsigned char control  : 4;
#else
      unsigned char control  : 4;
      unsigned char adr      : 4;
#endif
      unsigned char track;
      unsigned char point;
      unsigned char amin;
      unsigned char asec;
      unsigned char aframe;
      unsigned char res7;
      unsigned char pmin;
      unsigned char psec;
      unsigned char pframe;
    };

    K3b::Msf leadOut;

    for( int i = 4; i < dataLen; i += 11) {
      struct ftrackdesc* ft = (struct ftrackdesc*)&buffer[i];
      
      if( ft->sess_number != 1 ) {
	kdDebug() << "(K3bCdrecordCloneImageReader} session number != 1" << endl;
	close();
	return false;
      }

      // now we check some of the values
      if( ft->point >= 0x1 && ft->point <= 0x63 ) {
	if( ft->adr == 1 ) {
	  // check track starttime
	  if( ft->psec > 60 || ft->pframe > 75 ) {
	    kdDebug() << "(K3bCdrecordCloneImageReader) invalid track start: " 
		      << (int)ft->pmin << "." 
		      << (int)ft->psec << "."
		      << (int)ft->pframe << endl;
	    close();
	    return false;
	  }
	}
      }
      else {
	switch( ft->point ) {
	case 0xa0:
	  if( ft->adr != 1 ) {
	    kdDebug() << "(K3bCdrecordCloneImageReader) adr != 1" << endl;
	    close();
	    return false;
	  }

	  // disk type in psec
	  if( ft->psec != 0x00 && ft->psec != 0x10 && ft->psec != 0x20 ) {
	    kdDebug() << "(K3bCdrecordCloneImageReader) invalid disktype: " << ft->psec << endl;
	    close();
	    return false;
	  }

	  if( ft->pmin != 1 ) {
	    kdDebug() << "(K3bCdrecordCloneImageReader) first track number != 1 " << endl;
	    close();
	    return false;
	  }

	  if( ft->pframe != 0x0 ) {
	    kdDebug() << "(K3bCdrecordCloneImageReader) found data when there should be 0x0" << endl;
	    close();
	    return false;
	  }
	  break;

	case  0xa1:
	  if( ft->adr != 1 ) {
	    kdDebug() << "(K3bCdrecordCloneImageReader) adr != 1" << endl;
	    close();
	    return false;
	  }

	  if( !(ft->pmin >= 1) ) {
	    kdDebug() << "(K3bCdrecordCloneImageReader) last track number needs to be >= 1." << endl;
	    close();
	    return false;
	  }
	  if( ft->psec != 0x0 || ft->pframe != 0x0 ) {
	    kdDebug() << "(K3bCdrecordCloneImageReader) found data when there should be 0x0" << endl;
	    close();
	    return false;
	  }
	  break;

	case 0xa2:
	  if( ft->adr != 1 ) {
	    kdDebug() << "(K3bCdrecordCloneImageReader) adr != 1" << endl;
	    close();
	    return false;
	  }

	  // leadout... no check so far...
	  break;

	default:
	  if( ft->adr != 5 ) {
	    kdDebug() << "(K3bCdrecordCloneImageReader) adr != 5" << endl;
	    close();
	    return false;
	  }
	  break;
	}
      }

      // :( We use 00:00:00 == 0 lba)
      if( ft->adr == 1 && ft->point <= 0x63 ) {
	// ok, we seem to have a proper track on our hands.
	K3bTrack track( K3b::Msf( (int)ft->pmin, (int)ft->psec, (int)ft->pframe ) - 150,
			K3b::Msf( (int)ft->pmin, (int)ft->psec, (int)ft->pframe ) - 150,
			( ft->control & 0x4 ? K3bDevice::Track::DATA : K3bDevice::Track::AUDIO ),
			K3bDevice::Track::UNKNOWN /* FIXME */ );
			      
	if( !m_toc.isEmpty() )
	  m_toc[m_toc.count()-1].setLastSector( track.firstSector() - 1 );

	m_toc.append( track );
      }
      else if( ft->point == 0xa2 ) {
	// lead-out
	leadOut = K3b::Msf( (int)ft->pmin, (int)ft->psec, (int)ft->pframe ) - 151;
      }
    }
    
    if( !m_toc.isEmpty() )
      m_toc[m_toc.count()-1].setLastSector( leadOut );

    // ok, could be a cdrecord toc file
    m_bOpen = true;
  }
  else {
    kdDebug() << "(K3bCdrecordCloneImageReader) could not open file " << file << endl;
  }

  return isOpen();
}


void K3bCdrecordCloneImageReader::close()
{
  // since the reader does not keep any structures open we use a fake open
  // which just tells if the parsing succeeded
  m_bOpen = false; 
  m_toc.clear();
  setImageFileName( QString::null );
  setTocFile( QString::null );
}


bool K3bCdrecordCloneImageReader::isOpen() const
{
  return m_bOpen;
}


QString K3bCdrecordCloneImageReader::imageType() const
{
  return "cdrecord_clone";
}


QString K3bCdrecordCloneImageReader::imageTypeComment() const
{
  return i18n("Cdrecord Clone Image");
}


QString K3bCdrecordCloneImageReader::metaInformation() const
{
  QString s( "<p><b>" + i18n("Table of contents") + "</b><br>" );
  s.append( "<i>" + i18n("%n track", "%n tracks", toc().count() ) + " - "
	    + toc().length().toString() + " " + i18n("min") + "</i>" );
  s.append( "<p>" );
  
  unsigned int i = 1;
  for( K3bDevice::Toc::const_iterator it = toc().begin();
       it != toc().end(); ++it ) {
    
    // create track info
    s.append( i18n("Track") + " " + QString::number(i).rightJustify( 2, '0' ) );
    
    s.append( " <i>(" );
    if( (*it).type() == K3bDevice::Track::DATA )
      s.append( i18n("Data") );
    else
      s.append( i18n("Audio") );
    s.append( ")</i>" );

    s.append( "  " + (*it).length().toString() );
    s.append( "<br>" );
    
    ++i;
  }

  return s;
}
