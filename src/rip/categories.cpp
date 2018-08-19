// Copyright (C) 2005 by Shaheed Haque (srhaque@iee.org). All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//

#include "categories.h"

#include <KLocalizedString>

KCDDB::Categories::Categories()
{
    // These are only 11 Category values defined by CDDB. See
    //
    // http://www.freedb.org/modules.php?name=Sections&sop=viewarticle&artid=26
    //
    m_cddb << "blues" << "classical" << "country" <<
            "data" << "folk" << "jazz" << "misc" <<
            "newage" << "reggae" << "rock" << "soundtrack";
    m_i18n << i18n("Blues") << i18n("Classical") << i18nc("music genre", "Country") <<
            i18n("Data") << i18n("Folk") << i18n("Jazz") << i18n("Miscellaneous") <<
            i18n("New Age") << i18n("Reggae") << i18n("Rock") << i18n("Soundtrack");
}

const QString KCDDB::Categories::cddb2i18n(const QString &category) const
{
    int index = m_cddb.indexOf(category.trimmed());
    if (index != -1)
    {
        return m_i18n[index];
    }
    else
    {
        return cddb2i18n("misc");
    }
}

const QString KCDDB::Categories::i18n2cddb(const QString &category) const
{
    int index = m_i18n.indexOf(category.trimmed());
    if (index != -1)
    {
        return m_cddb[index];
    }
    else
    {
        return "misc";
    }
}
