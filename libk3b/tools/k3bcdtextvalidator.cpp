/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#include "k3bcdtextvalidator.h"

K3b::CdTextValidator::CdTextValidator(QObject *parent)
    : K3b::Latin1Validator(parent)
{
}


K3b::CdTextValidator::~CdTextValidator()
{
}


QValidator::State K3b::CdTextValidator::validate( QString& input, int& pos ) const
{
    if( input.length() > 160 )
        return Invalid;

    // forbid some characters that might introduce problems
    for( int i = 0; i < input.length(); ++i ) {
        if( input[i] == '/' || input[i] == '"' || input[i] == '\\' )
            return Invalid;
    }

    return K3b::Latin1Validator::validate( input, pos );
}
