/* 

    SPDX-FileCopyrightText: 2006 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
