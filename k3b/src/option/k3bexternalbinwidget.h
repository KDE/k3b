/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
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

#include <klistview.h>


class K3bExternalBinManager;
class QPushButton;
class KListView;
class QTabWidget;
class KEditListBox;
class QListViewItem;
class K3bExternalProgram;
class K3bExternalBin;


class K3bExternalBinWidget : public QWidget
{
  Q_OBJECT

 public:
  K3bExternalBinWidget( K3bExternalBinManager*, QWidget* parent = 0, const char* name = 0 );
  ~K3bExternalBinWidget();

  class K3bExternalBinViewItem;
  class K3bExternalProgramViewItem;

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

  QPtrList<K3bExternalProgramViewItem> m_programRootItems;
};


class K3bExternalBinWidget::K3bExternalProgramViewItem : public KListViewItem
{
 public:
  K3bExternalProgramViewItem( K3bExternalProgram* p, QListView* parent );
  
  K3bExternalProgram* program() const { return m_program; }
  
 protected:
  void paintCell( QPainter* p, const QColorGroup& cg, int column, int width, int align );
  
 private:
  K3bExternalProgram* m_program;
};



class K3bExternalBinWidget::K3bExternalBinViewItem : public KListViewItem
{
 public:
  K3bExternalBinViewItem( K3bExternalBin* bin, K3bExternalProgramViewItem* parent );

  K3bExternalBin* bin() const { return m_bin; }
  K3bExternalProgramViewItem* parentProgramItem() const { return m_parent; }

  bool isDefault() const { return m_default; }
  void setDefault( bool b );

 private:
  K3bExternalBin* m_bin;
  K3bExternalProgramViewItem* m_parent;

  bool m_default;
};


#endif
