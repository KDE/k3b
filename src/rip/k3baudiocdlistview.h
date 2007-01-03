/* 
 *
 * $Id$
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

class QDragObject;
class K3bAudioCdView;

/**
 * Internally used by K3bAudioCdView
 */
class K3bAudioCdListView : public K3bListView
{
  Q_OBJECT

 public:
  K3bAudioCdListView( K3bAudioCdView*, QWidget* parent = 0, const char* name = 0 );
  ~K3bAudioCdListView();

 protected:
  /**
   * @reimpl from KListView
   */
  QDragObject* dragObject();

 private:
  K3bAudioCdView* m_view;
};

#endif
