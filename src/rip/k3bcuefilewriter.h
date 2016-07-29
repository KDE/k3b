/* 
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_CUE_FILE_WRITER_H_
#define _K3B_CUE_FILE_WRITER_H_

#include <QTextStream>

#include "k3btoc.h"
#include "k3bcdtext.h"

/**
 * Write a CDRWIN cue file.
 * For now this writer only supports audio CDs
 * for usage in the K3b audio CD ripper.
 */

namespace K3b {
class CueFileWriter
{
public:
    CueFileWriter();

    bool save( QTextStream& );
    bool save( const QString& filename );

    void setData( const Device::Toc& toc ) { m_toc = toc; }
    void setCdText( const Device::CdText& text ) { m_cdText = text; }
    void setImage( const QString& name, const QString& type ) { m_image = name; m_dataType = type; }

private:
    Device::Toc m_toc;
    Device::CdText m_cdText;
    QString m_image;
    QString m_dataType;
};
}

#endif
