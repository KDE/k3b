/*
    SPDX-FileCopyrightText: 2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_VIEW_COLUMN_ADJUSTER_H_
#define _K3B_VIEW_COLUMN_ADJUSTER_H_

#include <QList>
#include <QObject>

class QEvent;
class QTreeView;

namespace K3b {
    class ViewColumnAdjuster : public QObject
    {
        Q_OBJECT

    public:
        explicit ViewColumnAdjuster( QObject* parent = nullptr );
        explicit ViewColumnAdjuster( QTreeView* parent );
        ~ViewColumnAdjuster() override;

        /**
         * Takes header into account if not hidden.
         */
        int columnSizeHint( int col ) const;

        /**
         * Sets the view column adjuster operates on.
         * Call this *after* calling setModel() on view,
         * otherwise column adjuster won't work.
         */
        void setView( QTreeView* view );
        void addFixedColumn( int );

        void setColumnMargin( int column, int margin );
        int columnMargin( int column ) const;

        bool eventFilter( QObject* watched, QEvent* event ) override;

    Q_SIGNALS:
        /**
         * If something is connected to this slot, adjustColumns
         * will not be called automatically.
         */
        void columnsNeedAjusting();

    public Q_SLOTS:
        void adjustColumns();

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_adjustColumns() )
    };
}

#endif
