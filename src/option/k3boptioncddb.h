/***************************************************************************
                          k3boptioncddb.h  -  description
                             -------------------
    begin                : Fri Nov 2 2001
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

#ifndef K3BOPTIONCDDB_H
#define K3BOPTIONCDDB_H

#include <qwidget.h>

class QFrame;
class QRadioButton;
class QGroupBox;
class QPushButton;
class QListBoxItem;
class QString;

class KLineEdit;
class KListBox;
/**
  *@author Sebastian Trueg
  */

class K3bOptionCddb : public QWidget {
	Q_OBJECT
public: 
	K3bOptionCddb(QFrame *parent, const char *name);
	~K3bOptionCddb();
   void apply();
   void readSettings();
private slots:
	void toggled(bool);
	void addCddbServer();
	void delCddbServer();
	void serverSelected(QListBoxItem*);
private:
   QFrame *m_frame;
	QRadioButton *m_cddbLockup;
	QPushButton *m_addButton;
	QPushButton *m_delButton;
	KLineEdit *m_cddbServerInput;
	KLineEdit *m_cddbPortInput;
	KListBox *m_cddbServerList;
	QGroupBox *m_groupCddbServer;
   void setup();
   void fillInputFields(QString hostString);

};

#endif
