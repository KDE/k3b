/* 

    SPDX-FileCopyrightText: 2004-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
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
