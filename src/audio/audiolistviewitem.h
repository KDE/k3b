/***************************************************************************
                          audiolistviewitem.h  -  description
                             -------------------
    begin                : Tue Mar 27 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
