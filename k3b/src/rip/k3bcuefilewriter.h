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

#ifndef _K3B_CUE_FILE_WRITER_H_
#define _K3B_CUE_FILE_WRITER_H_

#include <qtextstream.h>
#include <qstringlist.h>

#include <k3btoc.h>
#include <k3bcdtext.h>

namespace K3bDevice {
  class TrackCdText;
}

/**
 * Write a CDRWIN cue file.
 * For now this writer only supports audio CDs
 * for usage in the K3b audio CD ripper.
 */

class K3bCueFileWriter
{
 public:
  K3bCueFileWriter();

  bool save( QTextStream& );
  bool save( const QString& filename );

  void setData( const K3bDevice::Toc& toc ) { m_toc = toc; }
  void setCdText( const K3bDevice::CdText& text ) { m_cdText = text; }
  void setImage( const QString& name, const QString& type ) { m_image = name; m_dataType = type; }

 private:
  K3bDevice::Toc m_toc;
  K3bDevice::CdText m_cdText;
  QString m_image;
  QString m_dataType;
};

#endif
