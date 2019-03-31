/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bexternalbinpermissionmodel.h"
#include "k3bexternalbinmanager.h"
#include "k3bdefaultexternalprograms.h"
#include "k3bglobals.h"

#include <KLocalizedString>

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QSet>

#include <sys/stat.h>


namespace {
    
bool shouldRunSuidRoot( const K3b::ExternalBin* bin )
{
    //
    // Since kernel 2.6.8 older cdrecord versions are not able to use the SCSI subsystem when running suid root anymore
    // So for we ignore the suid root issue with kernel >= 2.6.8 and cdrecord < 2.01.01a02
    //
    // Some kernel version 2.6.16.something again introduced a problem here. Since I do not know the exact version
    // and a workaround was introduced in cdrecord 2.01.01a05 just use that version as the first for suid root.
    //
    // Seems as if cdrdao never had problems with suid root...
    //

    if( bin->name() == "cdrecord" ) {
        return ( K3b::simpleKernelVersion() < K3b::Version( 2, 6, 8 ) ||
                 bin->version() >= K3b::Version( 2, 1, 1, "a05" ) ||
                 bin->hasFeature( "wodim" ) );
    }
    else if( bin->name() == "cdrdao" ) {
        return true;
    }
    else if( bin->name() == "growisofs" ) {
        //
        // starting with 6.0 growiofs raises it's priority using nice(-20)
        // BUT: newer kernels have ridiculously low default memorylocked resource limit, which prevents privileged
        // users from starting growisofs 6.0 with "unable to anonymously mmap 33554432: Resource temporarily unavailable"
        // error message. Until Andy releases a version including a workaround we simply never configure growisofs suid root
        return false; // bin->version >= K3b::Version( 6, 0 );
    }
    else
        return false;
}

} // namespace

namespace K3b {

class ExternalBinPermissionModel::Private
{
public:
    explicit Private(ExternalBinManager const& ebm) : externalBinManager(ebm) {}
    ExternalBinManager const& externalBinManager;
    QString burningGroup;
    QList<const ExternalBin*> programs;
    QSet<const ExternalBin*> selectedPrograms;

    void buildProgramList();
    bool getProgramInfo( const ExternalBin* program,
                         QString& owner, QString& group, QString& wantedGroup,
                         int& perm, int& wantedPerm ) const;
    bool needChangePermissions( const ExternalBin* program ) const;
};


void ExternalBinPermissionModel::Private::buildProgramList()
{
    programs.clear();
    const QMap<QString, ExternalProgram*>& map = externalBinManager.programs();
    for( QMap<QString, ExternalProgram*>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it ) {
        if (it.key() == "cdrecord" ||
            it.key() == "cdrdao" ||
            it.key() == "growisofs") {
            programs += it.value()->bins();
        }
    }
    selectedPrograms = programs.toSet();
}


bool ExternalBinPermissionModel::Private::getProgramInfo( const ExternalBin* program,
                                                  QString& owner, QString& group, QString& wantedGroup,
                                                  int& perm, int& wantedPerm ) const
{
    // we need the uid bit which is not supported by QFileInfo
    struct stat s;
    if( ::stat( QFile::encodeName(program->path()), &s ) == 0 ) {

        QFileInfo fi( program->path() );
        owner = fi.owner();
        group = fi.group();
        perm = s.st_mode & 0007777;

        if( !burningGroup.isEmpty() && burningGroup != "root" )
            wantedGroup = burningGroup;
        else
            wantedGroup = "root";

        if( shouldRunSuidRoot( program ) ) {
            if( wantedGroup != "root" )
                wantedPerm = 0004710;
            else
                wantedPerm = 0004711;
        }
        else {
            if( wantedGroup != "root" )
                wantedPerm = 0000750;
            else
                wantedPerm = 0000755;
        }

        return true;
    }
    else {
        qDebug() << "(ExternalBinPermissionModel) unable to stat " << program->path();
        return false;
    }
}


bool ExternalBinPermissionModel::Private::needChangePermissions( const ExternalBin* program ) const
{
    QString owner, group, wantedGroup;
    int perm, wantedPerm;

    if( getProgramInfo( program, owner, group, wantedGroup, perm, wantedPerm ) )
        return( perm != wantedPerm || owner != "root" || group != wantedGroup );
    else
        return false;
}


ExternalBinPermissionModel::ExternalBinPermissionModel(ExternalBinManager const& externalBinManager, QObject* parent)
:
    QAbstractItemModel( parent ),
    d( new Private( externalBinManager ) )
{
    d->buildProgramList();
}


ExternalBinPermissionModel::~ExternalBinPermissionModel()
{
    delete d;
}


QList<HelperProgramItem> ExternalBinPermissionModel::selectedPrograms() const
{
    QList<HelperProgramItem> selectedPrograms;
    Q_FOREACH( const ExternalBin* program, d->selectedPrograms )
    {
        if( d->needChangePermissions( program ) )
            selectedPrograms << HelperProgramItem( program->path(), shouldRunSuidRoot( program ) );
    }
    return selectedPrograms;
}


bool ExternalBinPermissionModel::changesNeeded() const
{
    return !selectedPrograms().isEmpty();
}


QStringList ExternalBinPermissionModel::searchPaths() const
{
    return d->externalBinManager.searchPath();
}


const QString& ExternalBinPermissionModel::burningGroup() const
{
    return d->burningGroup;
}


const ExternalBin* ExternalBinPermissionModel::programForIndex( const QModelIndex& index ) const
{
    if( index.isValid() )
        return static_cast<const ExternalBin*>( index.internalPointer() );
    else
        return 0;
}


QModelIndex ExternalBinPermissionModel::indexForProgram( const ExternalBin* program ) const
{
    if( program != 0 && !d->programs.isEmpty() ) {
        int row = d->programs.indexOf( program );
        return createIndex( row, 0, const_cast<ExternalBin*>( program ) );
    }
    else
        return QModelIndex();
}


QVariant ExternalBinPermissionModel::data( const QModelIndex& index, int role ) const
{
    if( const ExternalBin* program = programForIndex( index ) ) {
        if( role == Qt::DisplayRole ) {
            if( index.column() == ProgramColumn ) {
                return program->path();
            } else {
                QString owner, group, wantedGroup;
                int perm, wantedPerm;

                if( d->getProgramInfo( program, owner, group, wantedGroup, perm, wantedPerm ) ) {

                    if( index.column() == PermissionsColumn ) {
                        return QString(QString::number( perm, 8 ).rightJustified( 4, '0' ) + ' ' + owner + '.' + group);
                    } else if ( index.column() == NewPermissionsColumn ) {
                        if( perm != wantedPerm || owner != "root" || group != wantedGroup )
                            return QString("%1 root.%2").arg(wantedPerm,0,8).arg(wantedGroup);
                        else
                            return i18n("no change");
                    }
                }
            }
        }
        else if( role == Qt::CheckStateRole && index.column() == ProgramColumn && d->needChangePermissions( program ) ) {
            if( d->selectedPrograms.contains( program ) )
                return Qt::Checked;
            else
                return Qt::Unchecked;
        }
    }
    return QVariant();
}


bool ExternalBinPermissionModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( role == Qt::CheckStateRole ) {
        if( const ExternalBin* program = programForIndex( index ) ) {
            if( value.toInt() == Qt::Unchecked && d->selectedPrograms.contains( program ) ) {
                d->selectedPrograms.remove( program );
                emit dataChanged( index, index );
                return true;
            }
            else if( value.toInt() == Qt::Checked && !d->selectedPrograms.contains( program ) ) {
                d->selectedPrograms.insert( program );
                emit dataChanged( index, index );
                return true;
            }
        }
    }
    return false;
}


Qt::ItemFlags ExternalBinPermissionModel::flags( const QModelIndex& index ) const
{
    if( const ExternalBin* program = programForIndex( index ) )
    {
        if( index.column() == ProgramColumn && d->needChangePermissions( program ) )
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
        else
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    else
        return 0;
}


QVariant ExternalBinPermissionModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        switch( section )
        {
            case ProgramColumn: return i18n( "Program" );
            case PermissionsColumn: return i18n( "Permissions" );
            case NewPermissionsColumn: return i18n( "New permissions" );
            default: return QVariant();
        }
    }
    else
        return QVariant();
}


QModelIndex ExternalBinPermissionModel::index( int row, int column, const QModelIndex& parent ) const
{
    if( hasIndex(row, column, parent) && !parent.isValid() ) {
        const ExternalBin* program = d->programs.at( row );
        if( program != 0 )
            return createIndex( row, column, const_cast<ExternalBin*>( program ) );
        else
            return QModelIndex();
    }
    else
        return QModelIndex();
}


QModelIndex ExternalBinPermissionModel::parent( const QModelIndex& index ) const
{
    Q_UNUSED( index );
    return QModelIndex();
}


int ExternalBinPermissionModel::rowCount( const QModelIndex& parent ) const
{
    if( !parent.isValid() )
        return d->programs.size();
    else
        return 0;
}


int ExternalBinPermissionModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return NumColumns;
}


QModelIndex ExternalBinPermissionModel::buddy( const QModelIndex& index ) const
{
    if( programForIndex( index ) != 0 )
        return ExternalBinPermissionModel::index( index.row(), ProgramColumn, index.parent() );
    else
        return index;
}

void ExternalBinPermissionModel::setBurningGroup( const QString& burningGroup )
{
    if( burningGroup != d->burningGroup ) {
        beginResetModel();
        d->burningGroup = burningGroup;
        
        // Remove from the selected list all programs
        // whose permissions don't need to be changed anymore
        for( QSet<const ExternalBin*>::iterator program = d->selectedPrograms.begin();
             program != d->selectedPrograms.end(); )
        {
            if( !d->needChangePermissions( *program ) )
                program = d->selectedPrograms.erase( program );
            else
                ++program;
        }
        endResetModel();
    }
}

void ExternalBinPermissionModel::update()
{
    beginResetModel();
    d->buildProgramList();
    d->selectedPrograms.intersect( d->programs.toSet() );
    endResetModel();
}

} // namespace K3b


