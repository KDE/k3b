/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bcuefilewriter.h"

#include <k3btrack.h>
#include <k3bmsf.h>
#include <k3bcore.h>
#include <k3bversion.h>

#include <qfile.h>
#include <qtextstream.h>
#include <qdatetime.h>


K3bCueFileWriter::K3bCueFileWriter()
{
}


bool K3bCueFileWriter::save( const QString& filename )
{
  QFile f( filename );

  if( !f.open( IO_WriteOnly ) ) {
    kdDebug() << "(K3bCueFileWriter) could not open file " << f.name() << endl;
    return false;
  }

  QTextStream s( &f );

  return save( s );
}


bool K3bCueFileWriter::save( QTextStream& t )
{
  t << "REM Cue file written by K3b " << k3bcore->version() << endl
    << endl;

  if( !m_cdText.isEmpty() ) {
    t << "PERFORMER \"" << m_cdText.performer() << "\"" << endl;
    t << "TITLE \"" << m_cdText.title() << "\"" << endl;
  }

  t << "FILE \"" << m_image << "\" " << m_dataType.upper() << endl;

  // the tracks
  unsigned int i = 0;
  for( K3bDevice::Toc::const_iterator it = m_toc.begin();
       it != m_toc.end(); ++it ) {

    const K3bDevice::Track& track = *it;

    t << "  TRACK " << QString::number(i+1).rightJustify( 2, '0' ) << " AUDIO" << endl;

    if( m_cdText.count() > i && !m_cdText[i].isEmpty() ) {
      t << "    PERFORMER \"" << m_cdText[i].performer() << "\"" << endl;
      t << "    TITLE \"" << m_cdText[i].title() << "\"" << endl;
    }
    
    //
    // the pregap is part of the current track like in toc files
    // and not part of the last track as on the CD
    //
    if( i > 0 ) {
      --it;
      if( (*it).index0() > 0 )
	t << "    INDEX 00 " << ((*it).firstSector() + (*it).index0()).toString() << endl;
      ++it;
    }
    t << "    INDEX 01 " << track.firstSector().toString() << endl;
    // TODO: add additional indices

    i++;
  }

  return ( t.device()->status() == IO_Ok );
}
