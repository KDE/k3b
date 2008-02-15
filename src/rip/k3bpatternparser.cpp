/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2004-2005 Jakob Petsovits <jpetso@gmx.at>
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


#include "k3bpatternparser.h"

#include <qregexp.h>
#include <qdatetime.h>
#include <q3valuestack.h>

#include <kglobal.h>
#include <klocale.h>


QString K3bPatternParser::parsePattern( const K3bCddbResultEntry& entry,
                                        int trackNumber,
                                        const QString& pattern,
                                        bool replace,
                                        const QString& replaceString )
{
  if( entry.titles.count() < trackNumber )
    return "";

  QString dir, s;
  char c = ' ';     // contains the character representation of a special string
  unsigned int len; // length of the current special string


  for( int i = 0; i < pattern.length(); ++i ) {

    if( pattern[i] == '%' ) {

      if( i + 1 < pattern.length() ) {
        len = 2;

        if( pattern[i+1] != '{' ) {  // strings like %a
          c = pattern[i+1].toLatin1();
        }
        else if( i + 3 >= pattern.length() ) {  // too short to contain a %{*} string
          c = ' ';
        }
        else {  // long enough to contain %{*}

          if( pattern[i+3] == '}' ) {  // strings like %{a}
            c = pattern[i+2].toLatin1();
            len = 4;
          }
          else {  // strings like %{artist}, or anything like %{*

            while( i + len - 1 < pattern.length() ) {
              ++len;

              if( pattern[i + len - 1] == '%' ) {  // don't touch other special strings
                c = ' ';
                --len;
                break;
              }
              else if( pattern[i + len - 1] == '}' ) {
                s = pattern.mid( i + 2, len - 3 );

                if( s == "title" ) {
                  c = TITLE;
                }
                else if( s == "artist" ) {
                  c = ARTIST;
                }
                else if( s == "number" ) {
                  c = NUMBER;
                }
                else if( s == "comment" ) {
                  c = COMMENT;
                }
                else if( s == "year" ) {
                  c = YEAR;
                }
                else if( s == "genre" ) {
                  c = GENRE;
                }
                else if( s == "albumtitle" ) {
                  c = ALBUMTITLE;
                }
                else if( s == "albumartist" ) {
                  c = ALBUMARTIST;
                }
                else if( s == "albumcomment" ) {
                  c = ALBUMCOMMENT;
                }
                else if( s == "date" ) {
                  c = DATE;
                }
                else {  // no valid pattern in here, don't replace anything
                  c = ' ';
                }
                break; // finished parsing %{* string
              }
            } // end of while(...)

          } // end of %{* strings

        } // end of if( long enough to contain %{*} )

        switch( c ) {
          case ARTIST:
            s = entry.artists[trackNumber-1];
            s.replace( '/', '_' );
            s.replace( '*', '_' );
            s.replace( '}', '*' );  // for conditional inclusion
            dir.append( s.isEmpty()
                ? i18n("unknown") + QString(" %1").arg(trackNumber)
                : s );
            break;
          case TITLE:
            s = entry.titles[trackNumber-1];
            s.replace( '/', '_' );
            s.replace( '*', '_' );
            s.replace( '}', '*' );
            dir.append( s.isEmpty()
                ? i18n("Track %1",trackNumber)
                : s );
            break;
          case NUMBER:
            dir.append( QString::number(trackNumber).rightJustified( 2, '0' ) );
            break;
          case YEAR:
            dir.append( QString::number( entry.year ) );
            break;
          case COMMENT:
            s = entry.extInfos[trackNumber-1];
            s.replace( '/', '_' );
            s.replace( '*', '_' );
            s.replace( '}', '*' );
            dir.append( s );
            break;
          case GENRE:
            s = ( entry.genre.isEmpty() ? entry.category : entry.genre );
            s.replace( '/', '_' );
            s.replace( '*', '_' );
            s.replace( '}', '*' );
            dir.append( s );
            break;
          case ALBUMARTIST:
            dir.append( entry.cdArtist.isEmpty()
                ? i18n("unknown") : entry.cdArtist );
            break;
          case ALBUMTITLE:
            s = entry.cdTitle;
            s.replace( '/', '_' );
            s.replace( '*', '_' );
            s.replace( '}', '*' );
            dir.append( s.isEmpty()
                ? i18n("unknown") : s );
            break;
          case ALBUMCOMMENT:
            s = entry.cdExtInfo;
            s.replace( '/', '_' );
            s.replace( '*', '_' );
            s.replace( '}', '*' );
            dir.append( s ); // I think it makes more sense to allow empty comments
            break;
          case DATE:
            dir.append( KGlobal::locale()->formatDate( QDate::currentDate() ) );
            break;
          default:
            dir.append( pattern.mid(i, len) );
            break;
        }
        i += len - 1;
      }
      else {  // end of pattern
        dir.append( "%" );
      }
    }
    else {
      dir.append( pattern[i] );
    }
  }



  // /* delete line comment to comment out
  // the following part: Conditional Inclusion

  Q3ValueStack<int> offsetStack;
  QString inclusion;
  bool isIncluded;

  static QRegExp conditionrx( "^[@|!][atyegrmx](?:='.*')?\\{" );
  conditionrx.setMinimal( TRUE );

  for( int i = 0; i < dir.length(); ++i ) {

    offsetStack.push(
      conditionrx.search(dir, i, QRegExp::CaretAtOffset) );

    if( offsetStack.top() == -1 ) {
      offsetStack.pop();
    }
    else {
      i += conditionrx.matchedLength() - 1;
      continue;
    }

    if( dir[i] == '}' && !offsetStack.isEmpty() ) {

      int offset = offsetStack.pop();
      int length = i - offset + 1;

      switch( dir[offset+1].unicode() ) {
      case ARTIST:
        s = entry.artists[trackNumber-1];
        break;
      case TITLE:
        s = entry.titles[trackNumber-1];
        break;
      case NUMBER:
        s = QString::number( trackNumber );
        break;
      case YEAR:
        s = QString::number( entry.year );
        break;
      case COMMENT:
        s = entry.extInfos[trackNumber-1];
        break;
      case GENRE:
        s = ( entry.genre.isEmpty() ? entry.category : entry.genre );
        break;
      case ALBUMARTIST:
        s = entry.cdArtist;
        break;
      case ALBUMTITLE:
        s = entry.cdTitle;
        break;
      case ALBUMCOMMENT:
        s = entry.cdExtInfo;
        break;
      case DATE:
        s = KGlobal::locale()->formatDate( QDate::currentDate() );
        break;
      default: // we must never get here,
        break; // all choices should be covered
      }

      if( dir[offset+2] == '{' ) { // no string matching, e.g. ?y{text}
        switch( dir[offset+1].unicode() ) {
        case YEAR:
          isIncluded = (s != "0");
          break;
        default:
          isIncluded = !s.isEmpty();
          break;
        }
        inclusion = dir.mid( offset + 3, length - 4 );
      }
      else { // with string matching, e.g. ?y='2004'{text}

	// Be aware that there might be ' in the condition text
        int endOfCondition = dir.find( '{', offset+4 )-1;
        QString condition = dir.mid( offset+4,
				     endOfCondition - (offset+4) );

        isIncluded = (s == condition);
        inclusion = dir.mid( endOfCondition+2,
                             i - (endOfCondition+2) );
      }

      if( dir[offset] == '!' )
          isIncluded = !isIncluded;
      // Leave it when it's '@'.

      dir.replace( offset, length, ( isIncluded ? inclusion : QString("") ) );

      if( isIncluded == TRUE )
        i -= length - inclusion.length();
      else
        i = offset - 1; // start next loop at offset

      continue;

    } // end of replace (at closing bracket '}')

  } // end of conditional inclusion for(...)

  // end of Conditional Inclusion */


  dir.replace( '*', '}' );  // bring the brackets back, if there were any

  if( replace )
    dir.replace( QRegExp( "\\s" ), replaceString );

  return dir;
}
