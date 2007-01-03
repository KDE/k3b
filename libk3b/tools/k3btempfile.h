/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_TEMP_FILE_H_
#define _K3B_TEMP_FILE_H_

#include <ktempfile.h>

#include <k3b_export.h>

/**
 * K3bTempFile does only change one thing over KTempFile:
 * It tries to use a default temp dir which is always world-readable.
 *
 * This is important in the following situation:
 *
 * cdrecord often runs suid root. Some distributions like Mandriva
 * set the default KDE temp dir to $HOME/tmp-$USER. Thus, if the home
 * dir is mounted via NFS root has no read permissions htere and cdrecord
 * fails to read the temp files.
 */
class LIBK3B_EXPORT K3bTempFile : public KTempFile
{
 public:
  K3bTempFile( const QString& filePrefix = QString::null, 
	       const QString& fileExtension = QString::null, 
	       int mode = 0600 );
  ~K3bTempFile();
};

#endif
