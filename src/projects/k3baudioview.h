/*

    SPDX-FileCopyrightText: 2003-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2009 Arthur Mello <arthur@mandriva.com>
    SPDX-FileCopyrightText: 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
    SPDX-FileCopyrightText: 2009-2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
        ~AudioView() override;

    public Q_SLOTS:
        void addUrls( const QList<QUrl>& urls ) override;

    protected:
        ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 ) override;

    private Q_SLOTS:
        void slotPlayerStateChanged();

    private:
        AudioDoc* m_doc;
        AudioViewImpl* m_audioViewImpl;
    };
}

#endif
