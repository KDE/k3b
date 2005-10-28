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

#ifndef _K3B_CD_IMAGE_BURNER_H_
#define _K3B_CD_IMAGE_BURNER_H_

#include "k3bimageburner.h"

/**
 * The Power horse in K3b: the K3bCDImageBurner burns CD images of any kind provided
 * by a K3bImageSource.
 */
class K3bCDImageBurner : public K3bImageBurner
{
  Q_OBJECT

 public:
  K3bCDImageBurner( K3bJobHandler*, QObject* parent = 0 );
  ~K3bCDImageBurner();

 protected:
  void startInternal();
  void cancelInternal();

 private slots:
  void slotWriterJobFinished( bool );
  void slotWriterNextTrack( int, int );
  void slotWriterJobPercent( int p );

 private:
  void cleanup();
  void setupBurningParameters();
  void setupBurningJob();

  class Private;
  Private* d;
};

#endif
