/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bviewcolumnadjuster.h"

#include <QAbstractItemModel>
#include <QDebug>
#include <QEvent>
#include <QHash>
#include <QList>
#include <QSet>
#include <QTreeView>
#include <QHeaderView>


class K3b::ViewColumnAdjuster::Private
{
public:
    Private( ViewColumnAdjuster* parent )
        : view( 0 ),
          q( parent ) {
    }

    int columnSizeHint( int col );

    void _k_adjustColumns();

    QTreeView* view;
    QSet<int> fixedColumns;
    QHash<int, int> margins;

private:
    ViewColumnAdjuster* q;
};


void K3b::ViewColumnAdjuster::Private::_k_adjustColumns()
{
    if ( q->receivers( SIGNAL(columnsNeedAjusting()) ) ) {
        emit q->columnsNeedAjusting();
    }
    else {
        q->adjustColumns();
    }
}


int K3b::ViewColumnAdjuster::Private::columnSizeHint( int col )
{
    int w = static_cast<QAbstractItemView*>( view )->sizeHintForColumn( col );
    QModelIndex firstRow = view->model()->index( 0, col );
    if ( firstRow.isValid() && firstRow.flags().testFlag( Qt::ItemIsUserCheckable ) ) {
        w += 10; // HACK
    }
    if ( !view->header()->isHidden() ) {
        w = qMax( w, view->header()->sectionSizeHint( col ) );
    }
    return w;
}


K3b::ViewColumnAdjuster::ViewColumnAdjuster( QObject* parent )
    : QObject( parent ),
      d( new Private(this) )
{
}


K3b::ViewColumnAdjuster::ViewColumnAdjuster( QTreeView* parent )
    : QObject( parent ),
      d( new Private(this) )
{
    setView( parent );
}


K3b::ViewColumnAdjuster::~ViewColumnAdjuster()
{
    delete d;
}


void K3b::ViewColumnAdjuster::setView( QTreeView* view )
{
    if ( d->view != view ) {
        if ( d->view ) {
            d->view->removeEventFilter( this );
            d->view->model()->disconnect( this );
        }
        d->view = view;
        if ( d->view ) {
            d->view->header()->setSectionResizeMode( QHeaderView::Fixed );
            d->view->installEventFilter( this );
            connect( d->view->model(), SIGNAL(modelReset()), SLOT(_k_adjustColumns()) );
            connect( d->view->model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(_k_adjustColumns()) );
            connect( d->view->model(), SIGNAL(headerDataChanged(Qt::Orientation,int,int)), SLOT(_k_adjustColumns()) );
            connect( d->view->model(), SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(_k_adjustColumns()) );
            connect( d->view->model(), SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(_k_adjustColumns()) );
            connect( d->view->model(), SIGNAL(columnsInserted(QModelIndex,int,int)), SLOT(_k_adjustColumns()) );
            connect( d->view->model(), SIGNAL(columnsRemoved(QModelIndex,int,int)), SLOT(_k_adjustColumns()) );
            d->view->header()->setSectionResizeMode( QHeaderView::Interactive );
        }
    }
}


void K3b::ViewColumnAdjuster::addFixedColumn( int col )
{
    d->fixedColumns << col;
}


void K3b::ViewColumnAdjuster::setColumnMargin( int col, int margin )
{
    d->margins[col] = margin;
}


int K3b::ViewColumnAdjuster::columnMargin( int column ) const
{
    if ( d->margins.contains( column ) ) {
        return d->margins[column];
    }
    else {
        return 0;
    }
}


bool K3b::ViewColumnAdjuster::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == d->view) {
        if (event->type() == QEvent::Resize || event->type() == QEvent::UpdateLater) {
            d->_k_adjustColumns();
        }
    }

    // we never "eat" an event
    return false;
}


int K3b::ViewColumnAdjuster::columnSizeHint( int col ) const
{
    return d->columnSizeHint( col );
}


void K3b::ViewColumnAdjuster::adjustColumns()
{
    // fixed sizes
    int x = 0;
    QList<int> otherCols;
    for ( int i = 0; i < d->view->model()->columnCount( d->view->rootIndex() ); ++i ) {
        if ( d->fixedColumns.contains( i ) ) {
            int w = d->columnSizeHint( i ) + columnMargin( i );
            d->view->setColumnWidth( i, w );
            x += w;
        }
        else {
            otherCols << i;
        }
    }

    if ( otherCols.count() ) {
        QList<int> otherColSizes;
        int xx = 0;
        int widthLeft = d->view->viewport()->contentsRect().width() - x;
        int max = widthLeft / otherCols.count();
        for ( int i = 0; i < otherCols.count(); ++i ) {
            int sh = d->columnSizeHint( otherCols[i] ) + columnMargin( otherCols[i] );
            otherColSizes << qMin( sh, max );
            xx += qMin( sh, max );
        }

        // do we have something left
        for ( int i = 0; i < otherCols.count(); ++i ) {
            d->view->setColumnWidth( otherCols[i], qMax( 0, otherColSizes[i] + ( widthLeft-xx )/otherColSizes.count() ) );
        }
    }
}

#include "moc_k3bviewcolumnadjuster.cpp"
