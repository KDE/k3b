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

#include <config.h>


#include "k3bclonetocreader.h"

#include <k3bdeviceglobals.h>

#include <qfile.h>
#include <qtextstream.h>

#include <kdebug.h>



K3bCloneTocReader::K3bCloneTocReader( const QString& filename )
  : K3bImageFileReader()
{
  openFile( filename );
}


K3bCloneTocReader::~K3bCloneTocReader()
{
}


void K3bCloneTocReader::readFile()
{
  // first of all we check if we find the image file which contains the data for this toc
  // cdrecord always uses this strange file naming:
  //   somedata
  //   somedata.toc

  // now get rid of the ".toc" extension
  QString imageFileName = filename().left( filename().length()-4 );
  if( !QFile::exists( imageFileName ) ) {
    kdDebug() << "(K3bCloneTocReader) could not find image file " << imageFileName << endl;
    return;
  }

  setImageFilename( imageFileName );

  QFile f( filename() );
  if( f.open( IO_ReadOnly ) ) {
    //
    // Inspired by clone.c from the cdrecord sources
    //
    char buffer[2048];
    int read = f.readBlock( buffer, 2048 );
    f.close();

    if( read == 2048 ) {
      kdDebug() << "(K3bCloneTocReader) TOC too large." << endl;
      return;
    }

    // the toc starts with a tocheader
    struct tocheader {
      unsigned char len[2];
      unsigned char first; // first session
      unsigned char last; // last session
    };

    struct tocheader* th = (struct tocheader*)buffer;
    int dataLen = K3bCdDevice::from2Byte( th->len ) + 2;  // the len field does not include it's own length

    if( th->first != 1 ) {
      kdDebug() << "(K3bCloneTocReader) first session != 1" << endl;
      return;
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

    for( int i = 4; i < dataLen; i += 11) {
      struct ftrackdesc* ft = (struct ftrackdesc*)&buffer[i];
      
      if( ft->sess_number != 1 ) {
	kdDebug() << "(K3bCloneTocReader} session number != 1" << endl;
	return;
      }

      // now we check some of the values
      if( ft->point >= 0x1 && ft->point <= 0x63 ) {
	if( ft->adr == 1 ) {
	  // check track starttime
	  if( ft->psec > 60 || ft->pframe > 75 ) {
	    kdDebug() << "(K3bCloneTocReader) invalid track start: " 
		      << (int)ft->pmin << "." 
		      << (int)ft->psec << "."
		      << (int)ft->pframe << endl;
	    return;
	  }
	}
      }
      else {
	switch( ft->point ) {
	case 0xa0:
	  if( ft->adr != 1 ) {
	    kdDebug() << "(K3bCloneTocReader) adr != 1" << endl;
	    return;
	  }

	  // disk type in psec
	  if( ft->psec != 0x00 && ft->psec != 0x10 && ft->psec != 0x20 ) {
	    kdDebug() << "(K3bCloneTocReader) invalid disktype: " << ft->psec << endl;
	    return;
	  }

	  if( ft->pmin != 1 ) {
	    kdDebug() << "(K3bCloneTocReader) first track number != 1 " << endl;
	    return;
	  }

	  if( ft->pframe != 0x0 ) {
	    kdDebug() << "(K3bCloneTocReader) found data when there should be 0x0" << endl;
	    return;
	  }
	  break;

	case  0xa1:
	  if( ft->adr != 1 ) {
	    kdDebug() << "(K3bCloneTocReader) adr != 1" << endl;
	    return;
	  }

	  if( !(ft->pmin >= 1) ) {
	    kdDebug() << "(K3bCloneTocReader) last track number needs to be >= 1." << endl;
	    return;
	  }
	  if( ft->psec != 0x0 || ft->pframe != 0x0 ) {
	    kdDebug() << "(K3bCloneTocReader) found data when there should be 0x0" << endl;
	    return;
	  }
	  break;

	case 0xa2:
	  if( ft->adr != 1 ) {
	    kdDebug() << "(K3bCloneTocReader) adr != 1" << endl;
	    return;
	  }

	  // leadout... no check so far...
	  break;

	default:
	  if( ft->adr != 5 ) {
	    kdDebug() << "(K3bCloneTocReader) adr != 5" << endl;
	    return;
	  }
	  break;
	}
      }
    }
    
    // ok, could be a cdrecord toc file
    setValid(true);
  }
  else {
    kdDebug() << "(K3bCloneTocReader) could not open file " << filename() << endl;
  }
}
