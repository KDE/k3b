/***************************************************************************
                              k3blistview.cpp
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


#include "k3blistview.h"

#include <qstringlist.h>
#include <qfontmetrics.h>
#include <qpainter.h>
#include <qheader.h>
#include <qrect.h>
#include <qpushbutton.h>
#include <qiconset.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qlineedit.h>



// ///////////////////////////////////////////////
//
// K3BLISTVIEWITEM
//
// ///////////////////////////////////////////////


class K3bListViewItem::ColumnInfo
{
public:
  ColumnInfo() {
    editorType = NONE;
    button = false;
    comboEditable = false;
  }

  bool button;
  int editorType;
  QStringList comboItems;
  bool comboEditable;
};



K3bListViewItem::K3bListViewItem(K3bListView *parent)
  : KListViewItem( parent )
{ 
  init();
}

K3bListViewItem::K3bListViewItem(QListViewItem *parent)
  : KListViewItem( parent )
{ 
  init();
}

K3bListViewItem::K3bListViewItem(K3bListView *parent, QListViewItem *after)
  : KListViewItem( parent, after )
{ 
  init();
}

K3bListViewItem::K3bListViewItem(QListViewItem *parent, QListViewItem *after)
  : KListViewItem( parent, after )
{ 
  init();
}


K3bListViewItem::K3bListViewItem(K3bListView *parent,
				 QString s1, QString s2,
				 QString s3, QString s4,
				 QString s5, QString s6,
				 QString s7, QString s8)
  : KListViewItem( parent, s1, s2, s3, s4, s5, s6, s7, s8 )
{ 
  init();
}


K3bListViewItem::K3bListViewItem(QListViewItem *parent,
				 QString s1, QString s2,
				 QString s3, QString s4,
				 QString s5, QString s6,
				 QString s7, QString s8)
  : KListViewItem( parent, s1, s2, s3, s4, s5, s6, s7, s8 )
{ 
  init();
}


K3bListViewItem::K3bListViewItem(K3bListView *parent, QListViewItem *after,
				 QString s1, QString s2,
				 QString s3, QString s4,
				 QString s5, QString s6,
				 QString s7, QString s8)
  : KListViewItem( parent, after, s1, s2, s3, s4, s5, s6, s7, s8 )
{ 
  init();
}


K3bListViewItem::K3bListViewItem(QListViewItem *parent, QListViewItem *after,
				 QString s1, QString s2,
				 QString s3, QString s4,
				 QString s5, QString s6,
				 QString s7, QString s8)
  : KListViewItem( parent, after, s1, s2, s3, s4, s5, s6, s7, s8 )
{ 
  init();
}


K3bListViewItem::~K3bListViewItem()
{
  delete m_editorButton;
  delete m_editorComboBox;
  delete m_editorSpinBox;
  delete m_editorLineEdit;
  m_columns.setAutoDelete( true );
}

void K3bListViewItem::setEditor( int column, int editor, const QStringList& cs )
{
  createColumnInfo(column);

  m_columns.at(column)->editorType = editor;
  if( !cs.isEmpty() )
    m_columns.at(column)->comboItems = cs;
}


void K3bListViewItem::setButton( int column, bool on )
{
  createColumnInfo(column);

  m_columns.at(column)->button = on;
}


void K3bListViewItem::createColumnInfo( int col )
{
  // create ColumnInfos up to column col
}

void K3bListViewItem::showEditor( int col )
{
  if( col >= 0 )
    m_editorColumn = col;

  placeEditor();

  if( m_columns.size() > col ) {
    switch( m_columns.at(col)->editorType ) {
    case COMBO:
      m_editorComboBox->show();
      break;
    case LINE:
      m_editorLineEdit->show();
      break;
    case SPIN:
      m_editorSpinBox->show();
      break;
    default:
      break;
    }
  }
}

void K3bListViewItem::hideEditor()
{
  if( m_editorSpinBox )
    m_editorSpinBox->hide();
  if( m_editorLineEdit )
    m_editorLineEdit->hide();
  if( m_editorComboBox )
    m_editorComboBox->hide();
}

void K3bListViewItem::placeEditor()
{
//   QRect r = listView()->itemRect( this );
//   if ( !r.size().isValid() ) {
//     listView()->ensureItemVisible( this );
//     r = listView()->itemRect( this );
//   }

//   r.setX( listView()->header()->sectionPos( m_editorColumn ) );
//   r.setWidth( listView()->header()->sectionSize( m_editorColumn ) - 1 );
//   r = QRect( listView()->viewportToContents( r.topLeft() ), r.size() );

//   if( m_editorButton ) {
//     m_editorButton->setFixedHeight( r.height() );
//     int ww = m_editorButton->sizeHint().width();
//     if( ww <= r.width() )
//       m_editorButton->setFixedWidth( ww );
//     else
//       m_editorButton->setFixedWidth( r.width() );
//     r.setWidth( r.width() - m_editorButton->width() );
//     listView()->moveChild( m_editorButton, r.right(), r.y() );
//   }


//   if( m_editor ) {
//     m_editor->resize( r.size() );
//     listView()->moveChild( m_editor, r.x(), r.y() );
//   }
}


void K3bListViewItem::init()
{
  m_editorComboBox = 0;
  m_editorSpinBox = 0;
  m_editorLineEdit = 0;
  m_editorButton = 0;
  m_editorColumn = 1;
}



// ///////////////////////////////////////////////
//
// K3BLISTVIEW
//
// ///////////////////////////////////////////////


K3bListView::K3bListView( QWidget* parent, const char* name )
  : KListView( parent, name ),
    m_noItemVMargin( 20 ),
    m_noItemHMargin( 20 )
{
  connect( header(), SIGNAL( sizeChange( int, int, int ) ),
	   this, SLOT( updateEditorSize() ) );
}

K3bListView::~K3bListView()
{
}


void K3bListView::setCurrentItem( QListViewItem* i )
{
  if( !i )
    return;

  if( K3bListViewItem* ki = dynamic_cast<K3bListViewItem*>(currentItem()) )
    ki->hideEditor();
  KListView::setCurrentItem( i );
  updateEditorSize();
}


void K3bListView::setNoItemText( const QString& text )
{
  m_noItemText = text;
  triggerUpdate();
}


// void K3bListView::drawContentsOffset( QPainter * p, int ox, int oy, int cx, int cy, int cw, int ch )
// {
//   KListView::drawContentsOffset( p, ox, oy, cx, cy, cw, ch );

//   if( childCount() == 0 && !m_noItemText.isEmpty()) {

//     p->setPen( Qt::darkGray );

//     QStringList lines = QStringList::split( "\n", m_noItemText );
//     int xpos = m_noItemHMargin;
//     int ypos = m_noItemVMargin + p->fontMetrics().height();

//     for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
//       p->drawText( xpos, ypos, *str );
//       ypos += p->fontMetrics().lineSpacing();
//     }
//   }
// }

void K3bListView::paintEmptyArea( QPainter* p, const QRect& rect )
{
  KListView::paintEmptyArea( p, rect );

  if( childCount() == 0 && !m_noItemText.isEmpty()) {

    p->fillRect( viewport()->rect(), viewport()->paletteBackgroundColor() );

    p->setPen( Qt::darkGray );

    QStringList lines = QStringList::split( "\n", m_noItemText );
    int xpos = m_noItemHMargin;
    int ypos = m_noItemVMargin + p->fontMetrics().height();

    for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
      p->drawText( xpos, ypos, *str );
      ypos += p->fontMetrics().lineSpacing();
    }
  }
}

void K3bListView::resizeEvent( QResizeEvent* e )
{
  KListView::resizeEvent( e );
  updateEditorSize();
}


void K3bListView::updateEditorSize()
{
  if( K3bListViewItem* ki = dynamic_cast<K3bListViewItem*>(currentItem()) )
    ki->showEditor();
}


#include "k3blistview.moc"
