/***************************************************************************
                          k3bpatternparser.cpp  -  description
                             -------------------
    begin                : Sun Dec 2 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
	  dir.append( entry.artists[trackNumber-1] );
	  break;
	case 't':
	  dir.append( entry.titles[trackNumber-1] );
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
	  dir.append( entry.cdArtist );
	  break;
	case 'm':
	  dir.append( entry.cdTitle );
	  break;
	case 'x':
	  dir.append( entry.cdExtInfo );
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
