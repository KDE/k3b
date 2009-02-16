/* 
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Mello <arthur@mandriva.com>
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

#include <k3bstandardview.h>
#include "k3baudiotrackview.h"

#include <qstringlist.h>


class K3bAudioDoc;
class K3bAudioTrack;
//class K3bAudioTrackView;

namespace K3b {
	class AudioProjectModel;
}

/**
 *@author Sebastian Trueg
 */
class K3bAudioView : public K3bStandardView
{
    Q_OBJECT
	
public: 
    K3bAudioView( K3bAudioDoc* pDoc, QWidget* parent );
    ~K3bAudioView();

    //K3bAudioTrackPlayer* player() const { return m_songlist->player(); }
    K3bAudioTrackPlayer* player() const { return 0; }

public Q_SLOTS:
    void addUrls( const KUrl::List& );

protected:
    K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

    void init();

    private Q_SLOTS:
    void slotAudioConversion();

private:
    K3bAudioDoc* m_doc;
	
    //K3bAudioTrackView* m_songlist;
	K3b::AudioProjectModel* m_model;
};

#endif
