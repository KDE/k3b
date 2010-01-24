/* 
 *
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#ifndef _K3B_VIDEODVD_RIPPING_VIEW_H_
#define _K3B_VIDEODVD_RIPPING_VIEW_H_

#include "k3bmediacontentsview.h"

class KActionCollection;

namespace K3b {

    class VideoDVDRippingView : public MediaContentsView
    {
        Q_OBJECT

    public:
        VideoDVDRippingView( QWidget* parent = 0 );
        ~VideoDVDRippingView();

        KActionCollection* actionCollection() const;

    private Q_SLOTS:
        void slotStartRipping();
        void slotContextMenu( const QPoint& pos );
        void slotCheck();
        void slotUncheck();
        void slotShowFiles();

    private:
        virtual void reloadMedium();
        virtual void enableInteraction( bool enable );
        virtual void activate( bool active );
        void initActions();
        
        class Private;
        Private* d;
    };

} // namespace K3b

#endif
