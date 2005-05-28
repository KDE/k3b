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

#ifndef _K3B_CUEFILE_PARSER_H_
#define _K3B_CUEFILE_PARSER_H_

#include "k3bimagefilereader.h"

#include <k3btoc.h>
#include <k3bcdtext.h>
#include "k3b_export.h"
/**
 * Parses a cue file.
 * Datatracks have either mode1 or mode2 where the latter contains xa form1/2.
 * The last track may not have a proper length!
 */
class LIBK3BPROJECT_EXPORT K3bCueFileParser : public K3bImageFileReader
{
 public:
  K3bCueFileParser( const QString& filename = QString::null );
  ~K3bCueFileParser();

  /**
   * CDRDAO does not use this image filename but replaces the extension from the cue file
   * with "bin" to get the image filename.
   * So in this case cdrecord won't be able to burn the cue file. That is why we need this hack.
   */
  bool imageFilenameInCue() const { return m_imageFilenameInCue; }

  const K3bDevice::Toc& toc() const;
  const K3bDevice::CdText& cdText() const;

 private:
  void readFile();
  bool parseLine( QString& line );
  void simplifyWhiteSpace( QString& s );

  bool m_imageFilenameInCue;

  class Private;
  Private* d;
};

#endif
