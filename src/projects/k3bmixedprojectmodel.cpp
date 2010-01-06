/*
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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

#include "k3bmixedprojectmodel.h"
#include "k3bdataprojectmodel.h"
#include "k3baudioprojectmodel.h"
#include "k3bmixeddoc.h"

#include <KLocale>
#include <KIcon>

namespace K3b
{
    MixedProjectModel::MixedProjectModel( K3b::MixedDoc* doc, QObject* parent )
    : K3b::MetaItemModel( parent ), m_doc( doc )
    {
        m_dataModel = new DataProjectModel( doc->dataDoc(), this );
        m_audioModel = new AudioProjectModel( doc->audioDoc(), this );

        addSubModel( i18n("Data Section"), KIcon("media-optical-data"), m_dataModel, true );
        addSubModel( i18n("Audio Section"), KIcon("media-optical-audio"), m_audioModel);
    }

    MixedProjectModel::~MixedProjectModel()
    {
    }

    void MixedProjectModel::slotCurrentRootIndexChanged( const QModelIndex& index )
    {
        m_currentRootIndex = index;
    }

    QVariant MixedProjectModel::headerData( int section, Qt::Orientation orientation, int role ) const
    {
        QAbstractItemModel *model = subModelForIndex( m_currentRootIndex );

        if (!model)
            return K3b::MetaItemModel::headerData( section, orientation, role );

        return model->headerData( section, orientation, role );
    }
}
