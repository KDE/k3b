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


#ifndef _K3B_AUDIO_NORMALIZE_JOB_H_
#define _K3B_AUDIO_NORMALIZE_JOB_H_


#include <k3bjob.h>

#include <qvaluevector.h>

class K3bProcess;
class KProcess;


class K3bAudioNormalizeJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bAudioNormalizeJob( QObject* parent = 0, const char* name = 0 );
  ~K3bAudioNormalizeJob();

 public slots:
  void start();
  void cancel();

  void setFilesToNormalize( const QValueVector<QString>& files ) { m_files = files; }

 private slots:
  void slotStdLine( const QString& line );
  void slotProcessExited( KProcess* p );

 private:
  K3bProcess* m_process;

  QValueVector<QString> m_files;
  bool m_canceled;

  enum Action {
    COMPUTING_LEVELS,
    ADJUSTING_LEVELS
  };

  int m_currentAction;
  int m_currentTrack;
};


#endif
