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

#include "k3bstringutils.h"

#include <qfontmetrics.h>

#include <kdebug.h>


QString K3b::cutToWidth( const QFontMetrics& fm, const QString& fullText, int cutWidth )
{
    return fm.elidedText( fullText, Qt::ElideRight, cutWidth );
}


QString K3b::squeezeTextToWidth( const QFontMetrics& fm, const QString& fullText, int cutWidth )
{
    return fm.elidedText( fullText, Qt::ElideMiddle, cutWidth );
}
