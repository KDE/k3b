/* 
 *
 * $Id: $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3B_EXTERNAL_BIN_WIDGET_H
#define K3B_EXTERNAL_BIN_WIDGET_H


#include <qwidget.h>
#include <qptrlist.h>


class K3bExternalBinManager;
class QPushButton;
class KListView;
class QTabWidget;
class KEditListBox;
class QListViewItem;


class K3bExternalBinWidget : public QWidget
{
  Q_OBJECT

 public:
  K3bExternalBinWidget( K3bExternalBinManager*, QWidget* parent = 0, const char* name = 0 );
  ~K3bExternalBinWidget();

 public slots:
  void rescan();
  void load();
  void save();

 private slots:
  void slotSetDefaultButtonClicked();
  void slotProgramSelectionChanged( QListViewItem* );
  void saveSearchPath();

 private:
  K3bExternalBinManager* m_manager;

  QTabWidget* m_mainTabWidget;
  KListView* m_programView;
  KListView* m_parameterView;
  KEditListBox* m_searchPathBox;

  QPushButton* m_defaultButton;
  QPushButton* m_rescanButton;

  class K3bExternalBinViewItem;
  class K3bExternalProgramViewItem;

  QPtrList<K3bExternalProgramViewItem> m_programRootItems;
};


#endif
