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

#include "k3btempfile.h"

#include <kglobal.h>
#include <kstandarddirs.h>


static inline QString defaultTempDir()
{
  // we need a world-readable temp dir
  
  // FIXME: check if the default is world-readable
//   QStringList dirs = KGlobal::dirs()->resourceDirs( "tmp" );
//   for( QStringList::const_iterator it = dirs.begin();
//        it != dirs.end(); ++it ) {
//     const QString& dir = *it;
    
//   }

  // fallback to /tmp
  return "/tmp/k3b";
}


K3bTempFile::K3bTempFile( const QString& filePrefix, 
			  const QString& fileExtension, 
			  int mode )
  : KTempFile( filePrefix.isEmpty() ? defaultTempDir() : filePrefix,
	       fileExtension,
	       mode )
{
}


K3bTempFile::~K3bTempFile()
{
}
