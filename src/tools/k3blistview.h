/***************************************************************************
                              k3blistview.h
                                   -
A KlistView with feature to display text and pixmap if no item is in the view.

                             -------------------
    begin                : Wed Sep  4 12:56:10 CEST 2002
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

#ifndef K3BLISTVIEW_H
#define K3BLISTVIEW_H


#include <klistview.h>

#include <qptrvector.h>
#include <qstringlist.h>

class QPainter;
class QPushButton;
class QIconSet;
class QResizeEvent;
class QComboBox;
class QSpinBox;
class QLineEdit;



class K3bListView : public KListView
{
  Q_OBJECT

 public:
  K3bListView (QWidget *parent = 0, const char *name = 0);
  virtual ~K3bListView();

  virtual void setCurrentItem( QListViewItem* );

 public slots:
  void setNoItemText( const QString& );
  //  void setNoItemPixmap( const QPixmap& );
  void setNoItemVerticalMargin( int i ) { m_noItemVMargin = i; }
  void setNoItemHorizontalMargin( int i ) { m_noItemHMargin = i; }

 public slots:
  void updateEditorSize();

 protected:
  /**
   * calls KListView::drawContentsOffset
   * and paints a the noItemText if no item is in the list
   */
 //  virtual void drawContentsOffset ( QPainter * p, int ox, int oy, int cx, int cy, int cw, int ch );
  virtual void resizeEvent( QResizeEvent* );
  virtual void paintEmptyArea( QPainter*, const QRect& rect );

 private:
  QString m_noItemText;
  //  QPixmap m_noItemPixmap;
  int m_noItemVMargin;
  int m_noItemHMargin;
};



class K3bListViewItem : public KListViewItem
{
 public:
  K3bListViewItem(K3bListView *parent);
  K3bListViewItem(QListViewItem *parent);
  K3bListViewItem(K3bListView *parent, QListViewItem *after);
  K3bListViewItem(QListViewItem *parent, QListViewItem *after);

  K3bListViewItem(K3bListView *parent,
		  QString, QString = QString::null,
		  QString = QString::null, QString = QString::null,
		  QString = QString::null, QString = QString::null,
		  QString = QString::null, QString = QString::null);

  K3bListViewItem(QListViewItem *parent,
		  QString, QString = QString::null,
		  QString = QString::null, QString = QString::null,
		  QString = QString::null, QString = QString::null,
		  QString = QString::null, QString = QString::null);

  K3bListViewItem(K3bListView *parent, QListViewItem *after,
		  QString, QString = QString::null,
		  QString = QString::null, QString = QString::null,
		  QString = QString::null, QString = QString::null,
		  QString = QString::null, QString = QString::null);

  K3bListViewItem(QListViewItem *parent, QListViewItem *after,
		  QString, QString = QString::null,
		  QString = QString::null, QString = QString::null,
		  QString = QString::null, QString = QString::null,
		  QString = QString::null, QString = QString::null);

  virtual ~K3bListViewItem();

  void setEditor( int, int, const QStringList& = QStringList() );
  void setButton( int, bool );

  void showEditor( int col = -1 );
  void hideEditor();
  void placeEditor();

 private:
  void init();
  void createColumnInfo( int );

  class ColumnInfo;
  QPtrVector<ColumnInfo> m_columns;
  enum EditorType { NONE, COMBO, LINE, SPIN };

  QPushButton* m_editorButton;
  QComboBox* m_editorComboBox;
  QSpinBox* m_editorSpinBox;
  QLineEdit* m_editorLineEdit;
  int m_editorColumn;
};


#endif
