/***************************************************************************
                          k3bdvdriplistviewitem.cpp  -  description
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

#include "k3bdvdriplistviewitem.h"
#include <qlistview.h>
#include <qstring.h>
#include <klistview.h>


K3bDvdRipListViewItem::K3bDvdRipListViewItem(QListView *parent) : KListViewItem(parent) {
    m_isTitleSet = false;
}
K3bDvdRipListViewItem::K3bDvdRipListViewItem(QListView*parent,QString label1, QString label2, QString label3, QString label4, QString label5, QString label6, QString label7, QString label8 ) :
    KListViewItem( parent,label1, label2, label3, label4, label5, label6, label7, label8 ){
    m_isTitleSet = false;
}
K3bDvdRipListViewItem::K3bDvdRipListViewItem(QListViewItem *parent,QString label1, QString label2, QString label3, QString label4, QString label5, QString label6, QString label7, QString label8 ) :
    KListViewItem( parent,label1, label2, label3, label4, label5, label6, label7, label8 ){
    m_isTitleSet = false;
}
K3bDvdRipListViewItem::~K3bDvdRipListViewItem(){
}
void K3bDvdRipListViewItem::setHiddenTitle( int t ){
    m_title=t;
}
int K3bDvdRipListViewItem::getHiddenTitle(  ){
    return m_title;
}
