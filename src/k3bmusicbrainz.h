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

#ifndef _K3B_MUSICBRAINZ_H_
#define _K3B_MUSICBRAINZ_H_

#include <config.h>

#ifdef HAVE_MUSICBRAINZ

#include <qcstring.h>
#include <qstring.h>


/**
 * A wrapper class around libmusicbrainz. Use in combination with K3bTRM.
 *
 * Tries to determine the artist and title related to a trm.
 */
class K3bMusicBrainz
{
 public:
  K3bMusicBrainz();
  ~K3bMusicBrainz();

  /**
   * \return number of found results.
   */
  int query( const QCString& trm );

  const QString& title( unsigned int i = 0 ) const;
  const QString& artist( unsigned int i = 0 ) const;

 private:
  class Private;
  Private* d;
};


#endif

#endif
