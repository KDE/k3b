/*
    SPDX-FileCopyrightText: 2005-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2009 Arthur Renato Mello <arthur@mandriva.com>
    SPDX-FileCopyrightText: 2009 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_VIDEO_DVDVIEW_H_
#define _K3B_VIDEO_DVDVIEW_H_

#include "k3bdataview.h"

namespace K3b {
    class VideoDvdDoc;

    class VideoDvdView : public DataView
    {
        Q_OBJECT

    public:
        explicit VideoDvdView( VideoDvdDoc* doc, QWidget *parent = 0 );
        ~VideoDvdView() override;

    protected:
        ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 ) override;

        void init();

    private:
        VideoDvdDoc* m_doc;
    };
}

#endif
