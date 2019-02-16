/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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
                      QWidget* parent = 0 );

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
