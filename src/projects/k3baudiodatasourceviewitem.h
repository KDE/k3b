/* 
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_AUDIO_DATA_SOURCE_VIEWITEM_H_
#define _K3B_AUDIO_DATA_SOURCE_VIEWITEM_H_

#include <k3blistview.h>

namespace K3b {
    class AudioTrack;
}
namespace K3b {
    class AudioDataSource;
}
namespace K3b {
    class AudioTrackViewItem;
}

namespace K3b {
class AudioDataSourceViewItem : public ListViewItem
{
 public:
  AudioDataSourceViewItem( AudioTrackViewItem* parent, 
			      AudioDataSourceViewItem* after, 
			      AudioDataSource* );

  AudioDataSource* source() const { return m_source; }
  AudioTrackViewItem* trackViewItem() const { return m_trackViewItem; }

  QString text( int i ) const;
  void setText( int col, const QString& text );

  bool animate();

  void setSelected( bool s );

  /**
   * Does nothing becasue we don't want no branches here.
   */
  void paintBranches( QPainter*, const QColorGroup &,
		      int, int, int ) {}

 private:
  AudioTrackViewItem* m_trackViewItem;
  AudioDataSource* m_source;
  int m_animationCounter;
};
}

#endif
