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

#ifndef _K3B_MODEL_UTILS_H_
#define _K3B_MODEL_UTILS_H_

#include <QModelIndexList>

class QAbstractItemModel;
class QString;

namespace K3b
{
namespace ModelUtils
{
    
    /**
     * @returns common check state for a specified index list. When indexes has a various check states, Qt::PartiallyChecked is returnd.
     */
    Qt::CheckState commonCheckState( const QModelIndexList& indexes );
    
    /**
     * Toggles check state of a multiple indexes.
     */
    void toggleCommonCheckState( QAbstractItemModel* model, const QModelIndexList& indexes );
    
    /**
     * @returns common text from a specified index list. When data values are different, a null string is returned.
     */
    QString commonText( const QModelIndexList& indexes, int role = Qt::DisplayRole );
    
    /**
     * Sets the same text for a multiple model indexes. The data is set when value is non-empty.
     */
    void setCommonText( QAbstractItemModel* model, const QModelIndexList& indexes, const QString& value, int role = Qt::EditRole );
    
} // namespace ModelUtils
} // namespace K3b

#endif // #ifndef _K3B_MODEL_UTILS_H_
