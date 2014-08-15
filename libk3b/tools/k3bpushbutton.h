/*
 *
 * Copyright (C) 2004-2007 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_PUSH_BUTTON_H_
#define _K3B_PUSH_BUTTON_H_

#include "k3b_export.h"

#include <KDELibs4Support/KDE/KPushButton>

class QMenu;
class QEvent;

namespace K3b {
    /**
     * A pushbutton with delayed popu pmenu support just like the KToolBarButton
     */
    class LIBK3B_EXPORT PushButton : public KPushButton
    {
        Q_OBJECT

    public:
        /**
         * Default constructor.
         */
        PushButton( QWidget* parent = 0 );

        /**
         * Constructor, that sets the button-text to @p text
         */
        PushButton( const QString& text, QWidget* parent = 0 );

        /**
         * Constructor, that sets an icon and the button-text to @p text
         */
/*   PushButton( const QIcon& icon, const QString& text, */
/* 		 QWidget* parent = 0 ); */

        /**
         * Constructor that takes a KGuiItem for the text, the icon, the tooltip
         * and the what's this help
         */
        PushButton( const KGuiItem& item, QWidget* parent = 0 );

        /**
         * Destructs the button.
         */
        ~PushButton();

        /**
         * The popup menu will show if the button is pressed down for about half a second
         * or if the mouse is moved while pressed just like the KToolBarButton.
         */
        void setDelayedPopupMenu( QMenu* );

    protected:
        virtual bool eventFilter( QObject*, QEvent* );

    private Q_SLOTS:
        void slotDelayedPopup();

    private:
        class Private;
        Private* d;
    };
}

#endif
