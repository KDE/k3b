/* This file is part of the KDE libraries
   Copyright (C) 2000 Charles Samuels <charles@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef KLISTVIEWLINEEDIT_H
#define KLISTVIEWLINEEDIT_H

#include <klineedit.h>
#include <klistview.h>

/**
 * the editor for a KListView.  please don't use this.
 * @internal
 **/
class KListViewLineEdit : public KLineEdit
{
Q_OBJECT
public:
	KListViewLineEdit(KListView *parent);
	~KListViewLineEdit();

signals:
	void done(QListViewItem*, int);

public slots:
	void terminate();
	void load(QListViewItem *i, int c);

protected:
	virtual void focusOutEvent(QFocusEvent *);
	virtual void keyPressEvent(QKeyEvent *e);
	virtual void paintEvent(QPaintEvent *e);
	void terminate(bool commit);
	QListViewItem *item;
	int col;
	KListView *p;

protected slots:
	void slotSelectionChanged();

};

#endif
