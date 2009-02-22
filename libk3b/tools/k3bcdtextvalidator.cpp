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
