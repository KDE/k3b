/***************************************************************************
                          k3bcdview.h  -  description
                             -------------------
    begin                : Sun Oct 28 2001
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

#ifndef K3BCDVIEW_H
#define K3BCDVIEW_H

#include <qwidget.h>

class QString;
struct cdrom_drive;
class K3bCddb;
class KListView;
/**
  *@author Sebastian Trueg
  */

class K3bCdView : public QWidget  {
	Q_OBJECT
public:
	K3bCdView(QWidget *, const char *);
	~K3bCdView();
	void showCdContent(struct cdrom_drive *drive);
	void closeDrive();
public slots:
	/** */
	void showCdView(QString device);
private:
	struct cdrom_drive *m_drive;
	K3bCddb *m_cddb;
	KListView *m_listView;
	QString m_device;
	void addItem(int, QString, QString, QString, long, QString);
	QString prepareFilename(QString);
private slots: // Private slots
  /** No descriptions */
  void reload();
  void grab();
  void applyOptions();

};

#endif
