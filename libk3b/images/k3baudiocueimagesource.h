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


#ifndef _K3B_AUDIOCUEIMAGESOURCE_H_
#define _K3B_AUDIOCUEIMAGESOURCE_H_

#include <k3bsimpleimagesource.h>


class K3bAudioCueImageSource : public K3bSimpleImageSource
{
  Q_OBJECT

 public:
  K3bAudioCueImageSource( K3bJobHandler* hdl, QObject* parent );
  K3bAudioCueImageSource( const QString& cuefile, K3bJobHandler* hdl, QObject* parent );
  ~K3bAudioCueImageSource();

  K3b::SectorSize sectorSize( unsigned int ) const { return K3b::SECTORSIZE_AUDIO; }

 public slots:
  void determineToc();
  void cancel();

  void setCueFile( const QString& );

 private slots:
  void slotAnalyserThreadFinished( bool );

 protected:
  bool init();
  long simpleRead(char* data, long maxLen);

 private:
  class AnalyserThread;

  class Private;
  Private* d;
};

#endif
