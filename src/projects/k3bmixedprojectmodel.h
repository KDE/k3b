/*
 * Copyright (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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

#ifndef K3BMIXEDMODEL_H
#define K3BMIXEDMODEL_H

#include "k3bmetaitemmodel.h"
#include <QPersistentModelIndex>

namespace K3b {
    class MixedDoc;
}

namespace K3b
{

    class DataProjectModel;
    class AudioProjectModel;

    /**
     * The mixed model encapsulates one data model and one audio model.
     * It uses the MetaItemModel to do so
     *
     * @author Gustavo Pichorim Boiko
     */
    class MixedProjectModel : public MetaItemModel
    {
        Q_OBJECT
    public:
        MixedProjectModel( MixedDoc* doc, QObject* parent = 0 );
        ~MixedProjectModel();
        QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        
        DataProjectModel* dataModel() const { return m_dataModel; }
        AudioProjectModel* audioModel() const { return m_audioModel; }

    public slots:
        /**
         * This method is used to know what is the root index being shown in the 
         * file treeview. Knowing what root is being used, we can show the appropriate
         * column headers for the right submodel
         */
        void slotCurrentRootIndexChanged( const QModelIndex& index );

    private:
        DataProjectModel *m_dataModel;
        AudioProjectModel *m_audioModel;
        MixedDoc *m_doc;
        QPersistentModelIndex m_currentRootIndex;
    };

}
#endif
