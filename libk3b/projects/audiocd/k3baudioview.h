/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BAUDIOVIEW_H
#define K3BAUDIOVIEW_H

#include <k3bview.h>

#include <qstringlist.h>


class K3bAudioDoc;
class K3bAudioTrack;
class K3bAudioTrackView;
class K3bAudioTrackPlayer;


/**
  *@author Sebastian Trueg
  */
class K3bAudioView : public K3bView
{
  Q_OBJECT
	
 public: 
  K3bAudioView( K3bAudioDoc* pDoc, QWidget* parent, const char *name = 0 );
  ~K3bAudioView();

 private:
  K3bAudioDoc* m_doc;
	
  K3bAudioTrackView* m_songlist;
  K3bAudioTrackPlayer* m_player;
};

#endif
