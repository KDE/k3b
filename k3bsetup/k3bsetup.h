/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 * Copyright (C) 2010 Dario Freddi <drf@kde.org>
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


#ifndef _K3BSETUP_H_
#define _K3BSETUP_H_

#include "ui_base_k3bsetup.h"
#include <KCModule>
#include <QStringList>

class K3bSetup : public KCModule, public Ui::base_K3bSetup
{
    Q_OBJECT

public:
    K3bSetup( QWidget* parent = 0, const QVariantList& args = QVariantList() );
    ~K3bSetup();

    QString quickHelp() const;

    void defaults();
    void load();
    void save();

private Q_SLOTS:
    void slotDataChanged();
    void slotBurningGroupChanged();
    void slotSearchPathChanged();

private:
    class Private;
    Private* d;
};

#endif
