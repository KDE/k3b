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

#ifndef _K3B_AUDIO_CONVERTER_JOB_H_
#define _K3B_AUDIO_CONVERTER_JOB_H_

#include <k3bthreadjob.h>

class QListView;
class K3bAudioEncoder;

class K3bAudioConverterJob : public K3bThreadJob
{
 public:
  K3bAudioConverterJob( QListView* view, 
			K3bAudioEncoder*,
			const QString& type,
			const QString& dest,
			K3bJobHandler*, 
			QObject* parent = 0, const char* name = 0 );
  ~K3bAudioConverterJob();

 private:
  class WorkThread;
  WorkThread* m_thread;
};

#endif
