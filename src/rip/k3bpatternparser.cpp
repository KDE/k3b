/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2004-2005 Jakob Petsovits <jpetso@gmx.at>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bpatternparser.h"

#include <KCddb/Cdinfo>

#include <KLocalizedString>

#include <QDateTime>
#include <QLocale>
#include <QRegExp>
#include <QStack>
#include <QDebug>

QString K3b::PatternParser::parsePattern( const KCDDB::CDInfo& entry,
                                          int trackNumber,
                                          const QString& extension,
                                          const QString& pattern,
                                          bool replace,
                                          const QString& replaceString )
{
    QString dir, s;
    char c = ' ';     // contains the character representation of a special string
    int len;          // length of the current special string


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
                                else if( s == "ext" ) {
                                    c = EXTENSION;
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
                    s = entry.track( trackNumber-1 ).get( KCDDB::Artist ).toString();
                    s.replace( '/', '_' );
                    s.replace( '*', '_' );
                    s.replace( '}', '*' );  // for conditional inclusion
                    dir.append( s.isEmpty()
                                ? i18n("unknown") + QString(" %1").arg(trackNumber)
                                : s );
                    break;
                case TITLE:
                    s = entry.track( trackNumber-1 ).get( KCDDB::Title ).toString();
                    s = s.trimmed(); // Remove whitespace from start and the end.
#ifdef K3B_DEBUG
                    qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << s;
#endif
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
                    dir.append( QString::number( entry.get( KCDDB::Year ).toInt() ) );
                    break;
                case COMMENT:
                    s = entry.track( trackNumber-1 ).get( KCDDB::Comment ).toString();
                    s.replace( '/', '_' );
                    s.replace( '*', '_' );
                    s.replace( '}', '*' );
                    dir.append( s );
                    break;
                case GENRE:
                    s = entry.get( KCDDB::Genre ).toString();
                    if ( s.isEmpty() )
                        s = entry.get( KCDDB::Category ).toString();
                    s.replace( '/', '_' );
                    s.replace( '*', '_' );
                    s.replace( '}', '*' );
                    dir.append( s );
                    break;
                case ALBUMARTIST:
                    s = entry.get( KCDDB::Artist ).toString();
                    dir.append( s.isEmpty() ? i18n("unknown") : s );
                    break;
                case ALBUMTITLE:
                    s = entry.get( KCDDB::Title ).toString();
                    s.replace( '/', '_' );
                    s.replace( '*', '_' );
                    s.replace( '}', '*' );
                    dir.append( s.isEmpty()
                                ? i18n("unknown") : s );
                    break;
                case ALBUMCOMMENT:
                    s = entry.get( KCDDB::Comment ).toString();
                    s.replace( '/', '_' );
                    s.replace( '*', '_' );
                    s.replace( '}', '*' );
                    dir.append( s ); // I think it makes more sense to allow empty comments
                    break;
                case DATE:
                    dir.append( QLocale().toString( QDate::currentDate() ) );
                    break;
                case EXTENSION:
                    dir.append( extension );
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

    QStack<int> offsetStack;
    QString inclusion;
    bool isIncluded;

    static QRegExp conditionrx( "^[@|!][atyegrmx](?:='.*')?\\{" );
    conditionrx.setMinimal( true );

    for( int i = 0; i < dir.length(); ++i ) {

        offsetStack.push(
            conditionrx.indexIn(dir, i, QRegExp::CaretAtOffset) );

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
                s = entry.track( trackNumber-1 ).get( KCDDB::Artist ).toString();
                break;
            case TITLE:
                s = entry.track( trackNumber-1 ).get( KCDDB::Title ).toString();
                break;
            case NUMBER:
                s = QString::number( trackNumber );
                break;
            case YEAR:
                s = QString::number( entry.get( KCDDB::Year ).toInt() );
                break;
            case COMMENT:
                s = entry.track( trackNumber-1 ).get( KCDDB::Comment ).toString();
                break;
            case GENRE:
                s = entry.get( KCDDB::Genre ).toString();
                if ( s.isEmpty() )
                    s = entry.get( KCDDB::Category ).toString();
                break;
            case ALBUMARTIST:
                s = entry.get( KCDDB::Artist ).toString();
                break;
            case ALBUMTITLE:
                s = entry.get( KCDDB::Title ).toString();
                break;
            case ALBUMCOMMENT:
                s = entry.get( KCDDB::Comment ).toString();
                break;
            case DATE:
                s = QLocale().toString( QDate::currentDate() );
                break;
            case EXTENSION:
                s = extension;
                break;
            default: // we must never get here,
                break; // all choices should be covered
            }

            s.replace( '/', '_' );
            s.replace( '*', '_' );
            s.replace( '}', '*' );

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
                int endOfCondition = dir.indexOf( '{', offset+4 )-1;
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

            if( isIncluded == true )
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

    if ( !dir.endsWith( '.' + extension ) )
        dir.append( '.' + extension );

    return dir;
}
