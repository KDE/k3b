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

#ifndef _K3B_TOC_FILE_WRITER_H_
#define _K3B_TOC_FILE_WRITER_H_

#include <qtextstream.h>
#include <qstringlist.h>

#include <k3btoc.h>
#include <k3bcdtext.h>

namespace K3bCdDevice {
  class TrackCdText;
}

class K3bTocFileWriter
{
 public:
  K3bTocFileWriter();

  bool save( QTextStream& );
  bool save( const QString& filename );

  void setData( const K3bCdDevice::Toc& toc ) { m_toc = toc; }
  void setCdText( const K3bCdDevice::CdText& text ) { m_cdText = text; }
  void setFilenames( const QStringList& names ) { m_filenames = names; }
  void setHideFirstTrack( bool b ) { m_hideFirstTrack = b; }

  /**
   * The default is 1.
   */
  void setSession( int s ) { m_sessionToWrite = s; }

 private:
  void writeHeader( QTextStream& t );
  void writeGlobalCdText( QTextStream& t );
  void writeTrackCdText( const K3bCdDevice::TrackCdText& track, QTextStream& t );
  void writeTrack( unsigned int index, const K3b::Msf& offset, QTextStream& t );
  void writeDataSource( unsigned int trackNumber, QTextStream& t );
  bool readFromStdin() const;

  K3bCdDevice::Toc m_toc;
  K3bCdDevice::CdText m_cdText;
  QStringList m_filenames;
  bool m_hideFirstTrack;
  int m_sessionToWrite;
};

#endif
