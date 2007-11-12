/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#include <config.h>

#ifdef HAVE_MUSICBRAINZ

#include "k3bmusicbrainz.h"

#include <musicbrainz/mb_c.h>

#include <kprotocolmanager.h>
#include <kurl.h>
#include <kdebug.h>
//Added by qt3to4:
#include <Q3CString>


class K3bMusicBrainz::Private
{
public:
  musicbrainz_t mb;

  QStringList titles;
  QStringList artists;
};


K3bMusicBrainz::K3bMusicBrainz()
{
  d = new Private;
  d->mb = mb_New();
  mb_UseUTF8( d->mb, 1 );
}


K3bMusicBrainz::~K3bMusicBrainz()
{
  mb_Delete( d->mb );
  delete d;
}


int K3bMusicBrainz::query( const Q3CString& trm )
{
  d->titles.clear();
  d->artists.clear();

  if( KProtocolManager::useProxy() ) {
    KURL proxy = KProtocolManager::proxyFor("http");
    mb_SetProxy( d->mb, const_cast<char*>(proxy.host().latin1()), short(proxy.port()) );
  }

  char* args[2];
  args[0] = trm.data();
  args[1] = 0;

  if( mb_QueryWithArgs( d->mb, (char*)MBQ_TrackInfoFromTRMId, (char**)args ) ) {
    
    unsigned int i = 1;
    while( mb_Select(d->mb, (char*)MBS_Rewind) && mb_Select1( d->mb, (char*)MBS_SelectTrack, i ) ) {
      Q3CString data(256);
      mb_GetResultData( d->mb, (char*)MBE_TrackGetArtistName, data.data(), 256 );
      d->artists.append( QString::fromUtf8( data ).stripWhiteSpace() );
      mb_GetResultData( d->mb, (char*)MBE_TrackGetTrackName, data.data(), 256 );
      d->titles.append( QString::fromUtf8( data ).stripWhiteSpace() );

      ++i;
    }

    return i-1;
  }
  else {
    char buffer[256];
    mb_GetQueryError( d->mb, buffer, 256 );
    kdDebug() << "(K3bMusicBrainz) query error: " << buffer << endl;
    return 0;
  }
}


const QString& K3bMusicBrainz::title( unsigned int i ) const
{
  return d->titles[i];
}


const QString& K3bMusicBrainz::artist( unsigned int i ) const
{
  return d->artists[i];
}

#endif
