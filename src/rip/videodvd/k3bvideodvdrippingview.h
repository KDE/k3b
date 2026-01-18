/* 
    SPDX-FileCopyrightText: 2010-2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
        explicit VideoDVDRippingView( QWidget* parent = nullptr );
        ~VideoDVDRippingView() override;

        KActionCollection* actionCollection() const;

    protected:
        bool eventFilter( QObject* obj, QEvent* event ) override;

    private Q_SLOTS:
        void slotStartRipping();
        void slotContextMenu( const QPoint& pos );
        void slotContextMenuAboutToShow();
        void slotCheck();
        void slotUncheck();
        void slotToggle();
        void slotShowFiles();

    private:
        void reloadMedium() override;
        void enableInteraction( bool enable ) override;
        void activate( bool active ) override;
        void initActions();
        
        class Private;
        Private* d;
    };

} // namespace K3b

#endif
