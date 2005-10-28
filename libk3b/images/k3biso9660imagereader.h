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


#ifndef _K3B_ISO9660IMAGE_READER_H_
#define _K3B_ISO9660IMAGE_READER_H_

#include <qstring.h>

#include "k3bimagereaderbase.h"

#include <k3b_export.h>


class K3bIso9660;

class LIBK3B_EXPORT K3bIso9660ImageReader : public K3bImageReaderBase
{
 public:
  K3bIso9660ImageReader();
  K3bIso9660ImageReader( const QString& );
  virtual ~K3bIso9660ImageReader();
  
  virtual bool open( const QString& file );
  virtual void close();
  virtual bool isOpen() const;

  virtual QString imageType() const;
  virtual QString imageTypeComment() const;

  virtual int mediaType() const;

  virtual QString metaInformation() const;

  /**
   * \return the used K3bIso9660 instance.
   */
  virtual K3bIso9660* iso9660() const { return m_iso; }

  virtual K3bImageSource* createImageSource( K3bJobHandler*, QObject* parent = 0 ) const;

 protected:
  QString createIso9660MetaInformation( K3bIso9660* ) const;

 private:
  K3bIso9660* m_iso;
};

#endif

