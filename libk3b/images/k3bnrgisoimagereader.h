/* 
 *
 * $Id: k3bnrgfilereader.h 412090 2005-05-10 18:28:07Z trueg $
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

#ifndef _K3B_NRG_ISO_IMAGE_READER_H_
#define _K3B_NRG_ISO_IMAGE_READER_H_

#include "k3biso9660imagereader.h"

#include <qfile.h>

#include <k3b_export.h>

class K3bIso9660;


/**
 * This class detects simple ISO9660 Nrg images
 */
class LIBK3B_EXPORT K3bNrgIsoImageReader : public K3bIso9660ImageReader
{
 public:
  K3bNrgIsoImageReader();
  K3bNrgIsoImageReader( const QString& filename );
  ~K3bNrgIsoImageReader();

  bool open( const QString& file );
  void close();
  bool isOpen() const;

  QString imageType() const;
  QString imageTypeComment() const;

  QString metaInformation() const;

  K3bImageSource* createImageSource( K3bJobHandler*, QObject* parent = 0 ) const;
 
  K3bIso9660* iso9660() const { return m_iso; }

 private:
  K3bIso9660* m_iso;
  QFile m_file;
};

#endif
