/* 
 *
 * $Id$
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

#ifndef _K3B_AUDIO_CUE_FILEWRITING_JOB_H_
#define _K3B_AUDIO_CUE_FILEWRITING_JOB_H_

#include <k3bjob.h>

class K3bAudioDoc;
class K3bAudioJob;
class K3bAudioDecoder;
class K3bThreadJob;
namespace K3bDevice {
  class Device;
}


class K3bAudioCueFileWritingJob : public K3bBurnJob
{
  Q_OBJECT

 public:
  K3bAudioCueFileWritingJob( K3bJobHandler*, QObject* parent = 0, const char* name = 0 );
  ~K3bAudioCueFileWritingJob();

  K3bDevice::Device* writer() const;
	
  QString jobDescription() const;
  QString jobDetails() const;

  const QString& cueFile() const { return m_cueFile; }

 public slots:
  void start();
  void cancel();

  void setCueFile( const QString& );
  void setSpeed( int s );
  void setBurnDevice( K3bDevice::Device* dev );
  void setWritingMode( int mode );
  void setSimulate( bool b );
  void setCopies( int c );
  void setOnTheFly( bool b );
  void setTempDir( const QString& );

 private slots:
  void slotAnalyserThreadFinished(bool);

 private:
  void importCueInProject();

  K3bDevice::Device* m_device;

  QString m_cueFile;
  K3bAudioDoc* m_audioDoc;
  K3bAudioJob* m_audioJob;
  K3bAudioDecoder* m_decoder;

  bool m_canceled;
  bool m_audioJobRunning;

  class AnalyserThread;
  AnalyserThread* m_analyserThread;
  K3bThreadJob* m_analyserJob;
};

#endif
