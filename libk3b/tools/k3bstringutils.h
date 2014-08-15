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


#ifndef _K3B_STRING_UTILS_H_
#define _K3B_STRING_UTILS_H_

#include <QtCore/QString>
#include <KDELibs4Support/kdemacros.h>

class QFontMetrics;

namespace K3b
{
    /**
     * Cuts the text at the end.
     * Example: "some long text" -> "some lo..."
     *
     * \deprecated Use QFontMetrics::elideText
     */
    KDE_DEPRECATED QString cutToWidth( const QFontMetrics&, const QString&, int );

    /**
     * squeezes the text.
     * Example: "some long text" -> "some...ext"
     *
     * \deprecated Use QFontMetrics::elideText
     */
    KDE_DEPRECATED QString squeezeTextToWidth( const QFontMetrics& fm, const QString& fullText, int cutWidth );
}

#endif
