/***************************************************************************
                          k3bsetup.h  -  Main Wizard Widget
                             -------------------
    begin                : Sun Aug 25 13:19:59 CEST 2002
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

#ifndef K3BSETUP2_H
#define K3BSETUP2_H

#include <kdialog.h>

class QWidgetStack;
class KPushButton;
class QLabel;
class K3bListView;
class K3bListViewItem;

class K3bSetup2Page;



class K3bSetup2 : public KDialog
{
  Q_OBJECT

 public:
  K3bSetup2( QWidget* parent = 0, const char* name = 0, bool modal = true, WFlags f = 0 );
  ~K3bSetup2();

  void addPage( K3bSetup2Page*, int id );

 public slots:
  void init();

 protected slots:
  void accept();
  void close();
  void next();
  void back();
  void showPage( int );
  void slotTaskViewButtonClicked(K3bListViewItem*, int );

 private:
  void setupGui();
  void setupPages();

  K3bListView* m_taskView;
  QWidgetStack* m_pageStack;

  KPushButton* m_nextButton;
  KPushButton* m_backButton;
  KPushButton* m_closeButton;
  KPushButton* m_finishButton;

  QLabel* m_headerLabel;
  
  int m_numberOfPages;
  int m_visiblePageIndex;
};


#endif
