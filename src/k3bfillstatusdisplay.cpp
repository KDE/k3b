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


#include "k3bfillstatusdisplay.h"
#include "audio/k3baudiodoc.h"
#include "k3bcore.h"

#include <tools/k3bdeviceselectiondialog.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicehandler.h>
#include <device/k3bmsf.h>

#include <qevent.h>
#include <qpainter.h>
#include <qcolor.h>
#include <qrect.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qvalidator.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qlayout.h>

#include <kaction.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <klineeditdlg.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kio/global.h>
#include <kmessagebox.h>


K3bFillStatusDisplayWidget::K3bFillStatusDisplayWidget( K3bDoc* doc, QWidget* parent )
  : QWidget( parent ),
    m_doc(doc)
{
  k3bcore->config()->setGroup( "General Options" );
  m_cdSize.addMinutes( k3bcore->config()->readNumEntry( "Default cd size", 74 ) );

  setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred ) );	
}


K3bFillStatusDisplayWidget::~K3bFillStatusDisplayWidget()
{
}


void K3bFillStatusDisplayWidget::setShowTime( bool b )
{
  m_showTime = b;
  update();
}


void K3bFillStatusDisplayWidget::setCdSize( const K3b::Msf& size )
{
  m_cdSize = size;
  update();
}


QSize K3bFillStatusDisplayWidget::sizeHint() const
{
  return minimumSizeHint();
}


QSize K3bFillStatusDisplayWidget::minimumSizeHint() const
{
  int margin = 2;
  QFontMetrics fm( font() );
  return QSize( -1, fm.height() + 2 * margin );
}


void K3bFillStatusDisplayWidget::mousePressEvent( QMouseEvent* e )
{
  if( e->button() == Qt::RightButton )
    emit contextMenu( e->globalPos() );
}


void K3bFillStatusDisplayWidget::paintEvent( QPaintEvent* )
{
  erase( rect() );

  QPainter p(this);

  long long docSize;
  long long cdSize;
  long long maxValue;
  long tolerance;

  if( m_showTime ) {
    docSize = m_doc->length().totalFrames() / 75 / 60;
    cdSize = m_cdSize.totalFrames() / 75 / 60;
    maxValue = (cdSize > docSize ? cdSize : docSize) + 10;
    tolerance = 1;
  }
  else {
    docSize = m_doc->size()/1024/1024;
    cdSize = m_cdSize.mode1Bytes()/1024/1024;
    maxValue = (cdSize > docSize ? cdSize : docSize) + 100;
    tolerance = 10;
  }

  // so split width() in maxValue pieces
  double one = (double)rect().width() / (double)maxValue;
  QRect crect( rect() );
  crect.setWidth( (int)(one*(double)docSize) );
	
  p.fillRect( crect, Qt::green );

  if( m_showTime )
    p.drawText( rect(), Qt::AlignLeft | Qt::AlignVCenter, 
		 " " + K3b::Msf( m_doc->length() ).toString(false) + " min" );
  else
    p.drawText( rect(), Qt::AlignLeft | Qt::AlignVCenter, 
		 " " + KIO::convertSize( m_doc->size() ) );
	
  // draw yellow if cdSize - tolerance < docSize
  if( docSize > cdSize - tolerance ) {
    crect.setLeft( crect.left() + (int)(one * (cdSize - tolerance)) );
    p.fillRect( crect, Qt::yellow );
  }
	
  // draw red if docSize > cdSize + tolerance
  if( docSize > cdSize + tolerance ) {
    crect.setLeft( crect.left() + (int)(one * tolerance*2) );
    p.fillRect( crect, Qt::red );
  }
	

  p.drawLine( rect().left() + (int)(one*cdSize), rect().bottom(), 
	       rect().left() + (int)(one*cdSize), rect().top() );
	
  // draw the text marks
  crect = rect();
  QString text = QString::number((long)cdSize);
  int textLength = fontMetrics().width(text);
  if( textLength+4 > crect.width() - (int)(one*cdSize) ) {
    // we don't have enough space on the right, so we paint to the left of the line
    crect.setLeft( (int)(one*cdSize) - textLength -4 );
  }
  else
    crect.setLeft( (int)(one*cdSize) + 4 );
  p.drawText( crect, Qt::AlignLeft | Qt::AlignVCenter, text );
}



// ----------------------------------------------------------------------------------------------------


class K3bFillStatusDisplay::Private
{
public:
  KActionCollection* actionCollection;
  KToggleAction* actionShowMinutes;
  KToggleAction* actionShowMegs;
  KToggleAction* action74Min;
  KToggleAction* action80Min;
  KToggleAction* action100Min;
  KToggleAction* actionDvd4_7GB;
  KToggleAction* actionCustomSize;
  KAction* actionDetermineSize;

  KPopupMenu* popup;
  KPopupMenu* dvdPopup;

  QToolButton* buttonMenu;

  K3bFillStatusDisplayWidget* displayWidget;

  bool showDvdSizes;
};


K3bFillStatusDisplay::K3bFillStatusDisplay(K3bDoc* doc, QWidget *parent, const char *name )
  : QFrame(parent,name)
{
  d = new Private;
  showDvdSizes( false );

  setFrameStyle( Panel | Sunken );

  d->displayWidget = new K3bFillStatusDisplayWidget( doc, this );
  d->buttonMenu = new QToolButton( this );
  d->buttonMenu->setIconSet( SmallIconSet("cdrom_unmount") );
  d->buttonMenu->setAutoRaise(true);
  QToolTip::add( d->buttonMenu, i18n("Fill display properties") );
  connect( d->buttonMenu, SIGNAL(clicked()), this, SLOT(slotMenuButtonClicked()) );

  QGridLayout* layout = new QGridLayout( this );
  layout->setSpacing(5);
  layout->setMargin(frameWidth());
  layout->addWidget( d->displayWidget, 0, 0 );
  layout->addWidget( d->buttonMenu, 0, 1 );
  layout->setColStretch( 0, 1 );

  setupPopupMenu();

  // defaults to megabytes
  d->displayWidget->setShowTime(false);
  d->actionShowMegs->setChecked( true );

  switch( d->displayWidget->cdSize().totalFrames()/75/60 ) {
  case 74:
    d->action74Min->setChecked( true );
    break;
  case 80:
    d->action80Min->setChecked( true );
    break;
  case 100:
    d->action100Min->setChecked( true );
    break;
  default:
    d->actionCustomSize->setChecked( true );
    break;
  }
}

K3bFillStatusDisplay::~K3bFillStatusDisplay()
{
  delete d;
}


void K3bFillStatusDisplay::paintEvent(QPaintEvent* e)
{
  // just to pass updates to the display
  d->displayWidget->update();
  QFrame::paintEvent(e);
}

void K3bFillStatusDisplay::setupPopupMenu()
{
  d->actionCollection = new KActionCollection( this );

  // we use a nother popup for the dvd sizes
  d->popup = new KPopupMenu( this, "popup" );
  d->dvdPopup = new KPopupMenu( this, "dvdpopup" );

  d->actionShowMinutes = new KToggleAction( i18n("Minutes"), "kmidi", 0, this, SLOT(showTime()), 
					   d->actionCollection, "fillstatus_show_minutes" );
  d->actionShowMegs = new KToggleAction( i18n("Megabytes"), "kwikdisk", 0, this, SLOT(showSize()), 
					d->actionCollection, "fillstatus_show_megabytes" );

  d->actionShowMegs->setExclusiveGroup( "show_size_in" );
  d->actionShowMinutes->setExclusiveGroup( "show_size_in" );

  d->action74Min = new KToggleAction( i18n("%1 MB").arg(650), 0, this, SLOT(slot74Minutes()), 
				     d->actionCollection, "fillstatus_74minutes" );
  d->action80Min = new KToggleAction( i18n("%1 MB").arg(700), 0, this, SLOT(slot80Minutes()), 
				     d->actionCollection, "fillstatus_80minutes" );
  d->action100Min = new KToggleAction( i18n("%1 MB").arg(880), 0, this, SLOT(slot100Minutes()), 
				      d->actionCollection, "fillstatus_100minutes" );
  d->actionDvd4_7GB = new KToggleAction( i18n("4.7 GB"), 0, this, SLOT(slotDvd4_7GB()), 
					 d->actionCollection, "fillstatus_dvd_4_7gb" );
  d->actionCustomSize = new KToggleAction( i18n("Custom..."), 0, this, SLOT(slotCustomSize()),
					  d->actionCollection, "fillstatus_custom_size" );

  d->action74Min->setExclusiveGroup( "cd_size" );
  d->action80Min->setExclusiveGroup( "cd_size" );
  d->action100Min->setExclusiveGroup( "cd_size" );
  d->actionDvd4_7GB->setExclusiveGroup( "cd_size" );
  d->actionCustomSize->setExclusiveGroup( "cd_size" );

  d->actionDetermineSize = new KAction( i18n("From Disk..."), "cdrom_unmount", 0,
				       this, SLOT(slotDetermineSize()), 
				       d->actionCollection, "fillstatus_size_from_disk" );
 
  d->popup->insertTitle( i18n("Show Size In") );
  d->actionShowMinutes->plug( d->popup );
  d->actionShowMegs->plug( d->popup );
  d->popup->insertTitle( i18n("CD size") );
  d->action74Min->plug( d->popup );
  d->action80Min->plug( d->popup );
  d->action100Min->plug( d->popup );
  d->actionCustomSize->plug( d->popup );
  d->actionDetermineSize->plug( d->popup );

  d->dvdPopup->insertTitle( i18n("DVD Size") );
  d->actionDvd4_7GB->plug( d->dvdPopup );
  d->actionCustomSize->plug( d->dvdPopup );
  d->actionDetermineSize->plug( d->dvdPopup );

  connect( d->displayWidget, SIGNAL(contextMenu(const QPoint&)), this, SLOT(slotPopupMenu(const QPoint&)) );
}


void K3bFillStatusDisplay::showSize()
{
  d->actionShowMegs->setChecked( true );

  d->action74Min->setText( i18n("%1 MB").arg(650) );
  d->action80Min->setText( i18n("%1 MB").arg(700) );
  d->action100Min->setText( i18n("%1 MB").arg(880) );

  d->displayWidget->setShowTime(false);
}
	
void K3bFillStatusDisplay::showTime()
{
  d->actionShowMinutes->setChecked( true );

  d->action74Min->setText( i18n("unused", "%n minutes", 74) );
  d->action80Min->setText( i18n("unused", "%n minutes", 80) );
  d->action100Min->setText( i18n("unused", "%n minutes", 100) );

  d->displayWidget->setShowTime(true);
}


void K3bFillStatusDisplay::showDvdSizes( bool b )
{
  d->showDvdSizes = b;
}


void K3bFillStatusDisplay::slot74Minutes()
{
  d->displayWidget->setCdSize( 74*60*75 );
}


void K3bFillStatusDisplay::slot80Minutes()
{
  d->displayWidget->setCdSize( 80*60*75 );
}


void K3bFillStatusDisplay::slot100Minutes()
{
  d->displayWidget->setCdSize( 100*60*75 );
}


void K3bFillStatusDisplay::slotDvd4_7GB()
{
  d->displayWidget->setCdSize( 510*60*75 );
}

void K3bFillStatusDisplay::slotCustomSize()
{
  bool ok;
  QString size = KLineEditDlg::getText( i18n("Custom CD Size"), 
					i18n("Please specify the size of your CD in minutes:"), 
					"74", &ok, this, new QIntValidator( this ) );
  if( ok ) {
    d->displayWidget->setCdSize( size.toInt()*60*75 );
    update();
  }
}


void K3bFillStatusDisplay::slotMenuButtonClicked()
{
  QSize size = d->showDvdSizes ? d->dvdPopup->sizeHint() : d->popup->sizeHint();
  slotPopupMenu( d->buttonMenu->mapToGlobal(QPoint(d->buttonMenu->width(), 0)) +
		 QPoint(-1*size.width(), -1*size.height()) );
}


void K3bFillStatusDisplay::slotPopupMenu( const QPoint& p )
{
  if( d->showDvdSizes )
    d->dvdPopup->popup(p);
  else
    d->popup->popup(p);
}


void K3bFillStatusDisplay::slotDetermineSize()
{
  K3bDevice* dev = K3bDeviceSelectionDialog::selectWriter( parentWidget() );
  if( dev ) {
    connect( K3bCdDevice::sendCommand( K3bCdDevice::DeviceHandler::REMAININGSIZE, dev ),
	     SIGNAL(finished(K3bCdDevice::DeviceHandler*)),
	     this,
	     SLOT(slotRemainingSize(K3bCdDevice::DeviceHandler*)) );
  }
}

void K3bFillStatusDisplay::slotRemainingSize( K3bCdDevice::DeviceHandler* dh )
{
  if( dh->success() ) {
    K3b::Msf size = dh->remainingSize();
    d->displayWidget->setCdSize( size );
    d->actionCustomSize->setChecked(true);
    update();
  }
  else {
    KMessageBox::error( parentWidget(), i18n("Could not get remaining size of disk.") );
  }
}


#include "k3bfillstatusdisplay.moc"
