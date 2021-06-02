// SPDX-FileCopyrightText: 2005 Shaheed Haque (srhaque@iee.org). All rights reserved.
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
#ifndef KCDDB_CATEGORIES_H
#define KCDDB_CATEGORIES_H

#include <QString>
#include <QStringList>

namespace KCDDB
{
   /**
    * Category values defined by CDDB.
    */
    class Categories
    {
    public:
        Categories();

        const QStringList &cddbList() const { return m_cddb; }
        const QStringList &i18nList() const { return m_i18n; }
        
        /**
         * Lookup the CDDB category, and return the i18n'd version.
         */
        const QString cddb2i18n(const QString &category) const;

        /**
         * Lookup the i18n category, and return the CDDB version.
         */
        const QString i18n2cddb(const QString &category) const;
    private:
        QStringList m_cddb;
        QStringList m_i18n;
    };
}

#endif
