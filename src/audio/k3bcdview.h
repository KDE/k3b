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
#include <qvbox.h>
class QString;
struct cdrom_drive;
class K3bCddb;
class K3bCdda;
class K3bCddaCopy;
class KListView;
/**
  *@author Sebastian Trueg
  */

class K3bCdView : public QVBox  {
	Q_OBJECT
public:
	K3bCdView(QWidget *, const char *);
	~K3bCdView();
	void showCdContent(struct cdrom_drive *drive);
	void closeDrive();
	void show();
	class CopyFiles;
public slots:
	/** */
	void showCdView(QString device);
private:
    struct cdrom_drive *m_drive;
    K3bCddb *m_cddb;
    K3bCdda *m_cdda;
    K3bCddaCopy *m_copy;
    KListView *m_listView;
    QString m_device;
    bool m_initialized;
    void addItem(int, QString, QString, QString, long, QString);
    QString prepareFilename(QString);
    void grab();
    void setupGUI();

private slots: // Private slots
  /** No descriptions */
  // Toolbar Button actions
  void reload();
  void prepareRipping();
  void changeSelectionMode();

  void applyOptions();
  struct cdrom_drive* pickDrive( QString device);

};

class K3bCdView::CopyFiles
{
public:
    CopyFiles( int _track, QString _filename) : m_track(_track), m_filename(_filename) {} ;
    int getTrack(){ return m_track; };
    QString getFilename(){ return m_filename; };
private:
    int m_track;
    QString m_filename;
}; // class CopyFiles

#endif
