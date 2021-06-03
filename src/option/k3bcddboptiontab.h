/* 
    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_CDDB_OPTIONTAB_H
#define K3B_CDDB_OPTIONTAB_H

#include <QWidget>

class KCModule;

namespace K3b {
class CddbOptionTab : public QWidget
{
    Q_OBJECT

public:
    explicit CddbOptionTab( QWidget* parent = 0 );
    ~CddbOptionTab() override;

public Q_SLOTS:
    void readSettings();
    void apply();

private:
    KCModule* m_cddbKcm;
};
}

#endif
