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


#include "k3bpatternparser.h"

#include <qregexp.h>
#include <qdatetime.h>
#include <qvaluestack.h>

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

  QString dir, s;
  for( unsigned int i = 0; i < pattern.length(); ++i ) {

    if( pattern[i] == '%' ) {
      ++i;

      if( i < pattern.length() ) {
	switch( pattern[i] ) {
	case 'a':
	  s = entry.artists[trackNumber-1];
	  s.replace( '/', '_' );
	  dir.append( s.isEmpty() 
		      ? i18n("unknown") + QString(" %1").arg(trackNumber)
		      : s );
	  break;
	case 't':
	  s = entry.titles[trackNumber-1];
	  s.replace( '/', '_' );
	  dir.append( s.isEmpty() 
		      ? i18n("Track %1").arg(trackNumber) 
		      : s );
	  break;
	case 'n':
	  dir.append( QString::number(trackNumber).rightJustify( 2, '0' ) );
	  break;
	case 'y':
	  dir.append( QString::number( entry.year ) );
	  break;
	case 'e':
	  s = entry.extInfos[trackNumber-1];
	  s.replace( '/', '_' );
	  dir.append( s );
	  break;
	case 'g':
	  s = ( entry.genre.isEmpty() ? entry.category : entry.genre );
	  s.replace( '/', '_' );
	  dir.append( s );
	  break;
	case 'r':
	  dir.append( entry.cdArtist.isEmpty() 
		      ? i18n("unknown") : entry.cdArtist );
	  break;
	case 'm':
	  s = entry.cdTitle;
	  s.replace( '/', '_' );
	  dir.append( s.isEmpty() 
		      ? i18n("unknown") : s );
	  break;
	case 'x':
	  s = entry.cdExtInfo;
	  s.replace( '/', '_' );
	  dir.append( s ); // I think it makes more sense to allow empty extinfos
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



  // /* delete line comment to comment out the following part:
  // Conditional Inclusion (2004 by Jakob Petsovits)

  QValueStack<int> offsetStack;
  QString inclusion;
  bool isIncluded;

  QRegExp* conditionrx =
    new QRegExp( "^[@|!][atyegrmx](?:='.*')?\\{" );
  conditionrx->setMinimal( TRUE );

  for( unsigned int i = 0; i < dir.length(); ++i ) {

    offsetStack.push(
      conditionrx->search(dir, i, QRegExp::CaretAtOffset) );

    if( offsetStack.top() == -1 ) {
      offsetStack.pop();
    }
    else {
      i += conditionrx->matchedLength() - 1;
      continue;
    }

    if( dir[i] == '}' and !offsetStack.isEmpty() ) {

      int offset = offsetStack.pop();
      int length = i - offset + 1;

      switch( (QChar) dir[offset+1] ) {
      case 'a':
        s = entry.artists[trackNumber-1];
        break;
      case 't':
        s = entry.titles[trackNumber-1];
        break;
      case 'n':
        s = QString::number( trackNumber );
        break;
      case 'y':
        s = QString::number( entry.year );
        break;
      case 'e':
        s = entry.extInfos[trackNumber-1];
        break;
      case 'g':
        s = ( entry.genre.isEmpty() ? entry.category : entry.genre );
        break;
      case 'r':
        s = entry.cdArtist;
        break;
      case 'm':
        s = entry.cdTitle;
        break;
      case 'x':
        s = entry.cdExtInfo;
        break;
      case 'd':
        s = KGlobal::locale()->formatDate( QDate::currentDate() );
        break;
      default: // we must never get here,
        break; // all choices should be covered
      }

      if( dir[offset+2] == '{' ) { // no string matching, e.g. ?y{text}
        switch( (QChar) dir[offset+1] ) {
        case 'y':
          isIncluded = (s != "0");
          break;
        default:
          isIncluded = !s.isEmpty();
          break;
        }
        inclusion = dir.mid( offset + 3, length - 4 );
      }
      else { // with string matching, e.g. ?y='2004'{text}

        int endOfCondition = dir.find( '\'', offset+4 );
        QString condition = dir.mid( offset+4,
                        endOfCondition - (offset+4) );

        isIncluded = (s == condition);
        inclusion = dir.mid( endOfCondition+2,
                            i - (endOfCondition+2) );
      }

      if( dir[offset] == '!' )
          isIncluded = !isIncluded;
      // Leave it when it's '@'.

      dir.replace( offset, length, ( isIncluded ? inclusion : "" ) );

      if( isIncluded == TRUE )
        i -= length - inclusion.length();
      else
        i = offset - 1; // start next loop at offset

      continue;

    } // end of replace (at closing bracket '}')

  } // end of conditional inclusion for(...)

  delete conditionrx;

  // end of Conditional Inclusion */


  return dir;
}
