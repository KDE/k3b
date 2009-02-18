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


#ifndef _K3BSETUP2_H_
#define _K3BSETUP2_H_

#include <kcmodule.h>
#include <kaboutdata.h>
#include "ui_base_k3bsetup2.h"

class Q3CheckListItem;

class base_K3bSetup2 : public QWidget, public Ui::base_K3bSetup2
{
public:
    base_K3bSetup2( QWidget *parent ) : QWidget( parent ) {
        setupUi( this );
    }
};

class K3bSetup2: public KCModule
{
    Q_OBJECT

public:
    K3bSetup2( QWidget* parent = 0, const QVariantList& args = QVariantList() );
    ~K3bSetup2();

    QString quickHelp() const;

    void load();
    void save();
    void defaults();

public Q_SLOTS:
    void updateViews();

private Q_SLOTS:
    void slotSearchPrograms();

private:
    void updatePrograms();
    void updateDevices();
    QString burningGroup() const;
    void makeReadOnly();
    Q3CheckListItem* createDeviceItem( const QString& deviceNode );

    class Private;
    Private* d;

    base_K3bSetup2* w;
};

#endif
