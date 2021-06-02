/*

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#include "k3bbootitem.h"
#include "k3bdatadoc.h"
#include "k3bdiritem.h"
#include "k3b_i18n.h"

namespace K3b {

BootItem::BootItem( const QString& fileName, DataDoc& doc, const QString& k3bName )
    : FileItem( fileName, doc, k3bName, ItemFlags(FILE|BOOT_IMAGE) ),
      m_noBoot(false),
      m_bootInfoTable(false),
      m_loadSegment(0),
      m_loadSize(0),
      m_imageType(FLOPPY)
{
    setExtraInfo( i18n("El Torito Boot image") );
}


BootItem::BootItem( const BootItem& item )
    : FileItem( item ),
      m_noBoot( item.m_noBoot ),
      m_bootInfoTable( item.m_bootInfoTable ),
      m_loadSegment( item.m_loadSegment ),
      m_loadSize( item.m_loadSize ),
      m_imageType( item.m_imageType ),
      m_tempPath( item.m_tempPath )
{
}


BootItem::~BootItem()
{
    take();
}


DataItem* BootItem::copy() const
{
    return new BootItem( *this );
}

} // namespace K3b
