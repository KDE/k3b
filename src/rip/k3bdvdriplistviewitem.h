/***************************************************************************
                          k3bdvdriplistviewitem.h  -  description
                             -------------------
    begin                : Tue May 14 2002
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

#ifndef K3BDVDRIPLISTVIEWITEM_H
#define K3BDVDRIPLISTVIEWITEM_H

#include <qlistview.h>
#include <klistview.h>
#include <qstring.h>

/**
  *@author Sebastian Trueg
  */

class K3bDvdRipListViewItem : public KListViewItem  {
public:
    K3bDvdRipListViewItem(QListView *parent);
    K3bDvdRipListViewItem(QListView *parent,QString label1, QString label2=QString::null, QString label3=QString::null, QString label4=QString::null, QString label5=QString::null, QString label6=QString::null, QString label7=QString::null, QString label8=QString::null );
    K3bDvdRipListViewItem(QListViewItem *parent,QString label1, QString label2=QString::null, QString label3=QString::null, QString label4=QString::null, QString label5=QString::null, QString label6=QString::null, QString label7=QString::null, QString label8=QString::null );
    ~K3bDvdRipListViewItem();
    void setHiddenTitle( int t );
    int getHiddenTitle();
    void setTitleSet( bool b){ m_isTitleSet=b;}
    bool isTitleSetEntry(){ return m_isTitleSet; }
private:
    bool m_isTitleSet;
    int m_title;
};

#endif
