/* This file is part of the KDE libraries
   Copyright (C) 2000 Reginald Stadlbauer <reggie@kde.org>
   Copyright (C) 2000 Charles Samuels <charles@kde.org>
   Copyright (C) 2000 Peter Putzer

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qdragobject.h>
#include <qtimer.h>
#include <qheader.h>
#include <qcursor.h>
#include <qstack.h>
#include <qtooltip.h>
#include <qpainter.h>
#include <qstyle.h>

#include <kglobalsettings.h>
#include <kcursor.h>
#include <kapp.h>
#include <kipc.h>
#include <kdebug.h>

#define private public
#include <qlistview.h>
#undef private

#include "klistview.h"
#include "klistviewlineedit.h"

#include <X11/Xlib.h>

class KListView::Tooltip : public QToolTip
{
public:
  Tooltip (KListView* parent, QToolTipGroup* group = 0L);
  virtual ~Tooltip () {}

protected:
  /**
   * Reimplemented from QToolTip for internal reasons.
   */
  virtual void maybeTip (const QPoint&);

private:
  KListView* mParent;
};

KListView::Tooltip::Tooltip (KListView* parent, QToolTipGroup* group)
  : QToolTip (parent, group),
        mParent (parent)
{
}

void KListView::Tooltip::maybeTip (const QPoint&)
{
  // FIXME
}

class KListView::KListViewPrivate
{
public:
  KListViewPrivate (KListView* listview)
    : pCurrentItem (0L),
      oldCursor (listview->viewport()->cursor()),
      dragDelay (KGlobalSettings::dndEventDelay()),
      editor (new KListViewLineEdit (listview)),
      itemsMovable (true),
      selectedBySimpleMove(false),
      itemsRenameable (false),
      validDrag (false),
      dragEnabled (false),
      autoOpen (true),
      dropVisualizer (true),
      dropHighlighter (false),
      createChildren (true),
      pressedOnSelected (false),
      wasShiftEvent (false),
      fullWidth (false),
      selectionDirection(0),
      tooltipColumn (0),
      selectionMode (Single),
      contextMenuKey (KGlobalSettings::contextMenuKey()),
      showContextMenusOnPress (KGlobalSettings::showContextMenusOnPress()),
      mDropVisualizerWidth (4)
  {
      renameable += 0;
      connect(editor, SIGNAL(done(QListViewItem*,int)), listview, SLOT(doneEditing(QListViewItem*,int)));
  }

  ~KListViewPrivate ()
  {
    delete editor;
  }

  QListViewItem* pCurrentItem;

  QTimer autoSelect;
  int autoSelectDelay;

  QCursor oldCursor;

  QPoint startDragPos;
  int dragDelay;

  KListViewLineEdit *editor;
  QValueList<int> renameable;

  bool cursorInExecuteArea:1;
  bool bUseSingle:1;
  bool bChangeCursorOverItem:1;
  bool itemsMovable:1;
  bool selectedBySimpleMove : 1;
  bool itemsRenameable:1;
  bool validDrag:1;
  bool dragEnabled:1;
  bool autoOpen:1;
  bool dropVisualizer:1;
  bool dropHighlighter:1;
  bool createChildren:1;
  bool pressedOnSelected:1;
  bool wasShiftEvent:1;
  bool fullWidth:1;

  //+1 means downwards (y increases, -1 means upwards, 0 means not selected), aleXXX
  int selectionDirection;
  int tooltipColumn;

  SelectionModeExt selectionMode;
  int contextMenuKey;
  bool showContextMenusOnPress;

  QRect mOldDropVisualizer;
  int mDropVisualizerWidth;
  QRect mOldDropHighlighter;
  QListViewItem *afterItemDrop;
  QListViewItem *parentItemDrop;

  QColor alternateBackground;
};


KListViewLineEdit::KListViewLineEdit(KListView *parent)
        : KLineEdit(parent->viewport()), item(0), col(0), p(parent)
{
        setFrame( false );
        hide();
        connect( parent, SIGNAL( selectionChanged() ), SLOT( slotSelectionChanged() ));
}

KListViewLineEdit::~KListViewLineEdit()
{
}

void KListViewLineEdit::load(QListViewItem *i, int c)
{
        item=i;
        col=c;

        QRect rect(p->itemRect(i));
        setText(item->text(c));

        int fieldX = rect.x() - 1;
        int fieldW = p->columnWidth(col) + 2;

        int pos = p->header()->mapToIndex(col);
        for ( int index = 0; index < pos; index++ )
            fieldX += p->columnWidth( p->header()->mapToSection( index ));

        if ( col == 0 ) {
            int d = i->depth() + (p->rootIsDecorated() ? 1 : 0);
            d *= p->treeStepSize();
            fieldX += d;
            fieldW -= d;
        }

        if ( i->pixmap( col ) ) {// add width of pixmap
            int d = i->pixmap( col )->width();
            fieldX += d;
            fieldW -= d;
        }

        setGeometry(fieldX, rect.y() - 1, fieldW, rect.height() + 2);
        show();
        setFocus();
}

void KListViewLineEdit::keyPressEvent(QKeyEvent *e)
{
        if(e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
            terminate(true);
        else if(e->key() == Qt::Key_Escape)
            terminate(false);
        else
            KLineEdit::keyPressEvent(e);
}

void KListViewLineEdit::terminate()
{
    terminate(true);
}

void KListViewLineEdit::terminate(bool commit)
{
    if ( item )
    {
        //kdDebug() << "KListViewLineEdit::terminate " << commit << endl;
        if (commit)
            item->setText(col, text());
        int c=col;
        QListViewItem *i=item;
        col=0;
        item=0;
        hide(); // will call focusOutEvent, that's why we set item=0 before
        emit done(i,c);
    }
}

void KListViewLineEdit::focusOutEvent(QFocusEvent *ev)
{
    QFocusEvent * focusEv = static_cast<QFocusEvent*>(ev);
    // Don't let a RMB close the editor
    if (focusEv->reason() != QFocusEvent::Popup && focusEv->reason() != QFocusEvent::ActiveWindow)
        terminate(true);
}

void KListViewLineEdit::paintEvent( QPaintEvent *e )
{
    KLineEdit::paintEvent( e );

    if ( !frame() ) {
        QPainter p( this );
        p.setClipRegion( e->region() );
        p.drawRect( rect() );
    }
}

// selection changed -> terminate. As our "item" can be already deleted,
// we can't call terminate(false), because that would emit done() with
// a dangling pointer to "item".
void KListViewLineEdit::slotSelectionChanged()
{
    item = 0;
    col = 0;
    hide();
}


KListView::KListView( QWidget *parent, const char *name )
  : QListView( parent, name ),
        d (new KListViewPrivate (this))
{
  setDragAutoScroll(true);

  connect( this, SIGNAL( onViewport() ),
                   this, SLOT( slotOnViewport() ) );
  connect( this, SIGNAL( onItem( QListViewItem * ) ),
                   this, SLOT( slotOnItem( QListViewItem * ) ) );

  connect (this, SIGNAL(contentsMoving(int,int)),
                   this, SLOT(cleanDropVisualizer()));
  connect (this, SIGNAL(contentsMoving(int,int)),
                   this, SLOT(cleanItemHighlighter()));

  slotSettingsChanged(KApplication::SETTINGS_MOUSE);
  connect( kapp, SIGNAL( settingsChanged(int) ), SLOT( slotSettingsChanged(int) ) );
  kapp->addKipcEventMask( KIPC::SettingsChanged );

  connect(&d->autoSelect, SIGNAL( timeout() ),
                  this, SLOT( slotAutoSelect() ) );

  // context menu handling
  if (d->showContextMenusOnPress)
        {
          connect (this, SIGNAL (rightButtonPressed (QListViewItem*, const QPoint&, int)),
                           this, SLOT (emitContextMenu (QListViewItem*, const QPoint&, int)));
        }
  else
        {
          connect (this, SIGNAL (rightButtonClicked (QListViewItem*, const QPoint&, int)),
                           this, SLOT (emitContextMenu (QListViewItem*, const QPoint&, int)));
        }

  connect (this, SIGNAL (menuShortCutPressed (KListView*, QListViewItem*)),
                   this, SLOT (emitContextMenu (KListView*, QListViewItem*)));
  d->alternateBackground = KGlobalSettings::alternateBackgroundColor();
}



KListView::~KListView()
{
  delete d;
}

bool KListView::isExecuteArea( const QPoint& point )
{
  if ( itemAt( point ) )
    return isExecuteArea( point.x() );

  return false;
}

bool KListView::isExecuteArea( int x )
{
  if( allColumnsShowFocus() )
    return true;
  else {
    int offset = 0;
    int width = columnWidth( 0 );
    int pos = header()->mapToIndex( 0 );

    for ( int index = 0; index < pos; index++ )
      offset += columnWidth( header()->mapToSection( index ) );

    return ( x > offset && x < ( offset + width ) );
  }
}

void KListView::slotOnItem( QListViewItem *item )
{
  if ( item && isExecuteArea( QCursor::pos().x() ) && (d->autoSelectDelay > -1) && d->bUseSingle ) {
    d->autoSelect.start( d->autoSelectDelay, true );
    d->pCurrentItem = item;
  }
}

void KListView::slotOnViewport()
{
  if ( d->bChangeCursorOverItem )
    viewport()->setCursor( d->oldCursor );

  d->autoSelect.stop();
  d->pCurrentItem = 0L;
}

void KListView::slotSettingsChanged(int category)
{
  switch (category)
  {
  case KApplication::SETTINGS_MOUSE:
    d->dragDelay =  KGlobalSettings::dndEventDelay();
    d->bUseSingle = KGlobalSettings::singleClick();

    disconnect(this, SIGNAL (mouseButtonClicked (int, QListViewItem*, const QPoint &, int)),
               this, SLOT (slotMouseButtonClicked (int, QListViewItem*, const QPoint &, int)));

    if( d->bUseSingle )
      connect (this, SIGNAL (mouseButtonClicked (int, QListViewItem*, const QPoint &, int)),
               this, SLOT (slotMouseButtonClicked( int, QListViewItem*, const QPoint &, int)));

    d->bChangeCursorOverItem = KGlobalSettings::changeCursorOverIcon();
    d->autoSelectDelay = KGlobalSettings::autoSelectDelay();

    if( !d->bUseSingle || !d->bChangeCursorOverItem )
       viewport()->setCursor( d->oldCursor );

    break;

  case KApplication::SETTINGS_POPUPMENU:
    d->contextMenuKey = KGlobalSettings::contextMenuKey ();
    d->showContextMenusOnPress = KGlobalSettings::showContextMenusOnPress ();

    if (d->showContextMenusOnPress)
    {
      disconnect (0L, 0L, this, SLOT (emitContextMenu (QListViewItem*, const QPoint&, int)));

      connect(this, SIGNAL (rightButtonPressed (QListViewItem*, const QPoint&, int)),
              this, SLOT (emitContextMenu (QListViewItem*, const QPoint&, int)));
    }
    else
    {
      disconnect (0L, 0L, this, SLOT (emitContextMenu (QListViewItem*, const QPoint&, int)));

      connect(this, SIGNAL (rightButtonClicked (QListViewItem*, const QPoint&, int)),
              this, SLOT (emitContextMenu (QListViewItem*, const QPoint&, int)));
    }
    break;

  default:
    break;
  }
}

void KListView::slotAutoSelect()
{
  // check that the item still exists
  if( itemIndex( d->pCurrentItem ) == -1 )
    return;

  if (!isActiveWindow())
        {
          d->autoSelect.stop();
          return;
        }

  //Give this widget the keyboard focus.
  if( !hasFocus() )
    setFocus();

  Window root;
  Window child;
  int root_x, root_y, win_x, win_y;
  uint keybstate;
  XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
                                 &root_x, &root_y, &win_x, &win_y, &keybstate );

  QListViewItem* previousItem = currentItem();
  setCurrentItem( d->pCurrentItem );

  if( d->pCurrentItem ) {
    //Shift pressed?
    if( (keybstate & ShiftMask) ) {
      bool block = signalsBlocked();
      blockSignals( true );

      //No Ctrl? Then clear before!
      if( !(keybstate & ControlMask) )
                clearSelection();

      bool select = !d->pCurrentItem->isSelected();
      bool update = viewport()->isUpdatesEnabled();
      viewport()->setUpdatesEnabled( false );

      bool down = previousItem->itemPos() < d->pCurrentItem->itemPos();
      QListViewItemIterator lit( down ? previousItem : d->pCurrentItem );
      for ( ; lit.current(); ++lit ) {
                if ( down && lit.current() == d->pCurrentItem ) {
                  d->pCurrentItem->setSelected( select );
                  break;
                }
                if ( !down && lit.current() == previousItem ) {
                  previousItem->setSelected( select );
                  break;
                }
                lit.current()->setSelected( select );
      }

      blockSignals( block );
      viewport()->setUpdatesEnabled( update );
      triggerUpdate();

      emit selectionChanged();

      if( selectionMode() == QListView::Single )
                emit selectionChanged( d->pCurrentItem );
    }
    else if( (keybstate & ControlMask) )
      setSelected( d->pCurrentItem, !d->pCurrentItem->isSelected() );
    else {
      bool block = signalsBlocked();
      blockSignals( true );

      if( !d->pCurrentItem->isSelected() )
                clearSelection();

      blockSignals( block );

      setSelected( d->pCurrentItem, true );
    }
  }
  else
    kdDebug() << "KListView::slotAutoSelect: That´s not supposed to happen!!!!" << endl;
}

void KListView::emitExecute( QListViewItem *item, const QPoint &pos, int c )
{
    if( isExecuteArea( viewport()->mapFromGlobal(pos) ) ) {

        // Double click mode ?
        if ( !d->bUseSingle )
        {
            emit executed( item );
            emit executed( item, pos, c );
        }
        else
        {
            Window root;
            Window child;
            int root_x, root_y, win_x, win_y;
            uint keybstate;
            XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
                           &root_x, &root_y, &win_x, &win_y, &keybstate );

            d->autoSelect.stop();

            //Don´t emit executed if in SC mode and Shift or Ctrl are pressed
            if( !( ((keybstate & ShiftMask) || (keybstate & ControlMask)) ) ) {
                emit executed( item );
                emit executed( item, pos, c );
            }
        }
    }
}

void KListView::focusInEvent( QFocusEvent *fe )
{
 //   kdDebug()<<"KListView::focusInEvent()"<<endl;
  QListView::focusInEvent( fe );
  if ((d->selectedBySimpleMove)
      && (d->selectionMode == Konqueror)
      && (fe->reason()!=QFocusEvent::Popup)
      && (fe->reason()!=QFocusEvent::ActiveWindow)
      && (currentItem()!=0))
  {
      currentItem()->setSelected(true);
      currentItem()->repaint();
      emit selectionChanged();
  };
}

void KListView::focusOutEvent( QFocusEvent *fe )
{
  cleanDropVisualizer();
  cleanItemHighlighter();

  d->autoSelect.stop();

  if ((d->selectedBySimpleMove)
      && (d->selectionMode == Konqueror)
      && (fe->reason()!=QFocusEvent::Popup)
      && (fe->reason()!=QFocusEvent::ActiveWindow)
      && (currentItem()!=0))
  {
      currentItem()->setSelected(false);
      currentItem()->repaint();
      emit selectionChanged();
  };

  QListView::focusOutEvent( fe );
}

void KListView::leaveEvent( QEvent *e )
{
  d->autoSelect.stop();

  QListView::leaveEvent( e );
}

bool KListView::event( QEvent *e )
{
  if (e->type() == QEvent::ApplicationPaletteChange)
    d->alternateBackground=KGlobalSettings::alternateBackgroundColor();
  
  return QListView::event(e);
}

void KListView::contentsMousePressEvent( QMouseEvent *e )
{
  if( (selectionModeExt() == Extended) && (e->state() & ShiftButton) && !(e->state() & ControlButton) )
  {
    bool block = signalsBlocked();
    blockSignals( true );

    clearSelection();

    blockSignals( block );
  }
  else if ((selectionModeExt()==Konqueror) && (d->selectedBySimpleMove))
  {
     d->selectedBySimpleMove=false;
     if (currentItem()!=0)
     {
        currentItem()->setSelected(false);
        currentItem()->repaint();
//        emit selectionChanged();
     };
  };

  QPoint p( contentsToViewport( e->pos() ) );
  QListViewItem *at = itemAt (p);

  // true if the root decoration of the item "at" was clicked (i.e. the +/- sign)
  bool rootDecoClicked = at
           && ( p.x() <= header()->cellPos( header()->mapToActual( 0 ) ) +
                treeStepSize() * ( at->depth() + ( rootIsDecorated() ? 1 : 0) ) + itemMargin() )
           && ( p.x() >= header()->cellPos( header()->mapToActual( 0 ) ) );

  if (e->button() == LeftButton && !rootDecoClicked)
  {
    //Start a drag
    d->startDragPos = e->pos();

    if (at)
    {
      d->validDrag = true;
      d->pressedOnSelected = at->isSelected();
    }
  }

  QListView::contentsMousePressEvent( e );
}

void KListView::contentsMouseMoveEvent( QMouseEvent *e )
{
  if (!dragEnabled() || d->startDragPos.isNull() || !d->validDrag)
      QListView::contentsMouseMoveEvent (e);

  QPoint vp = contentsToViewport(e->pos());
  QListViewItem *item = itemAt( vp );

  //do we process cursor changes at all?
  if ( item && d->bChangeCursorOverItem && d->bUseSingle )
    {
      //Cursor moved on a new item or in/out the execute area
      if( (item != d->pCurrentItem) ||
          (isExecuteArea(vp) != d->cursorInExecuteArea) )
        {
          d->cursorInExecuteArea = isExecuteArea(vp);

          if( d->cursorInExecuteArea ) //cursor moved in execute area
            viewport()->setCursor( KCursor::handCursor() );
          else //cursor moved out of execute area
            viewport()->setCursor( d->oldCursor );
        }
    }

  bool dragOn = dragEnabled();
  QPoint newPos = e->pos();
  if (dragOn && d->validDrag &&
      (newPos.x() > d->startDragPos.x()+d->dragDelay ||
       newPos.x() < d->startDragPos.x()-d->dragDelay ||
       newPos.y() > d->startDragPos.y()+d->dragDelay ||
       newPos.y() < d->startDragPos.y()-d->dragDelay))
    //(d->startDragPos - e->pos()).manhattanLength() > QApplication::startDragDistance())
    {
      QListView::contentsMouseReleaseEvent( 0 );
      startDrag();
      d->startDragPos = QPoint();
      d->validDrag = false;
    }
}

void KListView::contentsMouseReleaseEvent( QMouseEvent *e )
{
  if (e->button() == LeftButton)
  {
    // If the row was already selected, maybe we want to start an in-place editing
    if ( d->pressedOnSelected && itemsRenameable() )
    {
      QPoint p( contentsToViewport( e->pos() ) );
      QListViewItem *at = itemAt (p);
      if ( at )
      {
        // true if the root decoration of the item "at" was clicked (i.e. the +/- sign)
        bool rootDecoClicked =
                  ( p.x() <= header()->cellPos( header()->mapToActual( 0 ) ) +
                    treeStepSize() * ( at->depth() + ( rootIsDecorated() ? 1 : 0) ) + itemMargin() )
               && ( p.x() >= header()->cellPos( header()->mapToActual( 0 ) ) );

        if (!rootDecoClicked)
        {
          int col = header()->mapToLogical( header()->cellAt( p.x() ) );
          if ( d->renameable.contains(col) )
            rename(at, col);
        }
      }
    }

    d->pressedOnSelected = false;
    d->validDrag = false;
    d->startDragPos = QPoint();
  }
  QListView::contentsMouseReleaseEvent( e );
}

void KListView::contentsMouseDoubleClickEvent ( QMouseEvent *e )
{
  // We don't want to call the parent method because it does setOpen,
  // whereas we don't do it in single click mode... (David)
  //QListView::contentsMouseDoubleClickEvent( e );

  QPoint vp = contentsToViewport(e->pos());
  QListViewItem *item = itemAt( vp );
  emit QListView::doubleClicked( item ); // we do it now

  int col = item ? header()->mapToLogical( header()->cellAt( vp.x() ) ) : -1;

  if( item ) {
    emit doubleClicked( item, e->globalPos(), col );

    if( (e->button() == LeftButton) && !d->bUseSingle )
      emitExecute( item, e->globalPos(), col );
  }
}

void KListView::slotMouseButtonClicked( int btn, QListViewItem *item, const QPoint &pos, int c )
{
  if( (btn == LeftButton) && item )
    emitExecute(item, pos, c);
}

void KListView::contentsDropEvent(QDropEvent* e)
{
  cleanDropVisualizer();
  cleanItemHighlighter();

  if (acceptDrag (e))
  {
    e->acceptAction();
    QListViewItem *afterme;
    QListViewItem *parent;
    findDrop(e->pos(), parent, afterme);

    if (e->source() == viewport() && itemsMovable())
        movableDropEvent(parent, afterme);
    else
    {
        emit dropped(e, afterme);
        emit dropped(this, e, afterme);
        emit dropped(e, parent, afterme);
        emit dropped(this, e, parent, afterme);
    }
  }
}

void KListView::movableDropEvent (QListViewItem* parent, QListViewItem* afterme)
{
  QList<QListViewItem> items, afterFirsts, afterNows;
  QListViewItem *current=currentItem();
  bool hasMoved=false;
  for (QListViewItem *i = firstChild(), *iNext=0; i != 0; i = iNext)
  {
    iNext=i->itemBelow();
    if (!i->isSelected())
      continue;

    // don't drop an item after itself, or else
    // it moves to the top of the list
    if (i==afterme)
      continue;

    i->setSelected(false);

    QListViewItem *afterFirst = i->itemAbove();

        if (!hasMoved)
        {
                emit aboutToMove();
                hasMoved=true;
        }

    moveItem(i, parent, afterme);

    // ###### This should include the new parent !!! -> KDE 3.0
    // If you need this right now, have a look at keditbookmarks.
    emit moved(i, afterFirst, afterme);

    items.append (i);
    afterFirsts.append (afterFirst);
    afterNows.append (afterme);

    afterme = i;
  }
  clearSelection();
  for (QListViewItem *i=items.first(); i != 0; i=items.next() )
    i->setSelected(true);
  if (current)
    setCurrentItem(current);

  emit moved(items,afterFirsts,afterNows);

  if (firstChild())
    emit moved();
}

void KListView::contentsDragMoveEvent(QDragMoveEvent *event)
{
  if (acceptDrag(event))
  {
    event->acceptAction();
    //Clean up the view

    findDrop(event->pos(), d->parentItemDrop, d->afterItemDrop);
    if (dropVisualizer())
    {
      QRect tmpRect = drawDropVisualizer(0, d->parentItemDrop, d->afterItemDrop);
      if (tmpRect != d->mOldDropVisualizer)
      {
        cleanDropVisualizer();
        d->mOldDropVisualizer=tmpRect;
        viewport()->repaint(tmpRect);
      }
    }
    if (dropHighlighter())
    {
      QRect tmpRect = drawItemHighlighter(0, d->afterItemDrop);
      if (tmpRect != d->mOldDropHighlighter)
      {
        cleanItemHighlighter();
        d->mOldDropHighlighter=tmpRect;
        viewport()->repaint(tmpRect);
      }
    }
  }
  else
      event->ignore();
}

void KListView::contentsDragLeaveEvent (QDragLeaveEvent*)
{
  cleanDropVisualizer();
  cleanItemHighlighter();
}

void KListView::cleanDropVisualizer()
{
  if (d->mOldDropVisualizer.isValid())
  {
    QRect rect=d->mOldDropVisualizer;
    d->mOldDropVisualizer = QRect();
    viewport()->repaint(rect, true);
  }
}

int KListView::depthToPixels( int depth )
{
    return treeStepSize() * ( depth + (rootIsDecorated() ? 1 : 0) ) + itemMargin();
}

void KListView::findDrop(const QPoint &pos, QListViewItem *&parent, QListViewItem *&after)
{
  QPoint p (contentsToViewport(pos));

  // Get the position to put it in
  QListViewItem *atpos = itemAt(p);

  QListViewItem *above;
  if (!atpos) // put it at the end
    above = lastItem();
  else
  {
    // Get the closest item before us ('atpos' or the one above, if any)
      if (p.y() - itemRect(atpos).topLeft().y() < (atpos->height()/2))
          above = atpos->itemAbove();
      else
          above = atpos;
  }

  if (above)
  {
      // Now, we know we want to go after "above". But as a child or as a sibling ?
      // We have to ask the "above" item if it accepts children.
      if (above->isExpandable())
      {
          // The mouse is sufficiently on the right ? - doesn't matter if 'above' has visible children
          if (p.x() >= depthToPixels( above->depth() + 1 ) ||
              (above->isOpen() && above->childCount() > 0) )
          {
              parent = above;
              after = 0L;
              return;
          }
      }

      // Ok, there's one more level of complexity. We may want to become a new
      // sibling, but of an upper-level group, rather than the "above" item
      QListViewItem * betterAbove = above->parent();
      QListViewItem * last = above;
      while ( betterAbove )
      {
          // We are allowed to become a sibling of "betterAbove" only if we are
          // after its last child
          if ( last->nextSibling() == 0 )
          {
              if (p.x() < depthToPixels ( betterAbove->depth() + 1 ))
                  above = betterAbove; // store this one, but don't stop yet, there may be a better one
              else
                  break; // not enough on the left, so stop
              last = betterAbove;
              betterAbove = betterAbove->parent(); // up one level
          } else
              break; // we're among the child of betterAbove, not after the last one
      }
  }
  // set as sibling
  after = above;
  parent = after ? after->parent() : 0L ;
}

QListViewItem* KListView::lastChild () const
{
  QListViewItem* lastchild = firstChild();

  if (lastchild)
        for (; lastchild->nextSibling(); lastchild = lastchild->nextSibling());

  return lastchild;
}

QListViewItem *KListView::lastItem() const
{
  QListViewItem* last = lastChild();

  for (QListViewItemIterator it (last); it.current(); ++it)
    last = it.current();

  return last;
}

void KListView::startDrag()
{
  QDragObject *drag = dragObject();

  if (!drag)
        return;

  if (drag->drag() && drag->target() != viewport())
    emit moved();
}

QDragObject *KListView::dragObject() const
{
  if (!currentItem())
        return 0;

  return new QStoredDrag("application/x-qlistviewitem", viewport());
}

void KListView::setItemsMovable(bool b)
{
  d->itemsMovable=b;
}

bool KListView::itemsMovable() const
{
  return d->itemsMovable;
}

void KListView::setItemsRenameable(bool b)
{
  d->itemsRenameable=b;
}

bool KListView::itemsRenameable() const
{
  return d->itemsRenameable;
}


void KListView::setDragEnabled(bool b)
{
  d->dragEnabled=b;
}

bool KListView::dragEnabled() const
{
  return d->dragEnabled;
}

void KListView::setAutoOpen(bool b)
{
  d->autoOpen=b;
}

bool KListView::autoOpen() const
{
  return d->autoOpen;
}

bool KListView::dropVisualizer() const
{
  return d->dropVisualizer;
}

void KListView::setDropVisualizer(bool b)
{
  d->dropVisualizer=b;
}

QList<QListViewItem> KListView::selectedItems() const
{
  QList<QListViewItem> list;
  for (QListViewItem *i=firstChild(); i!=0; i=i->itemBelow())
        if (i->isSelected()) list.append(i);
  return list;
}


void KListView::moveItem(QListViewItem *item, QListViewItem *parent, QListViewItem *after)
{
  // sanity check - don't move a item into it's own child structure
  QListViewItem *i = parent;
  while(i)
    {
      if(i == item)
        return;
      i = i->parent();
    }

  // Basically reimplementing the QListViewItem(QListViewItem*, QListViewItem*) constructor
  // in here, without ever deleting the item.
  if (item->parent())
        item->parent()->takeItem(item);
  else
        takeItem(item);

  if (parent)
        parent->insertItem(item);
  else
        insertItem(item);

  if (after)
        item->moveToJustAfter(after);
}

void KListView::contentsDragEnterEvent(QDragEnterEvent *event)
{
  if (acceptDrag (event))
    event->accept();
}

void KListView::setDropVisualizerWidth (int w)
{
  d->mDropVisualizerWidth = w > 0 ? w : 1;
}

QRect KListView::drawDropVisualizer(QPainter *p, QListViewItem *parent,
                                    QListViewItem *after)
{
    QRect insertmarker;

    if (!after && !parent)
        insertmarker = QRect (0, 0, viewport()->width(), d->mDropVisualizerWidth/2);
    else
    {
        int level;
        if (after)
        {
            QListViewItem* it = 0L;
            if (after->isOpen())
            {
                // Look for the last child (recursively)
                it = after->firstChild();
                if (it)
                    while (it->nextSibling() || it->firstChild())
                        if ( it->nextSibling() )
                            it = it->nextSibling();
                        else
                            it = it->firstChild();
            }

            insertmarker = itemRect (it ? it : after);
            level = after->depth();
        }
        else if (parent)
        {
            insertmarker = itemRect (parent);
            level = parent->depth() + 1;
        }
        insertmarker.setLeft( treeStepSize() * ( level + (rootIsDecorated() ? 1 : 0) ) + itemMargin() );
        insertmarker.setRight (viewport()->width());
        insertmarker.setTop (insertmarker.bottom() - d->mDropVisualizerWidth/2 + 1);
        insertmarker.setBottom (insertmarker.bottom() + d->mDropVisualizerWidth/2);
    }

    // This is not used anymore, at least by KListView itself (see viewportPaintEvent)
    // Remove for KDE 3.0.
    if (p)
        p->fillRect(insertmarker, Dense4Pattern);

    return insertmarker;
}

QRect KListView::drawItemHighlighter(QPainter *painter, QListViewItem *item)
{
  QRect r;

  if (item)
  {
    r = itemRect(item);
    r.setLeft(r.left()+(item->depth()+1)*treeStepSize());
    if (painter)
      style().drawFocusRect(painter, r, colorGroup(), &colorGroup().highlight(), true);
  }

  return r;
}

void KListView::cleanItemHighlighter ()
{
  if (d->mOldDropHighlighter.isValid())
  {
    QRect rect=d->mOldDropHighlighter;
    d->mOldDropHighlighter = QRect();
    viewport()->repaint(rect, true);
  }
}

void KListView::rename(QListViewItem *item, int c)
{
  KListViewItem *ki = dynamic_cast<KListViewItem*>(item);
  if( !ki || ki->isRenamable() )   // treat QListViewItems as always renamable
    d->editor->load(item,c);
}

bool KListView::isRenameable (int col) const
{
  return d->renameable.contains(col);
}

void KListView::setRenameable (int col, bool yesno)
{
  if (col>=header()->count()) return;

  d->renameable.remove(col);
  if (yesno && d->renameable.find(col)==d->renameable.end())
    d->renameable+=col;
  else if (!yesno && d->renameable.find(col)!=d->renameable.end())
    d->renameable.remove(col);
}

void KListView::doneEditing(QListViewItem *item, int row)
{
  emit itemRenamed(item, item->text(row), row);
  emit itemRenamed(item);
}

bool KListView::acceptDrag(QDropEvent* e) const
{
  return acceptDrops() && itemsMovable() && (e->source()==viewport());
}

void KListView::setCreateChildren(bool b)
{
        d->createChildren=b;
}

bool KListView::createChildren() const
{
        return d->createChildren;
}


int KListView::tooltipColumn() const
{
        return d->tooltipColumn;
}

void KListView::setTooltipColumn(int column)
{
        d->tooltipColumn=column;
}

void KListView::setDropHighlighter(bool b)
{
        d->dropHighlighter=b;
}

bool KListView::dropHighlighter() const
{
        return d->dropHighlighter;
}

bool KListView::showTooltip(QListViewItem *item, const QPoint &, int column) const
{
        return ((tooltip(item, column).length()>0) && (column==tooltipColumn()));
}

QString KListView::tooltip(QListViewItem *item, int column) const
{
        return item->text(column);
}

void KListView::keyPressEvent (QKeyEvent* e)
{
  //don't we need a contextMenuModifier too ? (aleXXX)
  if (e->key() == d->contextMenuKey)
        {
          emit menuShortCutPressed (this, currentItem());
          return;
        }

  if (d->selectionMode != Konqueror)
        QListView::keyPressEvent (e);
  else
        konquerorKeyPressEvent (e);
}

//this one is only called in konq_listviewwidget, aleXXX
//### KDE 3: do something about it, aleXXX
void KListView::selectCurrentItemAndEnableSelectedBySimpleMoveMode()
{
   d->selectedBySimpleMove=true;
   if (currentItem()!=0)
   {
      currentItem()->setSelected(true);
      currentItem()->repaint();
      emit selectionChanged();
   };
}

void KListView::konquerorKeyPressEvent (QKeyEvent* e)
{
   //don't care whether it's on the keypad or not
    int e_state=(e->state() & ~Keypad);

    int oldSelectionDirection(d->selectionDirection);

    if ((e->key()!=Key_Shift) && (e->key()!=Key_Control)
        && (e->key()!=Key_Meta) && (e->key()!=Key_Alt))
    {
       if ((e_state==ShiftButton) && (!d->wasShiftEvent) && (!d->selectedBySimpleMove))
          selectAll(FALSE);
       d->selectionDirection=0;
       d->wasShiftEvent = (e_state == ShiftButton);
    };

    //d->wasShiftEvent = (e_state == ShiftButton);


    QListViewItem* item = currentItem();
    if (item==0) return;

    setUpdatesEnabled(false);

    QListViewItem* repaintItem1 = item;
    QListViewItem* repaintItem2 = 0L;
    QListViewItem* visItem = 0L;

    QListViewItem* nextItem = 0L;
    int items = 0;

    bool shiftOrCtrl((e_state==ControlButton) || (e_state==ShiftButton));
    int selectedItems(0);
    for (QListViewItem *tmpItem=firstChild(); tmpItem!=0; tmpItem=tmpItem->nextSibling())
       if (tmpItem->isSelected()) selectedItems++;

    if ((selectedItems==0)
        && (e_state==NoButton)
        && ((e->key()==Key_Down)
        || (e->key()==Key_Up)
        || (e->key()==Key_Next)
        || (e->key()==Key_Prior)
        || (e->key()==Key_Home)
        || (e->key()==Key_End)))
       d->selectedBySimpleMove=true;
    else if (selectedItems>1)
       d->selectedBySimpleMove=false;

    bool emitSelectionChanged(false);

    switch (e->key())
    {
    case Key_Escape:
       selectAll(FALSE);
       emitSelectionChanged=TRUE;
       break;

    case Key_Space:
       //toggle selection of current item
       if (d->selectedBySimpleMove)
          d->selectedBySimpleMove=false;
//       else
//       {
       item->setSelected(!item->isSelected());
       emitSelectionChanged=TRUE;
//       };
       break;

    case Key_Insert:
       //toggle selection of current item and move to the next item
       if (d->selectedBySimpleMove)
       {
          d->selectedBySimpleMove=false;
          if (!item->isSelected()) item->setSelected(TRUE);
       }
       else
       {
          item->setSelected(!item->isSelected());
       };

       nextItem=item->itemBelow();

       if (nextItem!=0)
       {
          repaintItem2=nextItem;
          visItem=nextItem;
          setCurrentItem(nextItem);
       };
       d->selectionDirection=1;
       emitSelectionChanged=TRUE;
       break;

    case Key_Down:
       nextItem=item->itemBelow();
       //toggle selection of current item and move to the next item
       if (shiftOrCtrl)
       {
          d->selectionDirection=1;
          if (d->selectedBySimpleMove)
             d->selectedBySimpleMove=false;
          else
          {
             if (oldSelectionDirection!=-1)
             {
                item->setSelected(!item->isSelected());
                emitSelectionChanged=TRUE;
             };
          };
       }
       else if ((d->selectedBySimpleMove) && (nextItem!=0))
       {
          item->setSelected(false);
          emitSelectionChanged=TRUE;
       };

       if (nextItem!=0)
       {
          if (d->selectedBySimpleMove)
             nextItem->setSelected(true);
          repaintItem2=nextItem;
          visItem=nextItem;
          setCurrentItem(nextItem);
       };
       break;

    case Key_Up:
       nextItem=item->itemAbove();
       d->selectionDirection=-1;
       //move to the prev. item and toggle selection of this one
       // => No, can't select the last item, with this. For symmetry, let's
       // toggle selection and THEN move up, just like we do in down (David)
       if (shiftOrCtrl)
       {
          if (d->selectedBySimpleMove)
             d->selectedBySimpleMove=false;
          else
          {
             if (oldSelectionDirection!=1)
             {
                item->setSelected(!item->isSelected());
                emitSelectionChanged=TRUE;
             };
          }
       }
       else if ((d->selectedBySimpleMove) && (nextItem!=0))
       {
          item->setSelected(false);
          emitSelectionChanged=TRUE;
       };

       if (nextItem!=0)
       {
          if (d->selectedBySimpleMove)
             nextItem->setSelected(true);
          repaintItem2=nextItem;
          visItem=nextItem;
          setCurrentItem(nextItem);
       };
       break;

    case Key_End:
       //move to the last item and toggle selection of all items inbetween
       nextItem=item;
       if (d->selectedBySimpleMove)
          item->setSelected(false);
       if (shiftOrCtrl)
          d->selectedBySimpleMove=false;

       while(nextItem!=0)
       {
          if (shiftOrCtrl)
             nextItem->setSelected(!nextItem->isSelected());
          if (nextItem->itemBelow()==0)
          {
             if (d->selectedBySimpleMove)
                nextItem->setSelected(true);
             repaintItem2=nextItem;
             visItem=nextItem;
             setCurrentItem(nextItem);
          }
          nextItem=nextItem->itemBelow();
       }
       emitSelectionChanged=TRUE;
       break;

    case Key_Home:
       //move to the last item and toggle selection of all items inbetween
       nextItem=item;
       if (d->selectedBySimpleMove)
          item->setSelected(false);
       if (shiftOrCtrl)
          d->selectedBySimpleMove=false;

       while(nextItem!=0)
       {
          if (shiftOrCtrl)
             nextItem->setSelected(!nextItem->isSelected());
          if (nextItem->itemAbove()==0)
          {
             if (d->selectedBySimpleMove)
                nextItem->setSelected(true);
             repaintItem2=nextItem;
             visItem=nextItem;
             setCurrentItem(nextItem);
          }
          nextItem=nextItem->itemAbove();
       }
       emitSelectionChanged=TRUE;
       break;

    case Key_Next:
       items=visibleHeight()/item->height();
       nextItem=item;
       if (d->selectedBySimpleMove)
          item->setSelected(false);
       if (shiftOrCtrl)
       {
          d->selectedBySimpleMove=false;
          d->selectionDirection=1;
       };

       for (int i=0; i<items; i++)
       {
          if (shiftOrCtrl)
             nextItem->setSelected(!nextItem->isSelected());
          //the end
          if ((i==items-1) || (nextItem->itemBelow()==0))

          {
             if (shiftOrCtrl)
                nextItem->setSelected(!nextItem->isSelected());
             if (d->selectedBySimpleMove)
                nextItem->setSelected(true);
             setUpdatesEnabled(true);
             ensureItemVisible(nextItem);
             setCurrentItem(nextItem);
             update();
             if ((shiftOrCtrl) || (d->selectedBySimpleMove))
             {
                emit selectionChanged();
             }
             return;
          }
          nextItem=nextItem->itemBelow();
       }
       break;

    case Key_Prior:
       items=visibleHeight()/item->height();
       nextItem=item;
       if (d->selectedBySimpleMove)
          item->setSelected(false);
       if (shiftOrCtrl)
       {
          d->selectionDirection=-1;
          d->selectedBySimpleMove=false;
       };

       for (int i=0; i<items; i++)
       {
          if ((nextItem!=item) &&(shiftOrCtrl))
             nextItem->setSelected(!nextItem->isSelected());
          //the end
          if ((i==items-1) || (nextItem->itemAbove()==0))

          {
             if (d->selectedBySimpleMove)
                nextItem->setSelected(true);
             setUpdatesEnabled(true);
             ensureItemVisible(nextItem);
             setCurrentItem(nextItem);
             update();
             if ((shiftOrCtrl) || (d->selectedBySimpleMove))
             {
                emit selectionChanged();
             }
             return;
          }
          nextItem=nextItem->itemAbove();
       }
       break;

    case Key_Minus:
       if ( item->isOpen() )
          setOpen( item, FALSE );
       break;
    case Key_Plus:
       if (  !item->isOpen() && (item->isExpandable() || item->childCount()) )
          setOpen( item, TRUE );
       break;
    default:
       bool realKey = ((e->key()!=Key_Shift) && (e->key()!=Key_Control)
                        && (e->key()!=Key_Meta) && (e->key()!=Key_Alt));

       bool selectCurrentItem = (d->selectedBySimpleMove) && (item->isSelected());
       if (realKey && selectCurrentItem)
          item->setSelected(false);
       //this is mainly for the "goto filename beginning with pressed char" feature (aleXXX)
       setSelectionMode (QListView::Multi);
       QListView::keyPressEvent (e);
       setSelectionMode (QListView::Extended);
       if (realKey && selectCurrentItem)
       {
          currentItem()->setSelected(true);
          emitSelectionChanged=TRUE;
       }
       repaintItem2=currentItem();
       if (realKey)
          visItem=currentItem();
       break;
    }

    setUpdatesEnabled(true);
    if (visItem)
       ensureItemVisible(visItem);

    QRect ir;
    if (repaintItem1)
       ir = ir.unite( itemRect(repaintItem1) );
    if (repaintItem2)
       ir = ir.unite( itemRect(repaintItem2) );

    if ( !ir.isEmpty() )
    {                 // rectangle to be repainted
       if ( ir.x() < 0 )
          ir.moveBy( -ir.x(), 0 );
       viewport()->repaint( ir, FALSE );
    }
    /*if (repaintItem1)
       repaintItem1->repaint();
    if (repaintItem2)
       repaintItem2->repaint();*/
    update();
    if (emitSelectionChanged)
       emit selectionChanged();
}

void KListView::setSelectionModeExt (SelectionModeExt mode)
{
    d->selectionMode = mode;

    switch (mode)
    {
    case Single:
    case Multi:
    case Extended:
    case NoSelection:
        setSelectionMode (static_cast<QListView::SelectionMode>(static_cast<int>(mode)));
        break;

    case Konqueror:
        setSelectionMode (QListView::Extended);
        break;

    default:
        kdWarning () << "Warning: illegal selection mode " << int(mode) << " set!" << endl;
        break;
    }
}

KListView::SelectionModeExt KListView::selectionModeExt () const
{
  return d->selectionMode;
}

int KListView::itemIndex( const QListViewItem *item ) const
{
    if ( !item )
        return -1;

    if ( item == firstChild() )
        return 0;
    else {
        QListViewItemIterator it(firstChild());
        uint j = 0;
        for (; it.current() && it.current() != item; ++it, ++j );

        if( !it.current() )
          return -1;

        return j;
    }
}

QListViewItem* KListView::itemAtIndex(int index)
{
   if (index<0)
      return 0;

   int j(0);
   for (QListViewItemIterator it=firstChild(); it.current(); it++)
   {
      if (j==index)
         return it.current();
      j++;
   };
   return 0;
};


void KListView::emitContextMenu (KListView*, QListViewItem* i)
{
  QPoint p;

  if (i)
        p = viewport()->mapToGlobal(itemRect(i).center());
  else
        p = mapToGlobal(rect().center());

  emit contextMenu (this, i, p);
}

void KListView::emitContextMenu (QListViewItem* i, const QPoint& p, int)
{
  emit contextMenu (this, i, p);
}

void KListView::setAcceptDrops (bool val)
{
  QListView::setAcceptDrops (val);
  viewport()->setAcceptDrops (val);
}

int KListView::dropVisualizerWidth () const
{
        return d->mDropVisualizerWidth;
}


void KListView::viewportPaintEvent(QPaintEvent *e)
{
  QListView::viewportPaintEvent(e);

  if (d->mOldDropVisualizer.isValid() && e->rect().intersects(d->mOldDropVisualizer))
    {
      QPainter painter(viewport());

      // This is where we actually draw the drop-visualizer
      painter.fillRect(d->mOldDropVisualizer, Dense4Pattern);
    }
  if (d->mOldDropHighlighter.isValid() && e->rect().intersects(d->mOldDropHighlighter))
    {
      QPainter painter(viewport());

      // This is where we actually draw the drop-highlighter
      style().drawFocusRect(&painter, d->mOldDropHighlighter, colorGroup(), 0, true);
    }
}

void KListView::setFullWidth()
{
  d->fullWidth = true;
  header()->setResizeEnabled(false);
}

void KListView::resizeEvent(QResizeEvent* e)
{
  QListView::resizeEvent(e);
  if (d->fullWidth && columns())
     setColumnWidth( 0, visibleWidth() - 1 );
}

const QColor &KListView::alternateBackground() const
{
  return d->alternateBackground;
}

void KListView::setAlternateBackground(const QColor &c)
{
  d->alternateBackground = c;
  repaint();
}


void KListView::setValidator(const QValidator* validator)
{
  d->editor->setValidator( validator );
}


struct KListViewItem::KListViewItemPrivate
{
  bool odd : 1;
  bool known : 1;
  bool renamable;
};

KListViewItem::KListViewItem(QListView *parent)
  : QListViewItem(parent)
{
  init();
}

KListViewItem::KListViewItem(QListViewItem *parent)
  : QListViewItem(parent)
{
  init();
}

KListViewItem::KListViewItem(QListView *parent, QListViewItem *after)
  : QListViewItem(parent, after)
{
  init();
}

KListViewItem::KListViewItem(QListViewItem *parent, QListViewItem *after)
  : QListViewItem(parent, after)
{
  init();
}

KListViewItem::KListViewItem(QListView *parent,
    QString label1, QString label2, QString label3, QString label4,
    QString label5, QString label6, QString label7, QString label8)
  : QListViewItem(parent, label1, label2, label3, label4, label5, label6, label7, label8)
{
  init();
}

KListViewItem::KListViewItem(QListViewItem *parent,
    QString label1, QString label2, QString label3, QString label4,
    QString label5, QString label6, QString label7, QString label8)
  : QListViewItem(parent, label1, label2, label3, label4, label5, label6, label7, label8)
{
  init();
}

KListViewItem::KListViewItem(QListView *parent, QListViewItem *after,
    QString label1, QString label2, QString label3, QString label4,
    QString label5, QString label6, QString label7, QString label8)
  : QListViewItem(parent, after, label1, label2, label3, label4, label5, label6, label7, label8)
{
  init();
}

KListViewItem::KListViewItem(QListViewItem *parent, QListViewItem *after,
    QString label1, QString label2, QString label3, QString label4,
    QString label5, QString label6, QString label7, QString label8)
  : QListViewItem(parent, after, label1, label2, label3, label4, label5, label6, label7, label8)
{
  init();
}

KListViewItem::~KListViewItem()
{
  delete d;
}

void KListViewItem::init()
{
  d = new KListViewItemPrivate;
  d->known = false;
  d->renamable = true;
}

const QColor &KListViewItem::backgroundColor()
{
  KListView *lv = dynamic_cast<KListView *>(listView());
  if (lv && lv->alternateBackground().isValid())
  {
    KListViewItem *above = 0;
    QStack<KListViewItem> items;
    for (KListViewItem *item = this; item; item = above)
    {
      items.push(item);
      above = dynamic_cast<KListViewItem *>(item->itemAbove());
      if (!above || above->d->known)
        break;
    }
    while (items.top())
    {
      items.top()->d->odd = above ? !above->d->odd : false;
      items.top()->d->known = true;
      above = items.pop();
    }
    if (d->odd)
      return lv->alternateBackground();
  }
  return listView()->viewport()->colorGroup().base();
}

void KListViewItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment)
{
  QColorGroup _cg = cg;
  const QPixmap *pm = listView()->viewport()->backgroundPixmap();
  if (pm && !pm->isNull())
  {
        _cg.setBrush(QColorGroup::Base, QBrush(backgroundColor(), *pm));
        p->setBrushOrigin( -listView()->contentsX(), -listView()->contentsY() );
  }
  else
        _cg.setColor(QColorGroup::Base, backgroundColor());
  
  QListViewItem::paintCell(p, _cg, column, width, alignment);
}

void KListViewItem::setRenamable( bool b )
{
  d->renamable = b;
}

bool KListViewItem::isRenamable() const
{
  return d->renamable;
}


#include "klistview.moc"
#include "klistviewlineedit.moc"

// vim: ts=2 sw=2 et
