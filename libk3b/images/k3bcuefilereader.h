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


#ifndef _K3B_CUEFILE_READER_H
#define _K3B_CUEFILE_READER_H_

#include "k3bimagereaderbase.h"

#include <k3bcuefileparser.h>
#include <k3btoc.h>
#include <k3bcdtext.h>

#include "k3b_export.h"


class LIBK3B_EXPORT K3bCueFileReader : public K3bImageReaderBase
{
 public:
  K3bCueFileReader();
  K3bCueFileReader( const QString& );
  ~K3bCueFileReader();

  bool open( const QString& file );
  void close();
  bool isOpen() const;

  QString imageType() const;
  QString imageTypeComment() const;

  /**
   * \return true if this is an audio cue file, otherwise false.
   */
  // FIXME: make this return false for audio cues since we can simply implement an
  //        image provider for that case.
  bool needsSpecialHandling() const;

  QString metaInformation() const;

  const K3bDevice::Toc& toc() const;
  const K3bDevice::CdText& cdText() const;

  K3bImageSource* createImageSource( K3bJobHandler*, QObject* parent = 0 ) const;

 private:
  K3bCueFileParser m_parser;
  bool m_bOpen;
};

#endif
