/*
 *
 *  Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_K3BDATAPROJECTDELEGATE_H
#define K3B_K3BDATAPROJECTDELEGATE_H

#include <QStyledItemDelegate>

namespace K3b {

    class DataProjectDelegate : public QStyledItemDelegate
    {
    public:
        DataProjectDelegate( QObject* parent = 0 );
        virtual void setEditorData( QWidget* editor, const QModelIndex& index ) const;
    };

}

#endif // K3B_K3BDATAPROJECTDELEGATE_H
