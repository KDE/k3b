/***************************************************************************
                          audiolistview.h  -  description
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

#ifndef AUDIOLISTVIEW_H
#define AUDIOLISTVIEW_H


#include <klistview.h>

class QDragEnterEvent;

/**
  *@author Sebastian Trueg
  */

class AudioListView : public KListView  {
   Q_OBJECT

public:
	AudioListView(QWidget *parent=0, const char *name=0);
	~AudioListView();

private:
  void setupColumns();

protected:
  bool acceptDrag(QDropEvent* e) const;
};

#endif
