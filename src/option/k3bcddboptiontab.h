/* 
 *
 * $Id$
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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
