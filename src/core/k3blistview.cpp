/* 
 *
 * $Id$
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



#include "k3blistview.h"

#include "k3bmsfedit.h"

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
#include <qevent.h>
#include <qvalidator.h>
#include <qfont.h>
#include <qpalette.h>
#include <qstyle.h>
#include <qapplication.h>

#include <limits.h>


// ///////////////////////////////////////////////
//
// K3BLISTVIEWITEM
//
// ///////////////////////////////////////////////


class K3bListViewItem::ColumnInfo
{
public:
  ColumnInfo()
    : showProgress(false),
      progressValue(0) {
    editorType = NONE;
    button = false;
    comboEditable = false;
    next = 0;
    fontSet = false;
    backgroundColorSet = false;
    foregroundColorSet = false;
  }

  ~ColumnInfo() {
    if( next )
      delete next;
  }

  bool button;
  int editorType;
  QStringList comboItems;
  bool comboEditable;
  bool fontSet;
  bool backgroundColorSet;
  bool foregroundColorSet;
  QFont font;
  QColor backgroundColor;
  QColor foregroundColor;
  ColumnInfo* next;

  bool showProgress;
  int progressValue;
};



K3bListViewItem::K3bListViewItem(QListView *parent)
  : KListViewItem( parent )
{ 
  init();
}

K3bListViewItem::K3bListViewItem(QListViewItem *parent)
  : KListViewItem( parent )
{ 
  init();
}

K3bListViewItem::K3bListViewItem(QListView *parent, QListViewItem *after)
  : KListViewItem( parent, after )
{ 
  init();
}

K3bListViewItem::K3bListViewItem(QListViewItem *parent, QListViewItem *after)
  : KListViewItem( parent, after )
{ 
  init();
}


K3bListViewItem::K3bListViewItem(QListView *parent,
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


K3bListViewItem::K3bListViewItem(QListView *parent, QListViewItem *after,
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
  if( K3bListView* lv = dynamic_cast<K3bListView*>(listView()) )
    if( lv->currentlyEditedItem() == this )
      lv->hideEditor();

  if( m_columns )
    delete m_columns;
}

void K3bListViewItem::setEditor( int column, int editor, const QStringList& cs )
{
  ColumnInfo* colInfo = getColumnInfo(column);

  colInfo->editorType = editor;
  if( !cs.isEmpty() )
    colInfo->comboItems = cs;
}


void K3bListViewItem::setButton( int column, bool on )
{
  ColumnInfo* colInfo = getColumnInfo(column);

  colInfo->button = on;
}


K3bListViewItem::ColumnInfo* K3bListViewItem::getColumnInfo( int col ) const
{
  if( !m_columns )
    m_columns = new ColumnInfo();

  ColumnInfo* info = m_columns;
  int i = 0;
  while( i < col ) {
    if( !info->next )
      info->next = new ColumnInfo();
    info = info->next;
    ++i;
  }

  return info;
}


void K3bListViewItem::init()
{
  m_columns = 0;
}


int K3bListViewItem::editorType( int col ) const
{
  ColumnInfo* info = getColumnInfo( col );
  return info->editorType;
}


bool K3bListViewItem::needButton( int col ) const
{
  ColumnInfo* info = getColumnInfo( col );
  return info->button;
}


const QStringList& K3bListViewItem::comboStrings( int col ) const
{
  ColumnInfo* info = getColumnInfo( col );
  return info->comboItems;
}


void K3bListViewItem::setFont( int col, const QFont& f )
{
  ColumnInfo* info = getColumnInfo( col );
  info->fontSet = true;
  info->font = f;
}


void K3bListViewItem::setBackgroundColor( int col, const QColor& c )
{
  ColumnInfo* info = getColumnInfo( col );
  info->backgroundColorSet = true;
  info->backgroundColor = c;
}


void K3bListViewItem::setForegroundColor( int col, const QColor& c )
{
 ColumnInfo* info = getColumnInfo( col );
 info->foregroundColorSet = true;
 info->foregroundColor = c;
}


void K3bListViewItem::setDisplayProgressBar( int col, bool displ )
{
  ColumnInfo* info = getColumnInfo( col );
  info->showProgress = displ;
}


void K3bListViewItem::setProgress( int col, int p )
{
  setDisplayProgressBar( col, true );
  ColumnInfo* info = getColumnInfo( col );
  info->progressValue = p;

  repaint();
}


void K3bListViewItem::paintCell( QPainter* p, const QColorGroup& cg, int col, int width, int align )
{
  ColumnInfo* info = getColumnInfo( col );

  QFont oldFont( p->font() );
  QFont newFont = info->fontSet ? info->font : oldFont;
  p->setFont( newFont );
  QColorGroup cgh(cg);
  if( info->foregroundColorSet )
    cgh.setColor( QColorGroup::Text, info->foregroundColor );
  if( info->backgroundColorSet )
    cgh.setColor( QColorGroup::Base, info->backgroundColor );

  if( info->showProgress ) {

    QStyle::SFlags flags = QStyle::Style_Default;
    if( listView()->isEnabled() )
        flags |= QStyle::Style_Enabled;
    if( listView()->hasFocus() )
        flags |= QStyle::Style_HasFocus;

    //
    // Since the QStyle API is so unflexible we need to copy the functionality of
    // QStyle::drawControl :(
    //

    // the QPainter is translated so 0, 0 is the upper left of our paint rect
    QRect r( 0, 0, width, height() );

    // clear the background (we cannot use paintEmptyArea since it's protected in QListView)
    if( K3bListView* lv = dynamic_cast<K3bListView*>(listView()) )
      lv->paintEmptyArea( p, r );
    else
      p->fillRect( 0, 0, width, height(), 
		   cgh.brush( QPalette::backgroundRoleFromMode(listView()->viewport()->backgroundMode()) ) );

    // Correct the highlight color if same as background,
    // or else we cannot see the progress...
    if( cgh.highlight() == cgh.background() )
      cgh.setColor( QColorGroup::Highlight, listView()->palette().active().highlight() );

    // draw the contents
    bool reverse = QApplication::reverseLayout();
    int fw = 2;
    int w = r.width() - 2*fw;

    const int unit_width = listView()->style().pixelMetric( QStyle::PM_ProgressBarChunkWidth );
    int u;
    if ( unit_width > 1 )
      u = (r.width()+unit_width/3) / unit_width;
    else
      u = w / unit_width;
    int p_v = info->progressValue;
    int t_s = 100;

    if ( u > 0 && p_v >= INT_MAX / u && t_s >= u ) {
      // scale down to something usable.
      p_v /= u;
      t_s /= u;
    }

    // nu < tnu, if last chunk is only a partial chunk
    int tnu, nu;
    tnu = nu = p_v * u / t_s;
    
    if (nu * unit_width > w)
      nu--;

    // Draw nu units out of a possible u of unit_width
    // width, each a rectangle bordered by background
    // color, all in a sunken panel with a percentage text
    // display at the end.
    int x = 0;
    int x0 = reverse ? r.right() - ((unit_width > 1) ?
				    unit_width : fw) : r.x() + fw;
    for (int i=0; i<nu; i++) {
      listView()->style().drawPrimitive( QStyle::PE_ProgressBarChunk, p,
					 QRect( x0+x, r.y(), unit_width, r.height() ),
					 cgh, QStyle::Style_Default );
      x += reverse ? -unit_width: unit_width;
    }

    // Draw the last partial chunk to fill up the
    // progressbar entirely
    if (nu < tnu) {
      int pixels_left = w - (nu*unit_width);
      int offset = reverse ? x0+x+unit_width-pixels_left : x0+x;
      listView()->style().drawPrimitive( QStyle::PE_ProgressBarChunk, p,
					 QRect( offset, r.y(), pixels_left,
						r.height() ), cgh, QStyle::Style_Default );
    }

    // draw the label
    QColor penColor = cgh.highlightedText();
    QColor *pcolor = 0;
    if( p_v*2 >= 100 )
      pcolor = &penColor;
    listView()->style().drawItem(p, r, AlignCenter | SingleLine, cgh, flags & QStyle::Style_Enabled, 0,
				 QString( "%1 %" ).arg(info->progressValue), -1, pcolor );
  }
  else
    KListViewItem::paintCell( p, cgh, col, width, align );

  p->setFont( oldFont );
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
  connect( this, SIGNAL(clicked(QListViewItem*, const QPoint&, int)),
	   this, SLOT(slotClicked(QListViewItem*, const QPoint&, int)) );

  m_editorButton = 0;
  m_editorComboBox = 0;
  m_editorSpinBox = 0;
  m_editorLineEdit = 0;
  m_editorMsfEdit = 0;
  m_currentEditItem = 0;
  m_currentEditColumn = 0;
  m_doubleClickForEdit = true;
  m_lastClickedItem = 0;
  m_validator = 0;
}

K3bListView::~K3bListView()
{
  // FIXME: is this really necessary or does viewport() delete them?
  delete m_editorButton;
  delete m_editorComboBox;
  delete m_editorSpinBox;
  delete m_editorLineEdit;
  delete m_editorMsfEdit;
}


void K3bListView::slotClicked( QListViewItem* item, const QPoint&, int col )
{
  if( K3bListViewItem* k3bItem = dynamic_cast<K3bListViewItem*>(item) ) {
    if( m_lastClickedItem == item || !m_doubleClickForEdit )
      showEditor( k3bItem, col );
    else
      hideEditor();
  }
  else
    hideEditor();

  m_lastClickedItem = item;
}


void K3bListView::editItem( K3bListViewItem* item, int col )
{
  if( item == 0 )
    hideEditor();
  else {
    showEditor( item, col );
  }
}


void K3bListView::hideEditor()
{
  m_lastClickedItem = 0;
  m_currentEditItem = 0;
  m_currentEditColumn = 0;

  if( m_editorSpinBox )
    m_editorSpinBox->hide();
  if( m_editorLineEdit )
    m_editorLineEdit->hide();
  if( m_editorComboBox )
    m_editorComboBox->hide();
  if( m_editorButton )
    m_editorButton->hide();
  if( m_editorMsfEdit )
    m_editorMsfEdit->hide();
}

void K3bListView::showEditor( K3bListViewItem* item, int col )
{
  if( item->needButton( col ) || item->editorType(col) != K3bListViewItem::NONE ) {
    m_currentEditColumn = col;
    m_currentEditItem = item;
  }

  placeEditor( item, col );
  if( item->needButton( col ) ) {
    m_editorButton->show();
  }
  switch( item->editorType(col) ) {
  case K3bListViewItem::COMBO:
    m_editorComboBox->show();
    m_editorComboBox->setFocus();
    break;
  case K3bListViewItem::LINE:
    m_editorLineEdit->show();
    m_editorLineEdit->setFocus();
    break;
  case K3bListViewItem::SPIN:
    m_editorSpinBox->show();
    m_editorSpinBox->setFocus();
    break;
  case K3bListViewItem::MSF:
    m_editorMsfEdit->show();
    m_editorMsfEdit->setFocus();
    break;
  default:
    break;
  }
}


void K3bListView::placeEditor( K3bListViewItem* item, int col )
{
  QRect r = itemRect( item );
  if ( !r.size().isValid() ) {
    ensureItemVisible( item );
    r = itemRect( item );
  }

  r.setX( header()->sectionPos( col ) );
  r.setWidth( header()->sectionSize( col ) - 1 );

  // check if the column is fully visible
  if( visibleWidth() < r.right() )
    r.setRight(visibleWidth());

  r = QRect( viewportToContents( r.topLeft() ), r.size() );

  if( item->pixmap( col ) ) {
    r.setX( r.x() + item->pixmap(col)->width() );
  }

  // the tree-stuff is painted in the first column
  if( col == 0 ) {
    r.setX( r.x() + item->depth() * treeStepSize() );
    if( rootIsDecorated() )
      r.setX( r.x() + treeStepSize() );
  }

  if( item->needButton(col) ) {
    prepareButton( item, col );
    m_editorButton->setFixedHeight( r.height() );
    // for now we make a square button
    m_editorButton->setFixedWidth( m_editorButton->height() );
    r.setWidth( r.width() - m_editorButton->width() );
    moveChild( m_editorButton, r.right(), r.y() );
  }

  if( QWidget* editor = prepareEditor( item, col ) ) {
    editor->resize( r.size() );
    //    editor->resize( QSize( r.width(), editor->minimumSizeHint().height() ) );
    moveChild( editor, r.x(), r.y() );
  }
}


void K3bListView::prepareButton( K3bListViewItem*, int )
{
  if( !m_editorButton ) {
    m_editorButton = new QPushButton( viewport() );
    connect( m_editorButton, SIGNAL(clicked()),
	     this, SLOT(slotEditorButtonClicked()) );
  }

  // TODO: do some useful things
  m_editorButton->setText( "..." );
}


QWidget* K3bListView::prepareEditor( K3bListViewItem* item, int col )
{
  switch( item->editorType(col) ) {
  case K3bListViewItem::COMBO:
    if( !m_editorComboBox ) {
      m_editorComboBox = new QComboBox( viewport() );
      connect( m_editorComboBox, SIGNAL(activated(const QString&)), 
	       this, SLOT(slotEditorComboBoxActivated(const QString&)) );
      if( m_validator )
	m_editorComboBox->setValidator( m_validator );
    }
    m_editorComboBox->clear();
    if( item->comboStrings( col ).isEmpty() ) {
      m_editorComboBox->insertItem( item->text( col ) );
    }
    else {
      m_editorComboBox->insertStringList( item->comboStrings(col) );
      int current = item->comboStrings(col).findIndex( item->text(col) );
      if( current != -1 )
	m_editorComboBox->setCurrentItem( current );
    }
    return m_editorComboBox;

  case K3bListViewItem::LINE:
    if( !m_editorLineEdit ) {
      m_editorLineEdit = new QLineEdit( viewport() );
      connect( m_editorLineEdit, SIGNAL(returnPressed()),
	       this, SLOT(slotEditorLineEditReturnPressed()) );
      m_editorLineEdit->setFrameStyle( QFrame::Box | QFrame::Plain );
      m_editorLineEdit->setLineWidth(1);
      if( m_validator )
	m_editorLineEdit->setValidator( m_validator );
    }

    m_editorLineEdit->setText( item->text( col ) );
    return m_editorLineEdit;

  case K3bListViewItem::SPIN:
    if( !m_editorSpinBox ) {
      m_editorSpinBox = new QSpinBox( viewport() );
      connect( m_editorSpinBox, SIGNAL(valueChanged(int)),
	       this, SLOT(slotEditorSpinBoxValueChanged(int)) );
    }
    // set the range
    m_editorSpinBox->setValue( item->text(col).toInt() );
    return m_editorSpinBox;

  case K3bListViewItem::MSF:
    if( !m_editorMsfEdit ) {
      m_editorMsfEdit = new K3bMsfEdit( viewport() );
//       m_editorMsfEdit->setFrameStyle( QFrame::Box | QFrame::Plain );
//       m_editorMsfEdit->setLineWidth(1);
      connect( m_editorMsfEdit, SIGNAL(valueChanged(int)),
	       this, SLOT(slotEditorMsfEditValueChanged(int)) );
    }
    m_editorMsfEdit->setText( item->text(col) );
    return m_editorMsfEdit;

  default:
    return 0;
  }
}

void K3bListView::setCurrentItem( QListViewItem* i )
{
  if( !i || i == currentItem() )
    return;

  if( m_currentEditItem )
    if( m_currentEditItem->editorType(m_currentEditColumn) == K3bListViewItem::LINE ) {
      if( m_editorLineEdit->validator() ) {
	QString str = m_editorLineEdit->text();
	int pos = 0;
	if( m_editorLineEdit->validator()->validate( str, pos ) == QValidator::Acceptable )
	  slotEditorLineEditReturnPressed();
      }
      else
	slotEditorLineEditReturnPressed();
    }

  hideEditor();
  m_currentEditItem = 0;
  KListView::setCurrentItem( i );
}


void K3bListView::setNoItemText( const QString& text )
{
  m_noItemText = text;
  triggerUpdate();
}


void K3bListView::drawContentsOffset( QPainter * p, int ox, int oy, int cx, int cy, int cw, int ch )
{
  KListView::drawContentsOffset( p, ox, oy, cx, cy, cw, ch );

  if( childCount() == 0 && !m_noItemText.isEmpty()) {

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

void K3bListView::paintEmptyArea( QPainter* p, const QRect& rect )
{
  KListView::paintEmptyArea( p, rect );

//   if( childCount() == 0 && !m_noItemText.isEmpty()) {

//     QPainter pp( viewport() );
//     pp.fillRect( viewport()->rect(), viewport()->paletteBackgroundColor() );
//     pp.end();

//     p->setPen( Qt::darkGray );

//     QStringList lines = QStringList::split( "\n", m_noItemText );
//     int xpos = m_noItemHMargin;
//     int ypos = m_noItemVMargin + p->fontMetrics().height();

//     for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
//       p->drawText( xpos, ypos, *str );
//       ypos += p->fontMetrics().lineSpacing();
//  }
//   }
}

void K3bListView::resizeEvent( QResizeEvent* e )
{
  KListView::resizeEvent( e );
  updateEditorSize();
}


void K3bListView::updateEditorSize()
{
  if( m_currentEditItem )
    placeEditor( m_currentEditItem, m_currentEditColumn );
}


void K3bListView::slotEditorLineEditReturnPressed()
{
  if( renameItem( m_currentEditItem, m_currentEditColumn, m_editorLineEdit->text() ) ) {
    m_currentEditItem->setText( m_currentEditColumn, m_editorLineEdit->text() );
    hideEditor();
    emit itemRenamed( m_currentEditItem, m_editorLineEdit->text(), m_currentEditColumn );
  }
  else
    m_editorLineEdit->setText( m_currentEditItem->text( m_currentEditColumn ) );
}


void K3bListView::slotEditorComboBoxActivated( const QString& str )
{
  if( renameItem( m_currentEditItem, m_currentEditColumn, str ) ) {
    m_currentEditItem->setText( m_currentEditColumn, str );
    emit itemRenamed( m_currentEditItem, str, m_currentEditColumn );
  }
  else {
    for( int i = 0; i < m_editorComboBox->count(); ++i ) {
      if( m_editorComboBox->text(i) == m_currentEditItem->text(m_currentEditColumn) ) {
	m_editorComboBox->setCurrentItem( i );
	break;
      }
    }
  }
}


void K3bListView::slotEditorSpinBoxValueChanged( int value )
{
  if( renameItem( m_currentEditItem, m_currentEditColumn, QString::number(value) ) ) {
    m_currentEditItem->setText( m_currentEditColumn, QString::number(value) );
    emit itemRenamed( m_currentEditItem, QString::number(value), m_currentEditColumn );
  }
  else
    m_editorSpinBox->setValue( m_currentEditItem->text( m_currentEditColumn ).toInt() );
}


void K3bListView::slotEditorMsfEditValueChanged( int value )
{
  if( renameItem( m_currentEditItem, m_currentEditColumn, QString::number(value) ) ) {
    m_currentEditItem->setText( m_currentEditColumn, QString::number(value) );
    emit itemRenamed( m_currentEditItem, QString::number(value), m_currentEditColumn );
  }
  else
    m_editorMsfEdit->setText( m_currentEditItem->text( m_currentEditColumn ) );
}


void K3bListView::slotEditorButtonClicked()
{
  slotEditorButtonClicked( m_currentEditItem, m_currentEditColumn );
}


bool K3bListView::renameItem( K3bListViewItem*, int, const QString& )
{
  return true;
}


void K3bListView::slotEditorButtonClicked( K3bListViewItem* item, int col )
{
  emit editorButtonClicked( item, col );
}


void K3bListView::focusOutEvent( QFocusEvent* e )
{
  // This should be enabled but due to the problem with the document view always loosing
  // the focus it's not.
  //  hideEditor();
  KListView::focusOutEvent( e );
}


void K3bListView::setValidator( QValidator* v )
{
  m_validator = v;
  if( m_editorLineEdit ) 
    m_editorLineEdit->setValidator( v );
  if( m_editorComboBox )
    m_editorComboBox->setValidator( v );
}


#include "k3blistview.moc"
