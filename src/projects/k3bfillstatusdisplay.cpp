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
#include "k3bdoc.h"

#include <k3bcore.h>
#include <k3bdeviceselectiondialog.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicemanager.h>
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
#include <kglobal.h>


class K3bFillStatusDisplayWidget::Private
{
public:
  K3b::Msf cdSize;
  bool showTime;
  K3bDoc* doc;
};


K3bFillStatusDisplayWidget::K3bFillStatusDisplayWidget( K3bDoc* doc, QWidget* parent )
  : QWidget( parent )
{
  d = new Private();
  d->doc = doc;
  setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred ) );	
}


K3bFillStatusDisplayWidget::~K3bFillStatusDisplayWidget()
{
  delete d;
}


const K3b::Msf& K3bFillStatusDisplayWidget::cdSize() const
{
  return d->cdSize;
}


void K3bFillStatusDisplayWidget::setShowTime( bool b )
{
  d->showTime = b;
  update();
}


void K3bFillStatusDisplayWidget::setCdSize( const K3b::Msf& size )
{
  d->cdSize = size;
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

  if( d->showTime ) {
    docSize = d->doc->length().totalFrames() / 75 / 60;
    cdSize = d->cdSize.totalFrames() / 75 / 60;
    maxValue = (cdSize > docSize ? cdSize : docSize) + 10;
    tolerance = 1;
  }
  else {
    docSize = d->doc->size()/1024/1024;
    cdSize = d->cdSize.mode1Bytes()/1024/1024;
    maxValue = (cdSize > docSize ? cdSize : docSize) + 100;
    tolerance = 10;
  }

  // so split width() in maxValue pieces
  double one = (double)rect().width() / (double)maxValue;
  QRect crect( rect() );
  crect.setWidth( (int)(one*(double)docSize) );
	
  p.fillRect( crect, Qt::green );

  QRect oversizeRect(crect);
  // draw yellow if cdSize - tolerance < docSize
  if( docSize > cdSize - tolerance ) {
    oversizeRect.setLeft( oversizeRect.left() + (int)(one * (cdSize - tolerance)) );
    p.fillRect( oversizeRect, Qt::yellow );
  }
	
  // draw red if docSize > cdSize + tolerance
  if( docSize > cdSize + tolerance ) {
    oversizeRect.setLeft( oversizeRect.left() + (int)(one * tolerance*2) );
    p.fillRect( oversizeRect, Qt::red );
  }


  QString docSizeText;
  if( d->showTime )
    docSizeText = K3b::Msf( d->doc->length() ).toString(false) + " " + i18n("min");
  else
    docSizeText = KIO::convertSize( d->doc->size() );

  //
  // we want to draw the docSizeText centered in the filled area
  // if there is not enough space we just align it left
  //
  int docSizeTextLength = fontMetrics().width(docSizeText);
  if( docSizeTextLength + 8 > crect.width() )
    p.drawText( rect(), Qt::AlignLeft | Qt::AlignVCenter, 
		" " + docSizeText );
  else
    p.drawText( crect, Qt::AlignHCenter | Qt::AlignVCenter, docSizeText );
		

  p.drawLine( rect().left() + (int)(one*cdSize), rect().bottom(), 
	       rect().left() + (int)(one*cdSize), rect().top() + ((rect().bottom()-rect().top())/2) );
	
  // draw the text marks
  crect = rect();
  QString text = i18n("Available: %1 of %2")
    .arg( d->showTime 
	  ? i18n("%1 min").arg((K3b::Msf( cdSize*60*75 ) - d->doc->length()).toString(false))
	  : KIO::convertSize( QMAX( (cdSize * 1024LL * 1024LL) - (long long)d->doc->size(), 0LL ) ) )
    .arg( d->showTime 
	  ? i18n("%1 min").arg(K3b::Msf( cdSize*60*75 ).toString(false))
	  : KIO::convertSizeFromKB( cdSize * 1024 ) );

  QFont fnt(font());
  fnt.setPointSize(8);
  fnt.setBold(false);
  p.setPen( Qt::gray );
  p.setFont(fnt);

  int textLength = QFontMetrics(fnt).width(text);
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
  KAction* actionSaveUserDefaults;
  KAction* actionLoadUserDefaults;

  KPopupMenu* popup;
  KPopupMenu* dvdPopup;

  QToolButton* buttonMenu;

  K3bFillStatusDisplayWidget* displayWidget;

  bool showDvdSizes;
  bool showTime;

  K3bDoc* doc;
};


K3bFillStatusDisplay::K3bFillStatusDisplay(K3bDoc* doc, QWidget *parent, const char *name )
  : QFrame(parent,name)
{
  d = new Private;
  d->doc = doc;

  setFrameStyle( Panel | Sunken );

  d->displayWidget = new K3bFillStatusDisplayWidget( doc, this );
//   d->buttonMenu = new QToolButton( this );
//   d->buttonMenu->setIconSet( SmallIconSet("cdrom_unmount") );
//   d->buttonMenu->setAutoRaise(true);
//   QToolTip::add( d->buttonMenu, i18n("Fill display properties") );
//   connect( d->buttonMenu, SIGNAL(clicked()), this, SLOT(slotMenuButtonClicked()) );

  QGridLayout* layout = new QGridLayout( this );
  layout->setSpacing(5);
  layout->setMargin(frameWidth());
  layout->addWidget( d->displayWidget, 0, 0 );
  //  layout->addWidget( d->buttonMenu, 0, 1 );
  layout->setColStretch( 0, 1 );

  setupPopupMenu();

  showDvdSizes( false );

  connect( d->doc, SIGNAL(changed()), this, SLOT(slotDocSizeChanged()) );
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

  d->actionLoadUserDefaults = new KAction( i18n("User Defaults"), "", 0, 
					   this, SLOT(slotLoadUserDefaults()),
					   d->actionCollection, "load_user_defaults" );
  d->actionSaveUserDefaults = new KAction( i18n("Save User Defaults"), "", 0, 
					   this, SLOT(slotSaveUserDefaults()),
					   d->actionCollection, "save_user_defaults" );
 
  d->popup->insertTitle( i18n("Show Size In") );
  d->actionShowMinutes->plug( d->popup );
  d->actionShowMegs->plug( d->popup );
  d->popup->insertTitle( i18n("CD Size") );
  d->action74Min->plug( d->popup );
  d->action80Min->plug( d->popup );
  d->action100Min->plug( d->popup );
  d->actionCustomSize->plug( d->popup );
  d->actionDetermineSize->plug( d->popup );
  d->popup->insertSeparator();
  d->actionLoadUserDefaults->plug( d->popup );
  d->actionSaveUserDefaults->plug( d->popup );

  d->dvdPopup->insertTitle( i18n("DVD Size") );
  d->actionDvd4_7GB->plug( d->dvdPopup );
  d->actionCustomSize->plug( d->dvdPopup );
  d->actionDetermineSize->plug( d->dvdPopup );
  d->dvdPopup->insertSeparator();
  d->actionLoadUserDefaults->plug( d->dvdPopup );
  d->actionSaveUserDefaults->plug( d->dvdPopup );

  connect( d->displayWidget, SIGNAL(contextMenu(const QPoint&)), this, SLOT(slotPopupMenu(const QPoint&)) );
}


void K3bFillStatusDisplay::showSize()
{
  d->actionShowMegs->setChecked( true );

  d->action74Min->setText( i18n("%1 MB").arg(650) );
  d->action80Min->setText( i18n("%1 MB").arg(700) );
  d->action100Min->setText( i18n("%1 MB").arg(880) );

  d->showTime = false;
  d->displayWidget->setShowTime(false);
}
	
void K3bFillStatusDisplay::showTime()
{
  d->actionShowMinutes->setChecked( true );

  d->action74Min->setText( i18n("unused", "%n minutes", 74) );
  d->action80Min->setText( i18n("unused", "%n minutes", 80) );
  d->action100Min->setText( i18n("unused", "%n minutes", 100) );

  d->showTime = true;
  d->displayWidget->setShowTime(true);
}


void K3bFillStatusDisplay::showDvdSizes( bool b )
{
  d->showDvdSizes = b;
  slotLoadUserDefaults();
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
  QString size = KLineEditDlg::getText( i18n("Custom Size"), 
					i18n("Please specify the size of the media in minutes:"), 
					d->showDvdSizes ? "74" : "510", &ok, this, new QIntValidator( this ) );
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
  K3bDevice* dev = K3bDeviceSelectionDialog::selectDevice( parentWidget(), 
							   d->showDvdSizes 
							   ? k3bcore->deviceManager()->dvdWriter() 
							   : k3bcore->deviceManager()->cdWriter() );

  if( dev ) {
    k3bcore->requestBusyInfo( i18n("Determine size of media in %1").arg(dev->vendor() + " " + dev->description() ) );

    connect( K3bCdDevice::sendCommand( K3bCdDevice::DeviceHandler::NG_DISKINFO, dev ),
	     SIGNAL(finished(K3bCdDevice::DeviceHandler*)),
	     this,
	     SLOT(slotRemainingSize(K3bCdDevice::DeviceHandler*)) );
  }
}

void K3bFillStatusDisplay::slotRemainingSize( K3bCdDevice::DeviceHandler* dh )
{
  k3bcore->requestBusyFinish();

  if( dh->success() ) {
    if( dh->ngDiskInfo().diskState() == K3bCdDevice::STATE_NO_MEDIA ) {
      KMessageBox::error( parentWidget(), i18n("No media found.") );
    }
    else {
      K3b::Msf size = dh->ngDiskInfo().remainingSize();
      if( size > 0 ) {    
	d->displayWidget->setCdSize( size );
	d->actionCustomSize->setChecked(true);
	update();
      }
      else
	KMessageBox::error( parentWidget(), i18n("Media is not empty.") );
    }
  }
  else
    KMessageBox::error( parentWidget(), i18n("Could not get remaining size of disk.") );
}


void K3bFillStatusDisplay::slotLoadUserDefaults()
{
  // load project specific values
  KConfig* c = k3bcore->config();
  c->setGroup( "default " + d->doc->documentType() + " settings" );

  // defaults to megabytes
  d->showTime = c->readBoolEntry( "show minutes", false );
  d->displayWidget->setShowTime(d->showTime);
  d->actionShowMegs->setChecked( !d->showTime );
  d->actionShowMinutes->setChecked( d->showTime );


  long size = c->readNumEntry( "default media size", d->showDvdSizes ? 510*60*75 : 80*60*75 );
  d->displayWidget->setCdSize( size );

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
  case 510:
    d->actionDvd4_7GB->setChecked( true );
    break;
  default:
    d->actionCustomSize->setChecked( true );
    break;
  }
}


void K3bFillStatusDisplay::slotSaveUserDefaults()
{
  // save project specific values
  KConfig* c = k3bcore->config();
  c->setGroup( "default " + d->doc->documentType() + " settings" );

  c->writeEntry( "show minutes", d->showTime );
  c->writeEntry( "default media size", d->displayWidget->cdSize().totalFrames() );
}



void K3bFillStatusDisplay::slotDocSizeChanged()
{
  // FIXME: properly localize this
  QToolTip::remove( this );
  QToolTip::add( this, 
		 KIO::convertSize( d->doc->size() ) + 
		 " (" + KGlobal::locale()->formatNumber( d->doc->size(), 0 ) + "), " +
		 K3b::Msf( d->doc->length() ).toString(false) + " min" );
}


#include "k3bfillstatusdisplay.moc"
