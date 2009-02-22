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

#ifndef _K3B_AUDIO_CD_LISTVIEW_H_
#define _K3B_AUDIO_CD_LISTVIEW_H_

#include <k3blistview.h>

class Q3DragObject;
namespace K3b {
    class AudioCdView;
}

/**
 * Internally used by AudioCdView
 */
namespace K3b {
class AudioCdListView : public ListView
{
  Q_OBJECT

 public:
  AudioCdListView( AudioCdView*, QWidget* parent = 0 );
  ~AudioCdListView();

 protected:
  /**
   * @reimpl from K3ListView
   */
  Q3DragObject* dragObject();

 private:
  AudioCdView* m_view;
};
}

#endif
