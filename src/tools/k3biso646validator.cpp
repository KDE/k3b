/*
 *
 * $Id: $
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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

#include "k3biso646validator.h"
#include <qregexp.h>

K3bIso646Validator::K3bIso646Validator(QObject *parent, const char *name )
    : QRegExpValidator( setIsoType( 0 ), parent, name )
{
}

K3bIso646Validator::K3bIso646Validator( int type, QObject *parent, const char *name )
    : QRegExpValidator( setIsoType( type ), parent, name )
{
}

K3bIso646Validator::K3bIso646Validator( int type, bool allowlowercase, QObject *parent, const char *name )
    : QRegExpValidator( setIsoType( type, allowlowercase ), parent, name )
{
}

K3bIso646Validator::~K3bIso646Validator()
{
}

QRegExp K3bIso646Validator::setIsoType( int type, bool AllowLowerCase )
{
    m_type = type;
    m_allowlowercase = AllowLowerCase;
    
    QRegExp rx;
    switch ( type ) {
        case K3bIso646Validator::Iso646_a:
            if ( AllowLowerCase )
                rx = QRegExp( "[a-zA-Z0-9!\" %&'()*+,-./:;<=>?_]*");
            else
                rx = QRegExp( "[A-Z0-9!\" %&'()*+,-./:;<=>?_]*" );
            break;
        case K3bIso646Validator::Iso646_d:
            if ( AllowLowerCase )
                rx = QRegExp( "[a-zA-Z0-9_]*" );
            else
                rx = QRegExp( "[A-Z0-9_]*" );
            break;
        default: 
            rx = setIsoType( 0 );
    }

    return rx;
}
