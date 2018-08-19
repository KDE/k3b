/* 
 *
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bencodingconverter.h"
#include <config-k3b.h>

#include <QDebug>

#ifdef HAVE_ICONV
#include <langinfo.h>
#include <iconv.h>
#endif


class K3b::EncodingConverter::Private
{
public:
#ifdef HAVE_ICONV
    iconv_t ic;
#endif
};


K3b::EncodingConverter::EncodingConverter()
    : d( new Private )
{
#ifdef HAVE_ICONV
    char* codec = nl_langinfo( CODESET );
    qDebug() << "(K3b::DataUrlAddingDialog) using locale codec: " << codec;
    d->ic = ::iconv_open( "UCS-2BE", codec );
#endif
}


K3b::EncodingConverter::~EncodingConverter()
{
#ifdef HAVE_ICONV
    ::iconv_close( d->ic );
#endif
    delete d;
}


bool K3b::EncodingConverter::encodedLocally( const QByteArray& s )
{
#ifdef HAVE_ICONV
    QByteArray utf8Encoded( s.length()*2, '\0' );
#ifdef ICONV_SECOND_ARGUMENT_IS_CONST
    const char* in = s.data();
#else
    char* in = const_cast<char*>( s.data() );
#endif
    char* out = utf8Encoded.data();
    size_t inSize = s.length();
    size_t outSize = utf8Encoded.size();
    return( (size_t)-1 != ::iconv( d->ic, &in, &inSize, &out, &outSize ) );
#else
    return true;
#endif
}
