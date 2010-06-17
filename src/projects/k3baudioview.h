/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Mello <arthur@mandriva.com>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *           (C) 2009-2010 Michal Malek <michalm@jabster.pl>
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


#ifndef K3BAUDIOVIEW_H
#define K3BAUDIOVIEW_H

#include "k3bview.h"

#include <QStringList>

namespace K3b {

    class AudioDoc;
    class AudioTrack;
    class AudioViewImpl;
    class ViewColumnAdjuster;

    class AudioView : public View
    {
        Q_OBJECT

    public:
        AudioView( AudioDoc* doc, QWidget* parent );
        ~AudioView();

    public Q_SLOTS:
        virtual void addUrls( const KUrl::List& urls );

    protected:
        virtual ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

    private:
        AudioDoc* m_doc;
        AudioViewImpl* m_audioViewImpl;
    };
}

#endif
