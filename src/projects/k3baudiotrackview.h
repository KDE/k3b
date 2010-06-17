/*
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_AUDIO_TRACK_VIEW_H_
#define _K3B_AUDIO_TRACK_VIEW_H_

#include <QtGui/QTreeView>


namespace K3b {

    class AudioDoc;

    class AudioTrackView : public QTreeView
    {
        Q_OBJECT

    public:
        AudioTrackView( AudioDoc* doc, QWidget* parent = 0 );
        ~AudioTrackView();

    private:
        AudioDoc* m_doc;
    };
}

#endif
