/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_GROWISOFS_IMAGER_H_
#define _K3B_GROWISOFS_IMAGER_H_


#include <k3bisoimager.h>



class K3bDvdDoc;
namespace K3bCdDevice {
  class DeviceHandler;
  class CdDevice;
}
class K3bExternalBin;


/**
 * This class is an imager and a writer. growisofs calls mkisofs
 * when writing multisession or growing in a single session.
 * In an ideal situation K3b would create the image and tell
 * growisofs that it's a multisession one.
 * But this is frontending and not a perfect situation ;)
 */
class K3bGrowisofsImager : public K3bIsoImager
{
  Q_OBJECT

 public:
  K3bGrowisofsImager( K3bDvdDoc* doc, QObject* parent = 0, const char* name = 0 );
  virtual ~K3bGrowisofsImager();

 public slots:
  virtual void start();
  virtual void cancel();

 protected slots:
  virtual void slotReceivedStderr( const QString& );
  virtual void slotProcessExited( KProcess* );
  void slotEjectingFinished( K3bCdDevice::DeviceHandler* );

 private:
  const K3bExternalBin* m_growisofsBin;
  const K3bExternalBin* m_mkisofsBin;

  K3bDvdDoc* m_doc;

  class Private;
  Private* d;
};

#endif
