/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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
#include <q3header.h>
#include <qrect.h>
#include <qpushbutton.h>
#include <qicon.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <q3listbox.h>
#include <qevent.h>
#include <qvalidator.h>
#include <qfont.h>
#include <qpalette.h>
#include <qstyle.h>
#include <qapplication.h>
#include <q3progressbar.h>
#include <qimage.h>
//Added by qt3to4:
#include <QKeyEvent>
#include <QPixmap>
#include <QFrame>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QPaintEvent>


#include <limits.h>



// ///////////////////////////////////////////////
//
// K3BLISTVIEWITEM
//
// ///////////////////////////////////////////////


class K3b::ListViewItem::ColumnInfo
{
public:
    ColumnInfo()
        : showProgress(false),
          progressValue(0),
          totalProgressSteps(100),
          margin(0),
          validator(0) {
        editorType = NONE;
        button = false;
        comboEditable = false;
        next = 0;
        fontSet = false;
        backgroundColorSet = false;
        foregroundColorSet = false;
    }

    ~ColumnInfo() {
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
    int totalProgressSteps;
    int margin;

    QValidator* validator;
};



K3b::ListViewItem::ListViewItem(Q3ListView *parent)
    : K3ListViewItem( parent )
{
    init();
}

K3b::ListViewItem::ListViewItem(Q3ListViewItem *parent)
    : K3ListViewItem( parent )
{
    init();
}

K3b::ListViewItem::ListViewItem(Q3ListView *parent, Q3ListViewItem *after)
    : K3ListViewItem( parent, after )
{
    init();
}

K3b::ListViewItem::ListViewItem(Q3ListViewItem *parent, Q3ListViewItem *after)
    : K3ListViewItem( parent, after )
{
    init();
}


K3b::ListViewItem::ListViewItem(Q3ListView *parent,
                                const QString& s1, const QString& s2,
                                const QString& s3, const QString& s4,
                                const QString& s5, const QString& s6,
                                const QString& s7, const QString& s8)
    : K3ListViewItem( parent, s1, s2, s3, s4, s5, s6, s7, s8 )
{
    init();
}


K3b::ListViewItem::ListViewItem(Q3ListViewItem *parent,
                                const QString& s1, const QString& s2,
                                const QString& s3, const QString& s4,
                                const QString& s5, const QString& s6,
                                const QString& s7, const QString& s8)
    : K3ListViewItem( parent, s1, s2, s3, s4, s5, s6, s7, s8 )
{
    init();
}


K3b::ListViewItem::ListViewItem(Q3ListView *parent, Q3ListViewItem *after,
                                const QString& s1, const QString& s2,
                                const QString& s3, const QString& s4,
                                const QString& s5, const QString& s6,
                                const QString& s7, const QString& s8)
    : K3ListViewItem( parent, after, s1, s2, s3, s4, s5, s6, s7, s8 )
{
    init();
}


K3b::ListViewItem::ListViewItem(Q3ListViewItem *parent, Q3ListViewItem *after,
                                const QString& s1, const QString& s2,
                                const QString& s3, const QString& s4,
                                const QString& s5, const QString& s6,
                                const QString& s7, const QString& s8)
    : K3ListViewItem( parent, after, s1, s2, s3, s4, s5, s6, s7, s8 )
{
    init();
}


K3b::ListViewItem::~ListViewItem()
{
    if( K3b::ListView* lv = dynamic_cast<K3b::ListView*>(listView()) )
        if( lv->currentlyEditedItem() == this )
            lv->hideEditor();

    if( m_columns )
        delete m_columns;
}


void K3b::ListViewItem::init()
{
    m_columns = 0;
    m_vMargin = 0;
}


int K3b::ListViewItem::width( const QFontMetrics& fm, const Q3ListView* lv, int c ) const
{
    return K3ListViewItem::width( fm, lv, c ) + getColumnInfo(c)->margin*2;
}


void K3b::ListViewItem::setEditor( int column, int editor, const QStringList& cs )
{
    ColumnInfo* colInfo = getColumnInfo(column);

    colInfo->editorType = editor;
    if( !cs.isEmpty() )
        colInfo->comboItems = cs;
}


void K3b::ListViewItem::setValidator( int column, QValidator* v )
{
    getColumnInfo(column)->validator = v;
}


QValidator* K3b::ListViewItem::validator( int col ) const
{
    return getColumnInfo(col)->validator;
}


void K3b::ListViewItem::setButton( int column, bool on )
{
    ColumnInfo* colInfo = getColumnInfo(column);

    colInfo->button = on;
}


K3b::ListViewItem::ColumnInfo* K3b::ListViewItem::getColumnInfo( int col ) const
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


int K3b::ListViewItem::editorType( int col ) const
{
    ColumnInfo* info = getColumnInfo( col );
    return info->editorType;
}


bool K3b::ListViewItem::needButton( int col ) const
{
    ColumnInfo* info = getColumnInfo( col );
    return info->button;
}


const QStringList& K3b::ListViewItem::comboStrings( int col ) const
{
    ColumnInfo* info = getColumnInfo( col );
    return info->comboItems;
}


void K3b::ListViewItem::setFont( int col, const QFont& f )
{
    ColumnInfo* info = getColumnInfo( col );
    info->fontSet = true;
    info->font = f;
}


void K3b::ListViewItem::setBackgroundColor( int col, const QColor& c )
{
    ColumnInfo* info = getColumnInfo( col );
    info->backgroundColorSet = true;
    info->backgroundColor = c;
    repaint();
}


void K3b::ListViewItem::setForegroundColor( int col, const QColor& c )
{
    ColumnInfo* info = getColumnInfo( col );
    info->foregroundColorSet = true;
    info->foregroundColor = c;
    repaint();
}


void K3b::ListViewItem::setDisplayProgressBar( int col, bool displ )
{
    ColumnInfo* info = getColumnInfo( col );
    info->showProgress = displ;
}


void K3b::ListViewItem::setProgress( int col, int p )
{
    ColumnInfo* info = getColumnInfo( col );
    if( !info->showProgress )
        setDisplayProgressBar( col, true );
    if( info->progressValue != p ) {
        info->progressValue = p;
        repaint();
    }
}


void K3b::ListViewItem::setTotalSteps( int col, int steps )
{
    ColumnInfo* info = getColumnInfo( col );
    info->totalProgressSteps = steps;

    repaint();
}


void K3b::ListViewItem::setMarginHorizontal( int col, int margin )
{
    ColumnInfo* info = getColumnInfo( col );
    info->margin = margin;

    repaint();
}


void K3b::ListViewItem::setMarginVertical( int margin )
{
    m_vMargin = margin;
    repaint();
}


int K3b::ListViewItem::marginHorizontal( int col ) const
{
    return getColumnInfo( col )->margin;
}


int K3b::ListViewItem::marginVertical() const
{
    return m_vMargin;
}


void K3b::ListViewItem::setup()
{
    K3ListViewItem::setup();

    setHeight( height() + 2*m_vMargin );
}


void K3b::ListViewItem::paintCell( QPainter* p, const QColorGroup& cg, int col, int width, int align )
{
    ColumnInfo* info = getColumnInfo( col );

    p->save();

    QFont oldFont( p->font() );
    QFont newFont = info->fontSet ? info->font : oldFont;
    p->setFont( newFont );
    QColorGroup cgh(cg);
    if( info->foregroundColorSet )
        cgh.setColor( QColorGroup::Text, info->foregroundColor );
    if( info->backgroundColorSet )
        cgh.setColor( QColorGroup::Base, info->backgroundColor );

    // in case this is the selected row has a margin we need to repaint the selection bar
    if( isSelected() &&
        (col == 0 || listView()->allColumnsShowFocus()) &&
        info->margin > 0 ) {

        p->fillRect( 0, 0, info->margin, height(),
                     cgh.brush( QColorGroup::Highlight ) );
        p->fillRect( width-info->margin, 0, info->margin, height(),
                     cgh.brush( QColorGroup::Highlight ) );
    }
    else { // in case we use the K3ListView alternate color stuff
        p->fillRect( 0, 0, info->margin, height(),
                     cgh.brush( QColorGroup::Base ) );
        p->fillRect( width-info->margin, 0, info->margin, height(),
                     cgh.brush( QColorGroup::Base ) );
    }

    // FIXME: the margin (we can only translate horizontally since height() is used for painting)
    p->translate( info->margin, 0 );

    if( info->showProgress ) {
        paintProgressBar( p, cgh, col, width-2*info->margin );
    }
    else {
        paintK3bCell( p, cgh, col, width-2*info->margin, align );
    }

    p->restore();
}


void K3b::ListViewItem::paintK3bCell( QPainter* p, const QColorGroup& cg, int col, int width, int align )
{
    Q3ListViewItem::paintCell( p, cg, col, width, align );
}


void K3b::ListViewItem::paintProgressBar( QPainter* p, const QColorGroup& cgh, int col, int width )
{
//   ColumnInfo* info = getColumnInfo( col );

//   QStyle::SFlags flags = QStyle::Style_Default;
//   if( listView()->isEnabled() )
//     flags |= QStyle::Style_Enabled;
//   if( listView()->hasFocus() )
//     flags |= QStyle::Style_HasFocus;

//   // FIXME: the QPainter is translated so 0, m_vMargin is the upper left of our paint rect
//   QRect r( 0, m_vMargin, width, height()-2*m_vMargin );

//   // create the double buffer pixmap
//   static QPixmap *doubleBuffer = 0;
//   if( !doubleBuffer )
//     doubleBuffer = new QPixmap;
//   doubleBuffer->resize( width, height() );

//   QPainter dbPainter( doubleBuffer );

//   // clear the background (we cannot use paintEmptyArea since it's protected in QListView)
//   if( K3b::ListView* lv = dynamic_cast<K3b::ListView*>(listView()) )
//     lv->paintEmptyArea( &dbPainter, r );
//   else
//     dbPainter.fillRect( 0, 0, width, height(),
// 			cgh.brush( QPalette::backgroundRoleFromMode(listView()->viewport()->backgroundMode()) ) );

//   // we want a little additional margin
//   r.setLeft( r.left()+1 );
//   r.setWidth( r.width()-2 );
//   r.setTop( r.top()+1 );
//   r.setHeight( r.height()-2 );

//   // this might be a stupid hack but most styles do not reimplement drawPrimitive PE_ProgressBarChunk
//   // so this way the user is happy....
//   static Q3ProgressBar* s_dummyProgressBar = 0;
//   if( !s_dummyProgressBar ) {
//     s_dummyProgressBar = new Q3ProgressBar();
//   }

//   s_dummyProgressBar->setTotalSteps( info->totalProgressSteps );
//   s_dummyProgressBar->setProgress( info->progressValue );

//   // some styles use the widget's geometry
//   s_dummyProgressBar->setGeometry( r );

//   listView()->style().drawControl(QStyle::CE_ProgressBarContents, &dbPainter, s_dummyProgressBar, r, cgh, flags );
//   listView()->style().drawControl(QStyle::CE_ProgressBarLabel, &dbPainter, s_dummyProgressBar, r, cgh, flags );

//   // now we really paint the progress in the listview
//   p->drawPixmap( 0, 0, *doubleBuffer );
}







K3b::CheckListViewItem::CheckListViewItem(Q3ListView *parent)
    : K3b::ListViewItem( parent ),
      m_checked(false)
{
}


K3b::CheckListViewItem::CheckListViewItem(Q3ListViewItem *parent)
    : K3b::ListViewItem( parent ),
      m_checked(false)
{
}


K3b::CheckListViewItem::CheckListViewItem(Q3ListView *parent, Q3ListViewItem *after)
    : K3b::ListViewItem( parent, after ),
      m_checked(false)
{
}


K3b::CheckListViewItem::CheckListViewItem(Q3ListViewItem *parent, Q3ListViewItem *after)
    : K3b::ListViewItem( parent, after ),
      m_checked(false)
{
}


bool K3b::CheckListViewItem::isChecked() const
{
    return m_checked;
}


void K3b::CheckListViewItem::setChecked( bool checked )
{
    m_checked = checked;
    repaint();
}


void K3b::CheckListViewItem::paintK3bCell( QPainter* p, const QColorGroup& cg, int col, int width, int align )
{
    K3b::ListViewItem::paintK3bCell( p, cg, col, width, align );
#ifdef __GNUC__
#warning FIXME: draw check mark
#endif
//   if( col == 0 ) {
//     if( m_checked ) {
//       QRect r( 0, marginVertical(), width, /*listView()->style().pixelMetric( QStyle::PM_CheckListButtonSize )*/height()-2*marginVertical() );

//       QStyle::SFlags flags = QStyle::Style_Default;
//       if( listView()->isEnabled() )
// 	flags |= QStyle::Style_Enabled;
//       if( listView()->hasFocus() )
// 	flags |= QStyle::Style_HasFocus;
//       if( isChecked() )
// 	flags |= QStyle::Style_On;
//       else
// 	flags |= QStyle::Style_Off;

//       listView()->style().drawPrimitive( QStyle::PE_CheckMark, p, r, cg, flags );
//     }
//   }
}






// ///////////////////////////////////////////////
//
// K3BLISTVIEW
//
// ///////////////////////////////////////////////


class K3b::ListView::Private
{
public:
    QLineEdit* spinBoxLineEdit;
    QLineEdit* msfEditLineEdit;
};


K3b::ListView::ListView( QWidget* parent )
    : K3ListView( parent ),
      m_noItemVMargin( 20 ),
      m_noItemHMargin( 20 )
{
    d = new Private;

    connect( header(), SIGNAL( sizeChange( int, int, int ) ),
             this, SLOT( updateEditorSize() ) );

    m_editorButton = 0;
    m_editorComboBox = 0;
    m_editorSpinBox = 0;
    m_editorLineEdit = 0;
    m_editorMsfEdit = 0;
    m_currentEditItem = 0;
    m_currentEditColumn = 0;
    m_doubleClickForEdit = true;
    m_lastClickedItem = 0;
}

K3b::ListView::~ListView()
{
    delete d;
}


QWidget* K3b::ListView::editor( K3b::ListViewItem::EditorType t ) const
{
    switch( t ) {
    case K3b::ListViewItem::COMBO:
        return m_editorComboBox;
    case K3b::ListViewItem::LINE:
        return m_editorLineEdit;
    case K3b::ListViewItem::SPIN:
        return m_editorSpinBox;
    case K3b::ListViewItem::MSF:
        return m_editorMsfEdit;
    default:
        return 0;
    }
}


void K3b::ListView::clear()
{
    hideEditor();
    K3ListView::clear();
}


void K3b::ListView::editItem( K3b::ListViewItem* item, int col )
{
    if( item == 0 )
        hideEditor();
    else if( item->isEnabled() ) {
        showEditor( item, col );
    }
}


void K3b::ListView::hideEditor()
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

void K3b::ListView::showEditor( K3b::ListViewItem* item, int col )
{
    if( !item )
        return;

    if( item->needButton( col ) || item->editorType(col) != K3b::ListViewItem::NONE ) {
        m_currentEditColumn = col;
        m_currentEditItem = item;
    }

    placeEditor( item, col );
    if( item->needButton( col ) ) {
        m_editorButton->show();
    }
    switch( item->editorType(col) ) {
    case K3b::ListViewItem::COMBO:
        m_editorComboBox->show();
        m_editorComboBox->setFocus();
        m_editorComboBox->setValidator( item->validator(col) );
        break;
    case K3b::ListViewItem::LINE:
        m_editorLineEdit->show();
        m_editorLineEdit->setFocus();
        m_editorLineEdit->setValidator( item->validator(col) );
        break;
    case K3b::ListViewItem::SPIN:
        m_editorSpinBox->show();
        m_editorSpinBox->setFocus();
        break;
    case K3b::ListViewItem::MSF:
        m_editorMsfEdit->show();
        m_editorMsfEdit->setFocus();
        break;
    default:
        break;
    }
}


void K3b::ListView::placeEditor( K3b::ListViewItem* item, int col )
{
    ensureItemVisible( item );
    QRect r = itemRect( item );

    r.setX( contentsToViewport( QPoint(header()->sectionPos( col ), 0) ).x() );
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


void K3b::ListView::prepareButton( K3b::ListViewItem*, int )
{
    if( !m_editorButton ) {
        m_editorButton = new QPushButton( viewport() );
        connect( m_editorButton, SIGNAL(clicked()),
                 this, SLOT(slotEditorButtonClicked()) );
    }

    // TODO: do some useful things
    m_editorButton->setText( "..." );
}


QWidget* K3b::ListView::prepareEditor( K3b::ListViewItem* item, int col )
{
    switch( item->editorType(col) ) {
    case K3b::ListViewItem::COMBO:
        if( !m_editorComboBox ) {
            m_editorComboBox = new QComboBox( viewport() );
            connect( m_editorComboBox, SIGNAL(activated(const QString&)),
                     this, SLOT(slotEditorComboBoxActivated(const QString&)) );
            m_editorComboBox->installEventFilter( this );
        }
        m_editorComboBox->clear();
        if( item->comboStrings( col ).isEmpty() ) {
            m_editorComboBox->addItem( item->text( col ) );
        }
        else {
            m_editorComboBox->addItems( item->comboStrings(col) );
            int current = item->comboStrings(col).indexOf( item->text(col) );
            if( current != -1 )
                m_editorComboBox->setCurrentIndex( current );
        }
        return m_editorComboBox;

    case K3b::ListViewItem::LINE: {
        if( !m_editorLineEdit ) {
            m_editorLineEdit = new QLineEdit( viewport() );
            m_editorLineEdit->installEventFilter( this );
        }

        QString txt = item->text( col );
        m_editorLineEdit->setText( txt );

        // select the edit text (handle extensions while doing so)
        int pos = txt.lastIndexOf( '.' );
        if( pos > 0 )
            m_editorLineEdit->setSelection( 0, pos );
        else
            m_editorLineEdit->setSelection( 0, txt.length() );

        return m_editorLineEdit;
    }

        //
        // A QSpinBox (and thus also a K3b::MsfEdit) uses a QLineEdit), thus
        // we have to use this QLineEdit as the actual object to dead with
        //

    case K3b::ListViewItem::SPIN:
        if( !m_editorSpinBox ) {
            m_editorSpinBox = new QSpinBox( viewport() );
            d->spinBoxLineEdit = static_cast<QLineEdit*>( m_editorSpinBox->child( 0, "QLineEdit" ) );
            connect( m_editorSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(slotEditorSpinBoxValueChanged(int)) );
            //      m_editorSpinBox->installEventFilter( this );
            d->spinBoxLineEdit->installEventFilter( this );
        }
        // set the range
        m_editorSpinBox->setValue( item->text(col).toInt() );
        return m_editorSpinBox;

    case K3b::ListViewItem::MSF:
        if( !m_editorMsfEdit ) {
            m_editorMsfEdit = new K3b::MsfEdit( viewport() );
            d->msfEditLineEdit = static_cast<QLineEdit*>( m_editorMsfEdit->child( 0, "QLineEdit" ) );
            connect( m_editorMsfEdit, SIGNAL(valueChanged(int)),
                     this, SLOT(slotEditorMsfEditValueChanged(int)) );
            //      m_editorMsfEdit->installEventFilter( this );
            d->msfEditLineEdit->installEventFilter( this );
        }
        m_editorMsfEdit->setMsfValue( K3b::Msf::fromString( item->text(col) ) );
        return m_editorMsfEdit;

    default:
        return 0;
    }
}

void K3b::ListView::setCurrentItem( Q3ListViewItem* i )
{
    if( !i || i == currentItem() )
        return;

    // I cannot remember why I did this here exactly. However, it resets the
    // m_lastClickedItem and thus invalidates the editing.
//   doRename();
//   hideEditor();
//   m_currentEditItem = 0;
    K3ListView::setCurrentItem( i );
}


void K3b::ListView::setNoItemText( const QString& text )
{
    m_noItemText = text;
    triggerUpdate();
}


void K3b::ListView::viewportPaintEvent( QPaintEvent* e )
{
    K3ListView::viewportPaintEvent( e );
}


// FIXME: move this to viewportPaintEvent
void K3b::ListView::drawContentsOffset( QPainter * p, int ox, int oy, int cx, int cy, int cw, int ch )
{
    K3ListView::drawContentsOffset( p, ox, oy, cx, cy, cw, ch );

    if( childCount() == 0 && !m_noItemText.isEmpty()) {

        p->setPen( Qt::darkGray );

        QStringList lines = m_noItemText.split( "\n" );
        int xpos = m_noItemHMargin;
        int ypos = m_noItemVMargin + p->fontMetrics().height();

        QStringList::Iterator end ( lines.end() );
        for( QStringList::Iterator str = lines.begin(); str != end; ++str ) {
            p->drawText( xpos, ypos, *str );
            ypos += p->fontMetrics().lineSpacing();
        }
    }
}

void K3b::ListView::paintEmptyArea( QPainter* p, const QRect& rect )
{
    K3ListView::paintEmptyArea( p, rect );

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

void K3b::ListView::resizeEvent( QResizeEvent* e )
{
    K3ListView::resizeEvent( e );
    updateEditorSize();
}


void K3b::ListView::updateEditorSize()
{
    if( m_currentEditItem )
        placeEditor( m_currentEditItem, m_currentEditColumn );
}


void K3b::ListView::slotEditorLineEditReturnPressed()
{
    if( doRename() ) {
        // edit the next line
        // TODO: add config for this
        if( K3b::ListViewItem* nextItem = dynamic_cast<K3b::ListViewItem*>( m_currentEditItem->nextSibling() ) )
            editItem( nextItem, currentEditColumn() );
        else {
            hideEditor();

            // keep the focus here
            viewport()->setFocus();
        }
    }
}


void K3b::ListView::slotEditorComboBoxActivated( const QString& )
{
    doRename();
//   if( renameItem( m_currentEditItem, m_currentEditColumn, str ) ) {
//     m_currentEditItem->setText( m_currentEditColumn, str );
//     emit itemRenamed( m_currentEditItem, str, m_currentEditColumn );
//   }
//   else {
//     for( int i = 0; i < m_editorComboBox->count(); ++i ) {
//       if( m_editorComboBox->text(i) == m_currentEditItem->text(m_currentEditColumn) ) {
// 	m_editorComboBox->setCurrentItem( i );
// 	break;
//       }
//     }
//   }
}


void K3b::ListView::slotEditorSpinBoxValueChanged( int )
{
//   if( renameItem( m_currentEditItem, m_currentEditColumn, QString::number(value) ) ) {
//     m_currentEditItem->setText( m_currentEditColumn, QString::number(value) );
//     emit itemRenamed( m_currentEditItem, QString::number(value), m_currentEditColumn );
//   }
//   else
//     m_editorSpinBox->setValue( m_currentEditItem->text( m_currentEditColumn ).toInt() );
}


void K3b::ListView::slotEditorMsfEditValueChanged( int )
{
    // FIXME: do we always need to update the value. Isn't it enough to do it at the end?
//   if( renameItem( m_currentEditItem, m_currentEditColumn, QString::number(value) ) ) {
//     m_currentEditItem->setText( m_currentEditColumn, QString::number(value) );
//     emit itemRenamed( m_currentEditItem, QString::number(value), m_currentEditColumn );
//   }
//   else
//     m_editorMsfEdit->setText( m_currentEditItem->text( m_currentEditColumn ) );
}


bool K3b::ListView::doRename()
{
    if( m_currentEditItem ) {
        QString newValue;
        switch( m_currentEditItem->editorType( m_currentEditColumn ) ) {
        case K3b::ListViewItem::COMBO:
            newValue = m_editorComboBox->currentText();
            break;
        case K3b::ListViewItem::LINE:
            newValue = m_editorLineEdit->text();
            break;
        case K3b::ListViewItem::SPIN:
            newValue = QString::number(m_editorSpinBox->value());
            break;
        case K3b::ListViewItem::MSF:
            newValue = QString::number(m_editorMsfEdit->value());
            break;
        }

        if( renameItem( m_currentEditItem, m_currentEditColumn, newValue ) ) {
            m_currentEditItem->setText( m_currentEditColumn, newValue );
            emit itemRenamed( m_currentEditItem, newValue, m_currentEditColumn );
            return true;
        }
        else {
            switch( m_currentEditItem->editorType( m_currentEditColumn ) ) {
            case K3b::ListViewItem::COMBO:
                for( int i = 0; i < m_editorComboBox->count(); ++i ) {
                    if( m_editorComboBox->itemText(i) == m_currentEditItem->text(m_currentEditColumn) ) {
                        m_editorComboBox->setCurrentIndex( i );
                        break;
                    }
                }
                break;
            case K3b::ListViewItem::LINE:
                m_editorLineEdit->setText( m_currentEditItem->text( m_currentEditColumn ) );
                break;
            case K3b::ListViewItem::SPIN:
                m_editorSpinBox->setValue( m_currentEditItem->text( m_currentEditColumn ).toInt() );
                break;
            case K3b::ListViewItem::MSF:
                m_editorMsfEdit->setMsfValue( K3b::Msf::fromString( m_currentEditItem->text( m_currentEditColumn ) ) );
                break;
            }
        }
    }


    return false;
}


void K3b::ListView::slotEditorButtonClicked()
{
    slotEditorButtonClicked( m_currentEditItem, m_currentEditColumn );
}


bool K3b::ListView::renameItem( K3b::ListViewItem* item, int col, const QString& text )
{
    Q_UNUSED(item);
    Q_UNUSED(col);
    Q_UNUSED(text);
    return true;
}


void K3b::ListView::slotEditorButtonClicked( K3b::ListViewItem* item, int col )
{
    emit editorButtonClicked( item, col );
}


bool K3b::ListView::eventFilter( QObject* o, QEvent* e )
{
    if( e->type() == QEvent::KeyPress ) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);
        if( ke->key() == Qt::Key_Tab ) {
            if( o == m_editorLineEdit ||
                o == d->msfEditLineEdit ||
                o == d->spinBoxLineEdit ) {
                K3b::ListViewItem* lastEditItem = m_currentEditItem;

                doRename();

                if( lastEditItem ) {
                    // can we rename one of the other columns?
                    int col = currentEditColumn()+1;
                    while( col < columns() && lastEditItem->editorType( col ) == K3b::ListViewItem::NONE )
                        ++col;
                    if( col < columns() )
                        editItem( lastEditItem, col );
                    else {
                        hideEditor();

                        // keep the focus here
                        viewport()->setFocus();

                        // search for the next editable item
                        while( K3b::ListViewItem* nextItem =
                               dynamic_cast<K3b::ListViewItem*>( lastEditItem->nextSibling() ) ) {
                            // edit first column
                            col = 0;
                            while( col < columns() && nextItem->editorType( col ) == K3b::ListViewItem::NONE )
                                ++col;
                            if( col < columns() ) {
                                editItem( nextItem, col );
                                break;
                            }

                            lastEditItem = nextItem;
                        }
                    }
                }

                return true;
            }
        }
        if( ke->key() == Qt::Key_Return ) {
            if( o == m_editorLineEdit ||
                o == d->msfEditLineEdit ||
                o == d->spinBoxLineEdit ) {
                K3b::ListViewItem* lastEditItem = m_currentEditItem;
                doRename();

                if( K3b::ListViewItem* nextItem =
                    dynamic_cast<K3b::ListViewItem*>( lastEditItem->nextSibling() ) )
                    editItem( nextItem, currentEditColumn() );
                else {
                    hideEditor();

                    // keep the focus here
                    viewport()->setFocus();
                }

                return true;
            }
        }
        else if( ke->key() == Qt::Key_Escape ) {
            if( o == m_editorLineEdit ||
                o == d->msfEditLineEdit ||
                o == d->spinBoxLineEdit ) {
                hideEditor();

                // keep the focus here
                viewport()->setFocus();

                return true;
            }
        }
    }

    else if( e->type() == QEvent::MouseButtonPress && o == viewport() ) {

        // first let's grab the focus
        viewport()->setFocus();

        QMouseEvent* me = static_cast<QMouseEvent*>( e );
        Q3ListViewItem* item = itemAt( me->pos() );
        int col = header()->sectionAt( me->pos().x() );
        if( K3b::CheckListViewItem* ci = dynamic_cast<K3b::CheckListViewItem*>( item ) ) {
            if( col == 0 ) {
                // FIXME: improve this click area!
                ci->setChecked( !ci->isChecked() );
                return true;
            }
        }

        if( me->button() == Qt::LeftButton ) {
            if( item != m_currentEditItem || m_currentEditColumn != col ) {
                doRename();
                if( K3b::ListViewItem* k3bItem = dynamic_cast<K3b::ListViewItem*>(item) ) {
                    if( me->pos().x() > item->depth()*treeStepSize() &&
                        item->isEnabled() &&
                        (m_lastClickedItem == item || !m_doubleClickForEdit) )
                        showEditor( k3bItem, col );
                    else {
                        hideEditor();

                        // keep the focus here
                        viewport()->setFocus();
                    }
                }
                else {
                    hideEditor();

                    // keep the focus here
                    viewport()->setFocus();
                }

                // do not count clicks in the item tree for editing
                if( item && me->pos().x() > item->depth()*treeStepSize() )
                    m_lastClickedItem = item;
            }
        }
    }

    else if( e->type() == QEvent::FocusOut ) {
        if( o == m_editorLineEdit ||
            o == d->msfEditLineEdit ||
            o == d->spinBoxLineEdit ||
            o == m_editorComboBox ) {
            // make sure we did not lose the focus to one of the edit widgets' children
            if( !qApp->focusWidget() || qApp->focusWidget()->parentWidget() != o ) {
                doRename();
                hideEditor();
            }
        }
    }

    return K3ListView::eventFilter( o, e );
}


void K3b::ListView::setK3bBackgroundPixmap( const QPixmap& pix, int pos )
{
    m_backgroundPixmap = pix;
    m_backgroundPixmapPosition = pos;
}


void K3b::ListView::viewportResizeEvent( QResizeEvent* e )
{
    if( !m_backgroundPixmap.isNull() ) {

        QSize size = viewport()->size().expandedTo( QSize( contentsWidth(), contentsHeight() ) );

        QPixmap bgPix( size );

        // FIXME: let the user specify the color
        bgPix.fill( palette().base().color() );

        if( bgPix.width() < m_backgroundPixmap.width() ||
            bgPix.height() < m_backgroundPixmap.height() ) {
            QPixmap newBgPix = m_backgroundPixmap.scaled( bgPix.size() );
            if( m_backgroundPixmapPosition == TOP_LEFT )
                bitBlt( &bgPix, 0, 0,
                        &newBgPix, 0, 0,
                        newBgPix.width(), newBgPix.height() );
            else {
                int dx = bgPix.width() / 2 - m_backgroundPixmap.width() /2;
                int dy = bgPix.height() / 2 - m_backgroundPixmap.height() /2;
                bitBlt( &bgPix, dx, dy, &newBgPix, 0, 0,
                        newBgPix.width(), newBgPix.height() );
            }
        }
        else {
            if( m_backgroundPixmapPosition == TOP_LEFT )
                bitBlt( &bgPix, 0, 0,
                        &m_backgroundPixmap, 0, 0,
                        m_backgroundPixmap.width(), m_backgroundPixmap.height() );
            else {
                int dx = bgPix.width() / 2 - m_backgroundPixmap.width() /2;
                int dy = bgPix.height() / 2 - m_backgroundPixmap.height() /2;
                bitBlt( &bgPix, dx, dy, &m_backgroundPixmap, 0, 0,
                        m_backgroundPixmap.width(), m_backgroundPixmap.height() );
            }
        }

        QPalette bgPalette;
        bgPalette.setBrush( viewport()->backgroundRole(), QBrush(bgPix) );
        viewport()->setPalette( bgPalette );
    }

    K3ListView::viewportResizeEvent( e );
}


Q3ListViewItem* K3b::ListView::parentItem( Q3ListViewItem* item )
{
    if( !item )
        return 0;
    if( item->parent() )
        return item->parent();
    else
        return K3b::ListView::parentItem( item->itemAbove() );
}



#include "k3blistview.moc"
