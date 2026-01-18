/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_CONTENTS_VIEW_H_
#define _K3B_CONTENTS_VIEW_H_

#include "k3bthememanager.h"
#include <QWidget>

namespace K3b {
    class ThemedHeader;

    class ContentsView : public QWidget
    {
        Q_OBJECT

    public:
        ~ContentsView() override;

    protected:
        ContentsView( bool withHeader,
                      QWidget* parent = nullptr );

        QWidget* mainWidget();
        void setMainWidget( QWidget* );
        void setTitle( const QString& title, const QString& subtitle = QString() );
        void setLeftPixmap( Theme::PixmapType );
        void setRightPixmap( Theme::PixmapType );
        
        /**
         * \return true if content view is a currently selected view
         */
        bool isActive() const { return m_active; }
        
    public Q_SLOTS:
        virtual void activate( bool active );

    private:
        ThemedHeader* m_header;
        QWidget* m_centerWidget;
        bool m_active;
    };
}

#endif
