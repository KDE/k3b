/***************************************************************************
                          k3bcddblocaldbtab.h  -  description
                             -------------------
    begin                : Mon Feb 11 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#ifndef K3BCDDBLOCALDBTAB_H
#define K3BCDDBLOCALDBTAB_H

#include <qwidget.h>
#include <qstringlist.h>

class QFrame;
class QTabWidget;
class KLineEdit;
class QMultiLineEdit;
/**
  *@author Sebastian Trueg
  */

class K3bCddbLocalDBTab : public QWidget  {
    Q_OBJECT
public: 
	K3bCddbLocalDBTab(QFrame *parent, const char *name);
	~K3bCddbLocalDBTab();
	void apply();
	void readSettings();
private:
    KLineEdit *m_songListPath;
    QMultiLineEdit *m_logOutput;
    QTabWidget *m_dbHandlingTab;
    QStringList m_missingSongList;
    void setup();
private slots:
    void browseDb();
    void clearDb();
    void verifyDb();
    void addDbEntry();
    void findDbEntries();
};

#endif
