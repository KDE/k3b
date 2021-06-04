/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bbootimagemodel.h"
#include "k3bdatadoc.h"

#include <KLocalizedString>

namespace K3b {

BootImageModel::BootImageModel( DataDoc* doc, QObject* parent )
    : QAbstractTableModel( parent ),
      m_doc( doc )
{
}


BootImageModel::~BootImageModel()
{
}


BootItem* BootImageModel::bootItemForIndex( const QModelIndex& index ) const
{
    if( index.isValid() && index.row() >= 0 && index.row() < m_doc->bootImages().size() )
        return m_doc->bootImages().at( index.row() );
    else
        return 0;
}


QModelIndex BootImageModel::indexForBootItem( BootItem* bootItem, int column ) const
{
    int i = m_doc->bootImages().indexOf( bootItem );
    if( i >= 0 && i < m_doc->bootImages().size() )
        return index( i, column );
    else
        return QModelIndex();
}


void BootImageModel::createBootItem( const QString& file, BootItem::ImageType imageType )
{
    if( !file.isEmpty() ) {
        int row = m_doc->bootImages().size();
        beginInsertRows( QModelIndex(), row, row );
        m_doc->createBootItem( file )->setImageType( imageType );
        endInsertRows();
    }
}


void BootImageModel::setImageType( const QModelIndex& index, BootItem::ImageType imageType )
{
    if( BootItem* bootItem = bootItemForIndex( index ) ) {
        bootItem->setImageType( imageType );
        QModelIndex changedIndex = sibling( index.row(), EmulationTypeColumn, index );
        Q_EMIT dataChanged( changedIndex, changedIndex );
    }
}


QVariant BootImageModel::data( const QModelIndex& index, int role ) const
{
    if( BootItem* bootItem = bootItemForIndex( index ) ) {
        if( Qt::DisplayRole == role ) {
            switch( index.column() ) {
                case EmulationTypeColumn:
                    switch( bootItem->imageType() ) {
                        case BootItem::FLOPPY:   return i18n("Floppy");
                        case BootItem::HARDDISK: return i18n("Harddisk");
                        default:                 return i18n("None");
                    }
                    
                case SizeColumn:
                    return QString( "%1 KB" ).arg( bootItem->size()/1024 );
                    
                case LocalPathColumn:
                    return bootItem->localPath();
                    
                default:
                    break;
            }
        }
    }
    return QVariant();
}


int BootImageModel::columnCount( const QModelIndex& /*parent*/ ) const
{
    return NumColumns;
}


int BootImageModel::rowCount( const QModelIndex& parent ) const
{
    if( parent.isValid() )
        return 0;
    else
        return m_doc->bootImages().count();
}


QVariant BootImageModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        switch( section ) {
            case EmulationTypeColumn: return i18n( "Emulation Type" );
            case SizeColumn: return i18n( "Size" );
            case LocalPathColumn: return i18n( "Local Path" );
            default: return QVariant();
        }
    }
    return QVariant();
}


bool BootImageModel::removeRows( int row, int count, const QModelIndex& parent )
{
    if( !parent.isValid() ) {
        beginRemoveRows( parent, row, row+count-1 );
        for( int i = 0; i < count; ++i ) {
            if( row >= 0 && row < m_doc->bootImages().size() ) {
                BootItem* item = m_doc->bootImages().takeAt( i );
                delete item;
            }
        }
        endRemoveRows();
        return true;
    }
    else {
        return false;
    }
}

} // namespace K3b


