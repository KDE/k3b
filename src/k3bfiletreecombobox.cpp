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


#include "k3bfiletreecombobox.h"
#include "k3bfiletreeview.h"

#include <device/k3bdevicemanager.h>
#include <device/k3bdevice.h>

#include <qrect.h>
#include <qapplication.h>
#include <qstyle.h>
#include <qlistbox.h>
#include <qheader.h>
#include <qevent.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qdrawutil.h>

#include <kdebug.h>
#include <kiconloader.h>


class K3bFileTreeComboBox::Private
{
public:
  Private() {
    poppedUp = false;
    ignoreNextMouseClick = false;
  }
  bool poppedUp;
  bool ignoreNextMouseClick; // used when the view was hidden with the arrow button
};


K3bFileTreeComboBox::K3bFileTreeComboBox( QWidget* parent, const char* name )
  : KComboBox( true, parent, name )
{
  d = new Private;

  m_fileTreeView = new K3bFileTreeView( this );
  m_fileTreeView->hide();
  m_fileTreeView->reparent( this, WType_Popup, QPoint(0,0), FALSE );
  m_fileTreeView->header()->hide();
  m_fileTreeView->installEventFilter(this);

  m_fileTreeView->addCdDeviceBranches( K3bDeviceManager::self() );
  m_fileTreeView->addDefaultBranches();

  // HACK! Why the hell is QComboBox that closed???
  listBox()->insertItem( "HACK" );

  connect( m_fileTreeView, SIGNAL(deviceExecuted(K3bDevice*)),
	   this, SLOT(slotDeviceExecuted(K3bDevice*)) );
  connect( m_fileTreeView, SIGNAL(urlExecuted(const KURL&)),
	   this, SLOT(slotUrlExecuted(const KURL&)) );

  connect( lineEdit(), SIGNAL(returnPressed()),
	   this, SLOT(slotGoUrl()) );

  // TODO: subclass KURLCompletition to support the dev:/ stuff and block any non-local urls
}


K3bFileTreeComboBox::~K3bFileTreeComboBox()
{
  delete d;
}


void K3bFileTreeComboBox::slotDeviceExecuted( K3bDevice* dev )
{
  setEditText( SmallIcon("cdrom_unmount"), dev->vendor() + " " + dev->description() + " (" + dev->ioctlDevice() + ")" );
  popdown();
  emit deviceExecuted( dev );
}


void K3bFileTreeComboBox::slotUrlExecuted( const KURL& url )
{
  setEditText( SmallIcon("folder"), url.path() );
  popdown();
  emit urlExecuted( url );
}


void K3bFileTreeComboBox::setEditText( const QPixmap& pix, const QString& t )
{
  // QComboBox::changeItem() doesn't honour the pixmap when
  // using an editable combobox, so we just remove and insert
  
  setUpdatesEnabled( false );
  lineEdit()->setUpdatesEnabled( false );

  removeItem(0);
  insertItem( pix, t, 0 );
  lineEdit()->setText( t );

  setUpdatesEnabled( true );
  lineEdit()->setUpdatesEnabled( true );
  update();
}


void K3bFileTreeComboBox::setCurrentItem( int )
{
  // the current item is always 0 due to the ugly but quite smart HACK ;)
}


void K3bFileTreeComboBox::setCurrentText( const QString& )
{
}


void K3bFileTreeComboBox::popup()
{
  // code mainly from qcombobox.cpp

  m_fileTreeView->triggerUpdate();
  int w = QMAX( m_fileTreeView->sizeHint().width(), width() );
  int h = m_fileTreeView->sizeHint().height();
  QRect screen = QApplication::desktop()->availableGeometry( this );
  int sx = screen.x();				// screen pos
  int sy = screen.y();
  int sw = screen.width();			// screen width
  int sh = screen.height();			// screen height
  QPoint pos = mapToGlobal( QPoint(0,height()) );
  // ## Similar code is in QPopupMenu
  int x = pos.x();
  int y = pos.y();

  // the complete widget must be visible
  if ( x + w > sx + sw )
    x = sx+sw - w;
  if ( x < sx )
    x = sx;
  if (y + h > sy+sh && y - h - height() >= 0 )
    y = y - h - height();

  QRect rect =
    style().querySubControlMetrics( QStyle::CC_ComboBox, this,
				    QStyle::SC_ComboBoxListBoxPopup,
				    QStyleOption( x, y, w, h ) );
  // work around older styles that don't implement the combobox
  // listbox popup subcontrol
  if ( rect.isNull() )
    rect.setRect( x, y, w, h );

  m_fileTreeView->setGeometry( rect );
  m_fileTreeView->raise();

  // TODO: somehow set the current item

  m_fileTreeView->show();
  d->poppedUp = true;
  d->ignoreNextMouseClick = false;
}


void K3bFileTreeComboBox::popdown()
{
  m_fileTreeView->hide();
  d->poppedUp = false;
  repaint(); // repaint the arrow
}


void K3bFileTreeComboBox::slotGoUrl()
{
  // TODO: check if it's a device or an url
  KURL url;
  url.setPath( currentText() );
  emit urlExecuted( url );
}


bool K3bFileTreeComboBox::eventFilter( QObject* o, QEvent* e )
{
  if( dynamic_cast<K3bFileTreeView*>(o) == m_fileTreeView ) {
    if( e->type() == QEvent::DragLeave ) {
      // the user dragged a dir from the filetree
      // now 
      popdown();
      return true;
    }
    else if( e->type() == QEvent::KeyPress ) {
      QKeyEvent *k = (QKeyEvent *)e;
      if( k->key() == Qt::Key_Escape ) {
	popdown();
	return true;
      }
    }
    else if( e->type() == QEvent::MouseButtonPress ) {
      QMouseEvent* me = (QMouseEvent*)e;
      if ( !m_fileTreeView->rect().contains( me->pos() ) ) {
	QRect arrowRect = style().querySubControlMetrics( QStyle::CC_ComboBox, this,
							  QStyle::SC_ComboBoxArrow);
	arrowRect = QStyle::visualRect(arrowRect, this);
	
	// Correction for motif style, where arrow is smaller
	// and thus has a rect that doesn't fit the button.
	arrowRect.setHeight( QMAX(  height() - (2 * arrowRect.y()), arrowRect.height() ) );

	if ( arrowRect.contains( mapFromGlobal(me->globalPos()) ) ) {
	  d->ignoreNextMouseClick = true;  // in the case we hit the arrow button
	}
	popdown();
      }
      return false; // we need this in the listview
    }

    return false;
  }
  else
    return KComboBox::eventFilter(o, e);
}


void K3bFileTreeComboBox::mousePressEvent( QMouseEvent* e )
{
  // mainly from qcombobox.cpp

  if ( e->button() != LeftButton )
    return;
  if ( d->ignoreNextMouseClick ) {
    d->ignoreNextMouseClick = FALSE;
    return;
  }

  QRect arrowRect = style().querySubControlMetrics( QStyle::CC_ComboBox, this,
						    QStyle::SC_ComboBoxArrow);
  arrowRect = QStyle::visualRect(arrowRect, this);

  // Correction for motif style, where arrow is smaller
  // and thus has a rect that doesn't fit the button.
  arrowRect.setHeight( QMAX(  height() - (2 * arrowRect.y()), arrowRect.height() ) );

  if ( arrowRect.contains( e->pos() ) ) {
    popup();
    repaint( FALSE );
  }
}


void K3bFileTreeComboBox::keyPressEvent( QKeyEvent* e )
{
  if( e->key() == Qt::Key_Escape ) {
    popdown();
  }
  KComboBox::keyPressEvent(e);
}


void K3bFileTreeComboBox::paintEvent( QPaintEvent* )
{
  // a lot of code from qcombobox.cpp

  // we only need this since there is no way to change the status of the arrow-button

  QPainter p( this );
  const QColorGroup & g = colorGroup();
  p.setPen(g.text());

  QStyle::SFlags flags = QStyle::Style_Default;
  if (isEnabled())
    flags |= QStyle::Style_Enabled;
  if (hasFocus())
    flags |= QStyle::Style_HasFocus;

  if ( width() < 5 || height() < 5 ) {
    qDrawShadePanel( &p, rect(), g, FALSE, 2,
		     &g.brush( QColorGroup::Button ) );
    return;
  }

  //  bool reverse = QApplication::reverseLayout();

  style().drawComplexControl( QStyle::CC_ComboBox, &p, this, rect(), g,
			      flags, QStyle::SC_All,
			      (d->poppedUp ?
			       QStyle::SC_ComboBoxArrow :
			       QStyle::SC_None ));

  QRect re = style().querySubControlMetrics( QStyle::CC_ComboBox, this,
					     QStyle::SC_ComboBoxEditField );
  re = QStyle::visualRect(re, this);
  p.setClipRect( re );

//     QListBoxItem * item = listBox()->item( 0 );
//     if ( item ) {
//       // we calculate the QListBoxTexts height (ignoring strut)
//       int itemh = d->listBox()->fontMetrics().lineSpacing() + 2;
//       p.translate( re.x(), re.y() + (re.height() - itemh)/2  );
//       item->paint( &p );
//     }
//   } else if ( d->listBox() && d->listBox()->item( 0 ) ) {
    p.setClipping( FALSE );
    QListBoxItem * item = listBox()->item( 0 );
    const QPixmap *pix = item->pixmap();
    if ( pix ) {
      p.fillRect( re.x(), re.y(), pix->width() + 4, re.height(),
		  colorGroup().brush( QColorGroup::Base ) );
      p.drawPixmap( re.x() + 2, re.y() +
		    ( re.height() - pix->height() ) / 2, *pix );
    }
//   }
  p.setClipping( FALSE );
}


#include "k3bfiletreecombobox.moc"

