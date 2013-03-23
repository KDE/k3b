/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bversion.h"

#include <qregexp.h>
#include <QtCore/QSharedData>

#include <kdebug.h>

namespace {
    /**
     * splits the leading number from s and puts it in num
     * the dot is removed and the rest put in suffix
     * if s does not start with a digit or the first non-digit char is not a dot
     * suffix = s and num = -1 is returned
     */
    void splitVersionString( const QString& s, int& num, QString& suffix )
    {
        int pos = s.indexOf( QRegExp("\\D") );
        if( pos < 0 ) {
            num = s.toInt();
            suffix = "";
        }
        else if( pos == 0 ) {
            num = -1;
            suffix = s;
        }
        else {
            num = s.left( pos ).toInt();
            suffix = s.mid( pos );
        }
    }
}


class K3b::Version::Private : public QSharedData
{
public:
    Private()
        : m_majorVersion( -1 ),
          m_minorVersion( -1 ),
          m_patchLevel( -1 ) {
    }

    QString m_versionString;
    int m_majorVersion;
    int m_minorVersion;
    int m_patchLevel;
    QString m_suffix;
};



K3b::Version::Version()
    :d( new Private() )
{
}


K3b::Version::Version( const K3b::Version& v )
{
    d = v.d;
}


K3b::Version::Version( const QString& version )
    :d( new Private() )
{
    setVersion( version );
}


K3b::Version::Version( int majorVersion,
                       int minorVersion,
                       int patchlevel,
                       const QString& suffix )
    :d( new Private() )
{
    setVersion( majorVersion, minorVersion, patchlevel, suffix );
}


K3b::Version::~Version()
{
}


K3b::Version& K3b::Version::operator=( const Version& other )
{
    d = other.d;
    return *this;
}


QString K3b::Version::toString() const { return d->m_versionString; }
QString K3b::Version::versionString() const { return d->m_versionString; }
int K3b::Version::majorVersion() const { return d->m_majorVersion; }
int K3b::Version::minorVersion() const { return d->m_minorVersion; }
int K3b::Version::patchLevel() const { return d->m_patchLevel; }
QString K3b::Version::suffix() const { return d->m_suffix; }


void K3b::Version::setVersion( const QString& v )
{
    QString suffix;
    splitVersionString( v.trimmed(), d->m_majorVersion, suffix );
    if( d->m_majorVersion >= 0 ) {
        if( suffix.startsWith('.') ) {
            suffix = suffix.mid( 1 );
            splitVersionString( suffix, d->m_minorVersion, suffix );
            if( d->m_minorVersion < 0 ) {
                kDebug() << "(K3b::Version) suffix must not start with a dot!";
                d->m_majorVersion = -1;
                d->m_minorVersion = -1;
                d->m_patchLevel = -1;
                d->m_suffix = "";
            }
            else {
                if( suffix.startsWith('.') ) {
                    suffix = suffix.mid( 1 );
                    splitVersionString( suffix, d->m_patchLevel, suffix );
                    if( d->m_patchLevel < 0 ) {
                        kDebug() << "(K3b::Version) suffix must not start with a dot!";
                        d->m_majorVersion = -1;
                        d->m_minorVersion = -1;
                        d->m_patchLevel = -1;
                        d->m_suffix = "";
                    }
                    else {
                        d->m_suffix = suffix;
                    }
                }
                else {
                    d->m_patchLevel = -1;
                    d->m_suffix = suffix;
                }
            }
        }
        else {
            d->m_minorVersion = -1;
            d->m_patchLevel = -1;
            d->m_suffix = suffix;
        }
    }

    d->m_versionString = createVersionString( d->m_majorVersion, d->m_minorVersion, d->m_patchLevel, d->m_suffix );
}


bool K3b::Version::isValid() const
{
    return (d->m_majorVersion >= 0);
}


void K3b::Version::setVersion( int majorVersion,
                               int minorVersion,
                               int patchlevel,
                               const QString& suffix )
{
    d->m_majorVersion = majorVersion;
    d->m_minorVersion = minorVersion;
    d->m_patchLevel = patchlevel;
    d->m_suffix = suffix;
    d->m_versionString = createVersionString( majorVersion, minorVersion, patchlevel, suffix );
}

K3b::Version& K3b::Version::operator=( const QString& v )
{
    setVersion( v );
    return *this;
}

K3b::Version K3b::Version::simplify() const
{
    K3b::Version v( *this );
    v.d->m_suffix.truncate(0);
    return v;
}

QString K3b::Version::createVersionString( int majorVersion,
                                           int minorVersion,
                                           int patchlevel,
                                           const QString& suffix )
{
    if( majorVersion >= 0 ) {
        QString s = QString::number(majorVersion);

        if( minorVersion > -1 ) {
            s.append( QString(".%1").arg(minorVersion) );
            if( patchlevel > -1 )
                s.append( QString(".%1").arg(patchlevel) );
        }

        if( !suffix.isNull() )
            s.append( suffix );

        return s;
    }
    else
        return "";
}


int K3b::Version::compareSuffix( const QString& suffix1, const QString& suffix2 )
{
    static QRegExp rcRx( "rc(\\d+)" );
    static QRegExp preRx( "pre(\\d+)" );
    static QRegExp betaRx( "beta(\\d+)" );
    static QRegExp alphaRx( "a(?:lpha)?(\\d+)" );

    // first we check if one of the suffixes (or both are empty) becasue that case if simple
    if( suffix1.isEmpty() ) {
        if( suffix2.isEmpty() )
            return 0;
        else
            return 1; // empty greater than the non-empty (should we treat something like 1.0a as greater than 1.0?)
    }
    else if( suffix2.isEmpty() )
        return -1;

    // now search for our special suffixes
    if( rcRx.exactMatch( suffix1 ) ) {
        int v1 = rcRx.cap(1).toInt();

        if( rcRx.exactMatch( suffix2 ) ) {
            int v2 = rcRx.cap(1).toInt();
            return ( v1 == v2 ? 0 : ( v1 < v2 ? -1 : 1 ) );
        }
        else if( preRx.exactMatch( suffix2 ) ||
                 betaRx.exactMatch( suffix2 ) ||
                 alphaRx.exactMatch( suffix2 ) )
            return 1; // rc > than all the others
        else
            return QString::compare( suffix1, suffix2 );
    }

    else if( preRx.exactMatch( suffix1 ) ) {
        int v1 = preRx.cap(1).toInt();

        if( rcRx.exactMatch( suffix2 ) ) {
            return -1; // pre is less than rc
        }
        else if( preRx.exactMatch( suffix2 ) ) {
            int v2 = preRx.cap(1).toInt();
            return ( v1 == v2 ? 0 : ( v1 < v2 ? -1 : 1 ) );
        }
        else if( betaRx.exactMatch( suffix2 ) ||
                 alphaRx.exactMatch( suffix2 ) )
            return 1; // pre is greater than beta or alpha
        else
            return QString::compare( suffix1, suffix2 );
    }

    else if( betaRx.exactMatch( suffix1 ) ) {
        int v1 = betaRx.cap(1).toInt();

        if( rcRx.exactMatch( suffix2 ) ||
            preRx.exactMatch( suffix2 ) )
            return -1; // beta is less than rc or pre
        else if( betaRx.exactMatch( suffix2 ) ) {
            int v2 = betaRx.cap(1).toInt();
            return ( v1 == v2 ? 0 : ( v1 < v2 ? -1 : 1 ) );
        }
        else if( alphaRx.exactMatch( suffix2 ) )
            return 1; // beta is greater then alpha
        else
            return QString::compare( suffix1, suffix2 );
    }

    else if( alphaRx.exactMatch( suffix1 ) ) {
        int v1 = alphaRx.cap(1).toInt();

        if( rcRx.exactMatch( suffix2 ) ||
            preRx.exactMatch( suffix2 ) ||
            betaRx.exactMatch( suffix2 ) )
            return -1; // alpha is less than all the others
        else if( alphaRx.exactMatch( suffix2 ) ) {
            int v2 = alphaRx.cap(1).toInt();
            return ( v1 == v2 ? 0 : ( v1 < v2 ? -1 : 1 ) );
        }
        else
            return QString::compare( suffix1, suffix2 );
    }

    else
        return QString::compare( suffix1, suffix2 );
}


bool K3b::operator<( const K3b::Version& v1, const K3b::Version& v2 )
{
    // both version objects need to be valid

    if( v1.majorVersion() == v2.majorVersion() ) {

        // 1 == 1.0
        if( ( v1.minorVersion() == v2.minorVersion() )
            ||
            ( v1.minorVersion() == -1 && v2.minorVersion() == 0 )
            ||
            ( v2.minorVersion() == -1 && v1.minorVersion() == 0 )
            )
        {
            // 1.0 == 1.0.0
            if( ( v1.patchLevel() == v2.patchLevel() )
                ||
                ( v1.patchLevel() == -1 && v2.patchLevel() == 0 )
                ||
                ( v2.patchLevel() == -1 && v1.patchLevel() == 0 )
                )
            {
                return K3b::Version::compareSuffix( v1.suffix(), v2.suffix() ) < 0;
            }
            else
                return ( v1.patchLevel() < v2.patchLevel() );
        }
        else
            return ( v1.minorVersion() < v2.minorVersion() );
    }
    else
        return ( v1.majorVersion() < v2.majorVersion() );
}


bool K3b::operator>( const K3b::Version& v1, const K3b::Version& v2 )
{
    return operator<( v2, v1 );
}


bool K3b::operator==( const K3b::Version& v1, const K3b::Version& v2 )
{
    return ( v1.majorVersion() == v2.majorVersion() &&
             v1.minorVersion() == v2.minorVersion() &&
             v1.patchLevel() == v2.patchLevel() &&
             K3b::Version::compareSuffix( v1.suffix(), v2.suffix() ) == 0 );
}


bool K3b::operator<=( const K3b::Version& v1, const K3b::Version& v2 )
{
    return ( operator<( v1, v2 ) || operator==( v1, v2 ) );
}


bool K3b::operator>=( const K3b::Version& v1, const K3b::Version& v2 )
{
    return ( operator>( v1, v2 ) || operator==( v1, v2 ) );
}
