/*
 *
 * Copyright (C) 2005-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
 *           (C) 2009      Michal Malek <michalm@jabster.pl>
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
