/* 
 *
 * $Id: $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef AUDIOLISTVIEWITEM_H
#define AUDIOLISTVIEWITEM_H

#include "../tools/k3blistview.h"

class K3bAudioTrack;


/**
  *@author Sebastian Trueg
  */

class K3bAudioListViewItem : public K3bListViewItem  {

 public:
  K3bAudioListViewItem( K3bAudioTrack* track, K3bListView* parent );
  K3bAudioListViewItem( K3bAudioTrack* track, K3bListView* parent, QListViewItem* after );
  ~K3bAudioListViewItem();

  /** reimplemented from QListViewItem */
  QString text(int i) const;
	
  /** reimplemented from QListViewItem */
  void setText(int col, const QString& text );
	
  /** reimplemented from QListViewItem */
  QString key( int column, bool a ) const;
	
  K3bAudioTrack* audioTrack() { return m_track; }

  int animationIconNumber;
		
 private:
  void init();
  K3bAudioTrack* m_track;
};

#endif
