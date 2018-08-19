/* 
 *
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bmodelutils.h"
#include <QModelIndex>

namespace K3b
{
namespace ModelUtils
{
    
    Qt::CheckState commonCheckState( const QModelIndexList& indexes )
    {
        int checked = 0;
        foreach( const QModelIndex& index, indexes ) {
            if ( index.data( Qt::CheckStateRole ).toInt() == Qt::Checked ) {
                ++checked;
            }
        }
        
        if ( checked == 0 )
            return Qt::Unchecked;
        else if( checked == indexes.count() )
            return Qt::Checked;
        else
            return Qt::PartiallyChecked;
    }
    
    void toggleCommonCheckState( QAbstractItemModel* model, const QModelIndexList& indexes )
    {
        if ( model != 0 ) {
            Qt::CheckState commonState = commonCheckState( indexes );
            if ( commonState == Qt::Checked )
                commonState = Qt::Unchecked;
            else
                commonState = Qt::Checked;
            
            foreach( const QModelIndex& index, indexes ) {
                model->setData( index, commonState, Qt::CheckStateRole );
            }
        }
    }
    
    QString commonText( const QModelIndexList& indexes, int role )
    {
        if ( !indexes.isEmpty() ) {
            QString firstData = indexes.first().data( role ).toString();
            for ( int i = 1; i < indexes.size(); ++i ) {
                if ( indexes[i].data( role ).toString() != firstData )
                    return QString();
            }
            return firstData;
        }
        return QString();
    }
    
    void setCommonText( QAbstractItemModel* model, const QModelIndexList& indexes, const QString& value, int role )
    {
        if ( model != 0 ) {
            Q_FOREACH( QModelIndex index, indexes ) {
                if( !value.isEmpty() || indexes.size() == 1 )
                    model->setData( index, value, role );
            }
        }
    }
    
} // namespace ModelUtils
} // namespace K3b
