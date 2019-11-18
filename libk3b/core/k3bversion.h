/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_VERSION_H_
#define _K3B_VERSION_H_

#include "k3b_export.h"

#include <QSharedDataPointer>
#include <QString>

namespace K3b {
    /**
     * \brief Representation of a version.
     *
     * Version represents a version consisting of a major version (accessible via majorVersion()),
     * a minor version (accessible via minorVersion()), a patchLevel (accessible via patchLevel()),
     * and a suffix (accessible via suffix()).
     *
     * The major version is mandatory while all other fields are optional (in case of the minor version
     * and the patchlevel -1 means that the field is undefined).
     *
     * Version tries to treat version suffixes in an "intelligent" way to properly compare versions
     * (see compareSuffix() for more details).
     *
     * Version may also be used everywhere a QString is needed as it automatically converts to a
     * string representation using createVersionString().
     */
    class LIBK3B_EXPORT Version
    {
    public:
        /**
         * construct an empty version object
         * which is invalid
         * @ see isValid()
         */
        Version();

        /**
         * copy constructor
         */
        Version( const Version& );

        /**
         * this constructor tries to parse the given version string
         */
        Version( const QString& version );

        /**
         * sets the version and generates a version string from it
         */
        Version( int majorVersion, int minorVersion, int pachlevel = -1, const QString& suffix = QString() );

        /**
         * Destructor
         */
        ~Version();

        /**
         * Copy operator
         */
        Version& operator=( const Version& );

        Version& operator=( const QString& v );

        /**
         * tries to parse the version string
         * used by the constructor
         */
        void setVersion( const QString& );

        bool isValid() const;

        /**
         * sets the version and generates a version string from it
         * used by the constructor
         *
         * If minorVersion or pachlevel are -1 they will not be used when generating the version string.
         */
        void setVersion( int majorVersion, int minorVersion = -1, int patchlevel = -1, const QString& suffix = QString() );

        QString toString() const;
        QString versionString() const;
        int majorVersion() const;
        int minorVersion() const;
        int patchLevel() const;
        QString suffix() const;

        /**
         * just to make it possible to use as a QString
         */
        operator QString() const { return toString(); }

        /**
         * \return A new Version object which equals this one except that the suffix is empty.
         */
        Version simplify() const;

        /**
         * If minorVersion or pachlevel are -1 they will not be used when generating the version string.
         * If minorVersion is -1 patchlevel will be ignored.
         */
        static QString createVersionString( int majorVersion,
                                            int minorVersion = -1,
                                            int patchlevel = -1,
                                            const QString& suffix = QString() );

        /**
         * "Intelligent" comparison of two version suffixes.
         *
         * This method checks for the following types of suffixes and treats them in the
         * following order:
         *
         * [empty prefix] > rcX > preX > betaX > alphaX = aX (where X is a number)
         *
         * Every other suffixes are compared alphanumerical.
         * An empty prefix is always considered newer than an unknown non-empty suffix (e.g. not one of the above.)
         *
         * @return \li -1 if suffix1 is less than suffix2
         *         \li 0 if suffix1 equals suffix2 (be aware that this is not the same as comparing to strings as
         *             alphaX equals aX in this case.)
         *         \li 1 if suffix1 is greater than suffix2
         */
        static int compareSuffix( const QString& suffix1, const QString& suffix2 );

    private:
        class Private;
        QSharedDataPointer<Private> d;
    };


    LIBK3B_EXPORT bool operator<( const Version& v1, const Version& v2 );
    LIBK3B_EXPORT bool operator>( const Version& v1, const Version& v2 );
    LIBK3B_EXPORT bool operator==( const Version& v1, const Version& v2 );
    LIBK3B_EXPORT bool operator<=( const Version& v1, const Version& v2 );
    LIBK3B_EXPORT bool operator>=( const Version& v1, const Version& v2 );
}

#endif
