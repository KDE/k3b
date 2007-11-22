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


#include "k3bfiletreecombobox.h"
#include "k3bfiletreeview.h"

#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bcore.h>
#include <k3bglobals.h>

#include <qrect.h>
#include <qapplication.h>
#include <qstyle.h>
#include <q3listbox.h>
#include <q3header.h>
#include <qevent.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qdrawutil.h>
#include <qdir.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QPixmap>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDesktopWidget>
#include <QLineEdit>

#include <kdebug.h>
#include <kiconloader.h>
#include <kurlcompletion.h>
#include <kuser.h>


class K3bFileTreeComboBox::Private
{
public:
  Private() {
    poppedUp = false;
    ignoreNextMouseClick = false;
  }
  bool poppedUp;
  bool ignoreNextMouseClick; // used when the view was hidden with the arrow button

  KUrlCompletion* urlCompletion;
};


K3bFileTreeComboBox::K3bFileTreeComboBox( QWidget* parent )
  : KComboBox( true, parent )
{
  d = new Private;

  d->urlCompletion = new KUrlCompletion();
  setCompletionObject( d->urlCompletion );

  m_fileTreeView = new K3bFileTreeView( this );
  m_fileTreeView->hide();
  m_fileTreeView->reparent( this, Qt::WType_Popup, QPoint(0,0), false );
  m_fileTreeView->header()->hide();
  m_fileTreeView->installEventFilter(this);

  m_fileTreeView->addDefaultBranches();
  m_fileTreeView->addCdDeviceBranches( k3bcore->deviceManager() );
  //FIXME kde4
#if 0
  // HACK! Why the hell is QComboBox that closed???
  listBox()->insertItem( "HACK" );
#endif
  connect( m_fileTreeView, SIGNAL(deviceExecuted(K3bDevice::Device*)),
	   this, SLOT(slotDeviceExecuted(K3bDevice::Device*)) );
  connect( m_fileTreeView, SIGNAL(urlExecuted(const KUrl&)),
	   this, SLOT(slotUrlExecuted(const KUrl&)) );

  connect( lineEdit(), SIGNAL(returnPressed()),
	   this, SLOT(slotGoUrl()) );

  // TODO: subclass KURLCompletition to support the dev:/ stuff and block any non-local urls
}


K3bFileTreeComboBox::~K3bFileTreeComboBox()
{
  delete d->urlCompletion;
  delete d;
}


void K3bFileTreeComboBox::slotDeviceExecuted( K3bDevice::Device* dev )
{
  setDevice( dev );
  popdown();
  emit deviceExecuted( dev );
}


void K3bFileTreeComboBox::slotUrlExecuted( const KUrl& url )
{
  setUrl( url );
  emit urlExecuted( url );
}


void K3bFileTreeComboBox::setUrl( const KUrl& url )
{
  setEditText( SmallIcon("folder"), K3b::convertToLocalUrl(url).path() );
  popdown();
}


void K3bFileTreeComboBox::setDevice( K3bDevice::Device* dev )
{
  setEditText( SmallIcon("cdrom_unmount"), dev->vendor() + " " + dev->description() + " (" + dev->blockDeviceName() + ")" );
}


void K3bFileTreeComboBox::setEditText( const QPixmap& pix, const QString& t )
{
  // QComboBox::changeItem() doesn't honor the pixmap when
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
  int w = qMax( m_fileTreeView->sizeHint().width(), width() );
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
  //FIXME kde4
#if 0
  QRect rect =
    style().querySubControlMetrics( QStyle::CC_ComboBox, this,
				    QStyle::SC_ComboBoxListBoxPopup,
				    QStyleOption( x, y, w, h ) );
  // work around older styles that don't implement the combobox
  // listbox popup subcontrol
  if ( rect.isNull() )
    rect.setRect( x, y, w, h );

  m_fileTreeView->setGeometry( rect );
#endif
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
  QString p = currentText();

  // check for a media url or a device string
  if( K3bDevice::Device* dev = K3b::urlToDevice( p ) ) {
    emit deviceExecuted( dev );
    return;
  }

  // check for our own internal format
  else if( p.contains("/dev/") ) {
    int pos1 = p.findRev('(');
    int pos2 = p.findRev(')');
    QString devStr = p.mid( pos1+1, pos2-pos1-1  );
    if( K3bDevice::Device* dev = k3bcore->deviceManager()->findDevice( devStr ) ) {
      emit deviceExecuted( dev );
      return;
    }
  }

  // no device -> select url

  //
  // Properly replace home dirs.
  // a single ~ will be replaced with the current user's home dir 
  // while for example "~ftp" will be replaced by the home dir of user
  // ftp
  //
  // TODO: move this to k3bglobals

  // to expand another user's home dir we need a tilde followed by a user name
  static QRegExp someUsersHomeDir( "\\~([^/]+)" );
  int pos = 0;
  while( ( pos = someUsersHomeDir.search( p, pos ) ) != -1 ) {
    KUser user( someUsersHomeDir.cap(1) );
    if( user.isValid() )
      p.replace( pos, someUsersHomeDir.cap(1).length() + 1, user.homeDir() );
    else
      ++pos; // skip this ~
  }

  // now replace the unmatched tildes with our home dir
  p.replace( "~", K3b::prepareDir( QDir::homePath() ) );


  lineEdit()->setText( p );
  KUrl url;
  url.setPath( p );
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
#if 0
	QRect arrowRect = style().querySubControlMetrics( QStyle::CC_ComboBox, this,
							  QStyle::SC_ComboBoxArrow);
	arrowRect = QStyle::visualRect(arrowRect, this);
	
	// Correction for motif style, where arrow is smaller
	// and thus has a rect that doesn't fit the button.
	arrowRect.setHeight( qMax(  height() - (2 * arrowRect.y()), arrowRect.height() ) );

	if ( arrowRect.contains( mapFromGlobal(me->globalPos()) ) ) {
	  d->ignoreNextMouseClick = true;  // in the case we hit the arrow button
	}
	popdown();
#endif
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

  if ( e->button() != Qt::LeftButton )
    return;
  if ( d->ignoreNextMouseClick ) {
    d->ignoreNextMouseClick = FALSE;
    return;
  }
  //FIXME kde4
#if 0
  QRect arrowRect = style().querySubControlMetrics( QStyle::CC_ComboBox, this,
						    QStyle::SC_ComboBoxArrow);
  arrowRect = QStyle::visualRect(arrowRect, this);

  // Correction for motif style, where arrow is smaller
  // and thus has a rect that doesn't fit the button.
  arrowRect.setHeight( qMax(  height() - (2 * arrowRect.y()), arrowRect.height() ) );

  if ( arrowRect.contains( e->pos() ) ) {
    popup();
    repaint( FALSE );
  }
#endif
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
 //FIXME kde4
#if 0
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
    Q3ListBoxItem * item = listBox()->item( 0 );
    const QPixmap *pix = item->pixmap();
    if ( pix ) {
      p.fillRect( re.x(), re.y(), pix->width() + 4, re.height(),
		  colorGroup().brush( QColorGroup::Base ) );
      p.drawPixmap( re.x() + 2, re.y() +
		    ( re.height() - pix->height() ) / 2, *pix );
    }
//   }
#endif
  p.setClipping( FALSE );
}


#include "k3bfiletreecombobox.moc"

