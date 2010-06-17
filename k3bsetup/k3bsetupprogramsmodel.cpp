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

#include "k3bsetupprogramsmodel.h"
#include "k3bexternalbinmanager.h"
#include "k3bdefaultexternalprograms.h"
#include "k3bglobals.h"

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KLocale>

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
namespace Setup {

class ProgramsModel::Private
{
public:
    ExternalBinManager* externalBinManager;
    QString burningGroup;
    QList<const ExternalBin*> programs;
    QSet<const ExternalBin*> selectedPrograms;

    void buildProgramList();
    bool getProgramInfo( const ExternalBin* program,
                         QString& owner, QString& group, QString& wantedGroup,
                         int& perm, int& wantedPerm ) const;
    bool needChangePermissions( const ExternalBin* program ) const;
};


void ProgramsModel::Private::buildProgramList()
{
    externalBinManager->search();
    programs.clear();
    const QMap<QString, ExternalProgram*>& map = externalBinManager->programs();
    for( QMap<QString, ExternalProgram*>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it ) {
        programs += it.value()->bins();
    }
}


bool ProgramsModel::Private::getProgramInfo( const ExternalBin* program,
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

        if( !burningGroup.isEmpty() )
            wantedGroup = burningGroup;
        else
            wantedGroup = "root";

        if( shouldRunSuidRoot( program ) ) {
            if( !burningGroup.isEmpty() )
                wantedPerm = 0004710;
            else
                wantedPerm = 0004711;
        }
        else {
            if( !burningGroup.isEmpty() )
                wantedPerm = 0000750;
            else
                wantedPerm = 0000755;
        }

        return true;
    }
    else {
        kDebug() << "(K3bSetup) unable to stat " << program->path();
        return false;
    }
}


bool ProgramsModel::Private::needChangePermissions( const ExternalBin* program ) const
{
    QString owner, group, wantedGroup;
    int perm, wantedPerm;

    if( getProgramInfo( program, owner, group, wantedGroup, perm, wantedPerm ) )
        return( perm != wantedPerm || owner != "root" || group != wantedGroup );
    else
        return false;
}


ProgramsModel::ProgramsModel( QObject* parent )
:
    QAbstractItemModel( parent ),
    d( new Private )
{
    d->externalBinManager = new ExternalBinManager( this );
    // these are the only programs that need special permissions
    d->externalBinManager->addProgram( new CdrdaoProgram() );
    d->externalBinManager->addProgram( new CdrecordProgram() );
    d->externalBinManager->addProgram( new GrowisofsProgram() );
    d->buildProgramList();
}


ProgramsModel::~ProgramsModel()
{
    delete d;
}


void ProgramsModel::load( const KConfig& config )
{
    d->selectedPrograms.clear();
    d->externalBinManager->readConfig( config.group( "External Programs" ) );
    d->buildProgramList();
    reset();
}


void ProgramsModel::save( KConfig& config ) const
{
    d->externalBinManager->saveConfig( config.group( "External Programs" ) );
}


void ProgramsModel::defaults()
{
    d->selectedPrograms.clear();
    d->buildProgramList();
    reset();
}


QList<ProgramItem> ProgramsModel::selectedPrograms() const
{
    QList<ProgramItem> selectedPrograms;
    Q_FOREACH( const ExternalBin* program, d->selectedPrograms )
    {
        if( d->needChangePermissions( program ) )
            selectedPrograms << ProgramItem( program->path(), shouldRunSuidRoot( program ) );
    }
    return selectedPrograms;
}


bool ProgramsModel::changesNeeded() const
{
    return !selectedPrograms().isEmpty();
}


QStringList ProgramsModel::searchPaths() const
{
    return d->externalBinManager->searchPath();
}


const ExternalBin* ProgramsModel::programForIndex( const QModelIndex& index ) const
{
    if( index.isValid() )
        return static_cast<const ExternalBin*>( index.internalPointer() );
    else
        return 0;
}


QModelIndex ProgramsModel::indexForProgram( const ExternalBin* program ) const
{
    if( program != 0 && !d->programs.isEmpty() ) {
        int row = d->programs.indexOf( program );
        return createIndex( row, 0, const_cast<ExternalBin*>( program ) );
    }
    else
        return QModelIndex();
}


QVariant ProgramsModel::data( const QModelIndex& index, int role ) const
{
    if( const ExternalBin* program = programForIndex( index ) ) {
        if( role == Qt::DisplayRole ) {
            if( index.column() == 0 ) {
                return program->name();
            }
            else if( index.column() == 1 ) {
                return program->version().toString();
            }
            else if( index.column() == 2 ) {
                return program->path();
            }
            else {
                QString owner, group, wantedGroup;
                int perm, wantedPerm;

                if( d->getProgramInfo( program, owner, group, wantedGroup, perm, wantedPerm ) ) {

                    if( index.column() == 3 ) {
                        return QString::number( perm, 8 ).rightJustified( 4, '0' ) + " " + owner + "." + group;
                    }
                    else if ( index.column() == 4 ) {
                        if( perm != wantedPerm || owner != "root" || group != wantedGroup )
                            return QString("%1 root.%2").arg(wantedPerm,0,8).arg(wantedGroup);
                        else
                            return i18n("no change");
                    }
                }
            }
        }
        else if( role == Qt::CheckStateRole && index.column() == 0 && d->needChangePermissions( program ) ) {
            if( d->selectedPrograms.contains( program ) )
                return Qt::Checked;
            else
                return Qt::Unchecked;
        }
    }
    return QVariant();
}


bool ProgramsModel::setData( const QModelIndex& index, const QVariant& value, int role )
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


Qt::ItemFlags ProgramsModel::flags( const QModelIndex& index ) const
{
    if( const ExternalBin* program = programForIndex( index ) )
    {
        if( index.column() == 0 && d->needChangePermissions( program ) )
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
        else
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    else
        return 0;
}


QVariant ProgramsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        switch( section )
        {
            case 0: return i18n( "Program" );
            case 1: return i18n( "Version" );
            case 2: return i18n( "Path" );
            case 3: return i18n( "Permissions" );
            case 4: return i18n( "New permissions" );
            default: return QVariant();
        }
    }
    else
        return QVariant();
}


QModelIndex ProgramsModel::index( int row, int column, const QModelIndex& parent ) const
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


QModelIndex ProgramsModel::parent( const QModelIndex& index ) const
{
    Q_UNUSED( index );
    return QModelIndex();
}


int ProgramsModel::rowCount( const QModelIndex& parent ) const
{
    if( !parent.isValid() )
        return d->programs.size();
    else
        return 0;
}


int ProgramsModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return 5;
}


void ProgramsModel::setBurningGroup( const QString& burningGroup )
{
    if( burningGroup != d->burningGroup ) {
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
        reset();
    }
}

void ProgramsModel::setSearchPaths( const QStringList& searchPaths )
{
    if( searchPaths != d->externalBinManager->searchPath() ) {
        d->externalBinManager->setSearchPath( searchPaths );
        update();
    }
}

void ProgramsModel::update()
{
    d->buildProgramList();
    d->selectedPrograms.intersect( d->programs.toSet() );
    reset();
}

} // namespace Setup
} // namespace K3b

#include "k3bsetupprogramsmodel.moc"
