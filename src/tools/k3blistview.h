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
class QFocusEvent;
class QValidator;
class K3bMsfEdit;

class K3bListView;


class K3bListViewItem : public KListViewItem
{
 public:
  K3bListViewItem(QListView *parent);
  K3bListViewItem(QListViewItem *parent);
  K3bListViewItem(QListView *parent, QListViewItem *after);
  K3bListViewItem(QListViewItem *parent, QListViewItem *after);

  K3bListViewItem(QListView *parent,
		  QString, QString = QString::null,
		  QString = QString::null, QString = QString::null,
		  QString = QString::null, QString = QString::null,
		  QString = QString::null, QString = QString::null);

  K3bListViewItem(QListViewItem *parent,
		  QString, QString = QString::null,
		  QString = QString::null, QString = QString::null,
		  QString = QString::null, QString = QString::null,
		  QString = QString::null, QString = QString::null);

  K3bListViewItem(QListView *parent, QListViewItem *after,
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

  void setEditor( int col, int type, const QStringList& = QStringList() );
  void setButton( int col, bool );

  int editorType( int col ) const;
  bool needButton( int col ) const;
  const QStringList& comboStrings( int col ) const;

  enum EditorType { NONE, COMBO, LINE, SPIN, MSF };

  void setFont( int col, const QFont& f );
  void setBackgroundColor( int col, const QColor& );
  void setForegroundColor( int col, const QColor& );

  virtual void paintCell( QPainter* p, const QColorGroup& cg, int col, int width, int align );

 private:
  class ColumnInfo;
  mutable ColumnInfo* m_columns;

  ColumnInfo* getColumnInfo( int ) const;
  void init();
};




class K3bListView : public KListView
{
  Q_OBJECT

 public:
  K3bListView (QWidget *parent = 0, const char *name = 0);
  virtual ~K3bListView();

  virtual void setCurrentItem( QListViewItem* );

 signals:
  void editorButtonClicked( K3bListViewItem*, int );

 public slots:
  void setNoItemText( const QString& );
  //  void setNoItemPixmap( const QPixmap& );
  void setNoItemVerticalMargin( int i ) { m_noItemVMargin = i; }
  void setNoItemHorizontalMargin( int i ) { m_noItemHMargin = i; }
  void setDoubleClickForEdit( bool b ) { m_doubleClickForEdit = b; }
  void setValidator( QValidator* v );

 private slots:
  void updateEditorSize();
  void slotClicked( QListViewItem*, const QPoint&, int );
  virtual void slotEditorLineEditReturnPressed();
  virtual void slotEditorComboBoxActivated( const QString& );
  virtual void slotEditorSpinBoxValueChanged( int );
  virtual void slotEditorMsfEditValueChanged( int );
  virtual void slotEditorButtonClicked();

 protected slots:
  void showEditor( K3bListViewItem*, int col );
  void placeEditor( K3bListViewItem*, int col );
  void hideEditor();

  /**
   * This is called whenever one of the editor's contents changes
   * the default implementation just returnes true
   */
  virtual bool renameItem( K3bListViewItem*, int, const QString& );

  /**
   * default impl just emits signal
   * editorButtonClicked(...)
   */
  virtual void slotEditorButtonClicked( K3bListViewItem*, int );

 protected:
  /**
   * calls KListView::drawContentsOffset
   * and paints a the noItemText if no item is in the list
   */
  virtual void drawContentsOffset ( QPainter * p, int ox, int oy, int cx, int cy, int cw, int ch );
  virtual void resizeEvent( QResizeEvent* );
  virtual void paintEmptyArea( QPainter*, const QRect& rect );
  virtual void focusOutEvent( QFocusEvent* );

  K3bListViewItem* currentEditItem() const { return m_currentEditItem; }
  int currentEditColumn() const { return m_currentEditColumn; }

 private:
  QWidget* prepareEditor( K3bListViewItem* item, int col );
  void prepareButton( K3bListViewItem* item, int col );

  QString m_noItemText;
  //  QPixmap m_noItemPixmap;
  int m_noItemVMargin;
  int m_noItemHMargin;

  K3bListViewItem* m_currentEditItem;
  int m_currentEditColumn;

  bool m_doubleClickForEdit;
  QListViewItem* m_lastClickedItem;

  QPushButton* m_editorButton;
  QComboBox* m_editorComboBox;
  QSpinBox* m_editorSpinBox;
  QLineEdit* m_editorLineEdit;
  K3bMsfEdit* m_editorMsfEdit;

  // TODO: think about a more universal solution!
  QValidator* m_validator;
};


#endif
