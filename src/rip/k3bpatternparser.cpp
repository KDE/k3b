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


#include "k3bpatternparser.h"

#include <qregexp.h>
#include <qdatetime.h>

#include <kglobal.h>
#include <klocale.h>


QString K3bPatternParser::parsePattern( const K3bCddbResultEntry& entry, 
					unsigned int trackNumber,
					const QString& pattern, 
					bool replace, 
					const QString& replaceString )
{
  if( entry.titles.count() < trackNumber )
    return "";

  QString dir;
  for( unsigned int i = 0; i < pattern.length(); ++i ) {

    if( pattern[i] == '%' ) {
      ++i;

      if( i < pattern.length() ) {
	switch( pattern[i] ) {
	case 'a':
	  dir.append( entry.artists[trackNumber-1].isEmpty() ? i18n("unknown") : entry.artists[trackNumber-1] );
	  break;
	case 't':
	  dir.append( entry.titles[trackNumber-1].isEmpty() ? i18n("unknown") : entry.titles[trackNumber-1] );
	  break;
	case 'n':
	  dir.append( QString::number(trackNumber).rightJustify( 2, '0' ) );
	  break;
	case 'e':
	  dir.append( entry.extInfos[trackNumber-1] );
	  break;
	case 'g':
	  dir.append( entry.genre.isEmpty() ? entry.category : entry.genre );
	  break;
	case 'r':
	  dir.append( entry.cdArtist.isEmpty() ? i18n("unknown") : entry.cdArtist );
	  break;
	case 'm':
	  dir.append( entry.cdTitle.isEmpty() ? i18n("unknown") : entry.cdTitle );
	  break;
	case 'x':
	  dir.append( entry.cdExtInfo ); // I think it makes more sense to allow empty extinfos
	  break;
	case 'd':
	  dir.append( KGlobal::locale()->formatDate( QDate::currentDate() ) );
	  break;
	default:
	  dir.append( "%" );
	  dir.append( pattern[i] );
	  break;
	}
      }
      else {  // end of pattern
	dir.append( "%" );
      }
    }
    else {
      dir.append( pattern[i] );
    }
  }

  if( replace )
    dir.replace( QRegExp( "\\s" ), replaceString );

  return dir;
}
