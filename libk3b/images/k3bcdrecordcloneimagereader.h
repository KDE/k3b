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


#ifndef _K3B_CDRECORDCLONEIMAGE_READER_H_
#define _K3B_CDRECORDCLONEIMAGE_READER_H_

#include "k3bimagereaderbase.h"

#include <k3btoc.h>

#include <k3b_export.h>


class LIBK3B_EXPORT K3bCdrecordCloneImageReader : public K3bImageReaderBase
{
public:
  K3bCdrecordCloneImageReader();
  K3bCdrecordCloneImageReader( const QString& );
  ~K3bCdrecordCloneImageReader();

  bool open( const QString& file );
  void close();
  bool isOpen() const;

  QString imageType() const;
  QString imageTypeComment() const;

  bool needsSpecialHandling() const { return true; }

  QString metaInformation() const;

  const K3bDevice::Toc& toc() const { return m_toc; }

 private:
  K3bDevice::Toc m_toc;
  bool m_bOpen;
};

#endif

