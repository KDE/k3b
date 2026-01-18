/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2009 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2009 Michal Malek <michalm@jabster.pl>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3BEXTERNALBINPERMISSIONMODEL_H
#define K3BEXTERNALBINPERMISSIONMODEL_H

#include "k3bhelperprogramitem.h"
#include "config-k3b.h"
#include <QAbstractItemModel>
#include <QList>
#include <QStringList>

namespace K3b {

    class ExternalBin;
    class ExternalBinManager;

    class ExternalBinPermissionModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        enum Columns {
            ProgramColumn,
            PermissionsColumn,
            NewPermissionsColumn,
            NumColumns
        };

    public:
        explicit ExternalBinPermissionModel(ExternalBinManager const& externalBinManager, QObject* parent = nullptr);
        ~ExternalBinPermissionModel() override;

        QList<HelperProgramItem> selectedPrograms() const;
        bool changesNeeded() const;
        QStringList searchPaths() const;
        const QString& burningGroup() const;

        const ExternalBin* programForIndex( const QModelIndex& index ) const;
        QModelIndex indexForProgram( const ExternalBin* program ) const;

        QVariant data( const QModelIndex& index, int role ) const override;
        bool setData( const QModelIndex& index, const QVariant& value, int role ) override;
        Qt::ItemFlags flags( const QModelIndex& index ) const override;
        QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;
        QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const override;
        QModelIndex parent( const QModelIndex& index ) const override;
        int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
        int columnCount( const QModelIndex& parent = QModelIndex() ) const override;
        QModelIndex buddy( const QModelIndex& index ) const override;

    public Q_SLOTS:
        void setBurningGroup( const QString& burningGroup );
        void update();

    private:
        class Private;
        Private* d;
    };

} // namespace K3b

#endif // K3BEXTERNALBINPERMISSIONMODEL_H
