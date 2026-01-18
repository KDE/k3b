/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_TITLE_LABEL_H_
#define _K3B_TITLE_LABEL_H_

#include "k3b_export.h"

#include <QResizeEvent>
#include <QFrame>

class QResizeEvent;

namespace K3b {
    class LIBK3B_EXPORT TitleLabel : public QFrame
    {
        Q_OBJECT

    public:
        explicit TitleLabel( QWidget* parent = nullptr );
        ~TitleLabel() override;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

        bool event( QEvent* event ) override;

    public Q_SLOTS:
        /**
         * default: 2
         */
        void setMargin( int );

        void setTitle( const QString& title, const QString& subTitle = QString() );
        void setSubTitle( const QString& subTitle );

        /**
         * The title label only supports alignments left, hcenter, and right
         *
         * Default alignment is left.
         */
        void setAlignment( Qt::Alignment alignment );

    protected:
        void resizeEvent( QResizeEvent* ) override;
        void paintEvent( QPaintEvent* ) override;

    private:
        void updatePositioning();

        class Private;
        Private* d;
    };
}

#endif
