/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_VALIDATORS_H_
#define _K3B_VALIDATORS_H_

#include "k3b_export.h"
#include <QValidator>


namespace K3b {
    /**
     * Simple validator that validates a string char by char
     */
    class LIBK3B_EXPORT CharValidator : public QValidator
    {
    public:
        explicit CharValidator( QObject* parent = 0 );

        virtual State validateChar( const QChar& ) const = 0;

        State validate( QString& s, int& pos ) const override;

        /**
         * Replaces all invalid chars with the repplace char
         */
        void fixup( QString& ) const override;

        /**
         * Default to '_'
         */
        void setReplaceChar( const QChar& c ) { m_replaceChar = c; }

    private:
        QChar m_replaceChar;
    };


    class LIBK3B_EXPORT Latin1Validator : public CharValidator
    {
    public:
        explicit Latin1Validator( QObject* parent = 0 );

        State validateChar( const QChar& ) const override;
    };


    class LIBK3B_EXPORT AsciiValidator : public Latin1Validator
    {
    public:
        explicit AsciiValidator( QObject* parent = 0 );

        State validateChar( const QChar& ) const override;
    };


    /**
     * The Validator extends QRegExpValidator with a fixup method
     * that just replaces all characters that are not allowed with the
     * replace character. It only makes sense for QRegExps that simply
     * allow or forbid some characters.
     */
    class LIBK3B_EXPORT Validator : public QRegExpValidator
    {
    public:
        explicit Validator( QObject* parent );
        Validator( const QRegExp& rx, QObject* parent );

        void setReplaceChar( const QChar& s ) { m_replaceChar = s; }
        const QChar& replaceChar() const { return m_replaceChar; }

        void fixup( QString& ) const override;

    private:
        QChar m_replaceChar;
    };


    namespace Validators
    {
        /**
         * just replaces all characters that are not allowed with the
         * replace character. It only makes sense for QRegExps that simply
         * allow or forbid some characters.
         */
        LIBK3B_EXPORT QString fixup( const QString&, const QRegExp&, const QChar& replaceChar = '_' );

        /**
         * Validates an ISRC code of the form "CCOOOYYSSSSS" where:
         * <ul>
         * <li>C: country code (upper case letters or digits)</li>
         * <li>O: owner code (upper case letters or digits)</li>
         * <li>Y: year (digits)</li>
         * <li>S: serial number (digits)</li>
         * </ul>
         */
        LIBK3B_EXPORT Validator* isrcValidator( QObject* parent = 0 );

        /**
         * This needs to be replaced by something better in the future...
         * Even the name sucks!
         */
        LIBK3B_EXPORT Validator* iso9660Validator( bool allowEmpty = true, QObject* parent = 0 );

        /**
         * (1) d-characters are: A-Z, 0-9, _ (see ISO-9660:1988, Annex A, Table 15)
         * (2) a-characters are: A-Z, 0-9, _, space, !, ", %, &, ', (, ), *, +, ,, -, ., /, :, ;, <, =, >, ?
         * (see ISO-9660:1988, Annex A, Table 14)
         */
        enum Iso646Type {
            Iso646_a,
            Iso646_d
        };

        LIBK3B_EXPORT Validator* iso646Validator( int type = Iso646_a,
                                                  bool AllowLowerCase = false,
                                                  QObject* parent = 0 );
    }
}

#endif
