/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bfillstatusdisplay.h"
#include "k3bdoc.h"

#include <k3bapplication.h>
#include <k3bmediaselectiondialog.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bmsf.h>
#include <k3bradioaction.h>
#include <k3bmediacache.h>

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
#include <qwhatsthis.h>
#include <qtimer.h>

#include <kaction.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <kinputdialog.h>
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
  : QWidget( parent, 0, WRepaintNoErase )
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
  // double buffer
  QPixmap buffer( size() );
  buffer.fill( colorGroup().base() );
  QPainter p;
  p.begin( &buffer, this );
  p.setPen( Qt::black ); // we use a fixed bar color (which is not very nice btw, so we also fix the text color)

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
    docSizeText = d->doc->length().toString(false) + " " + i18n("min");
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
  QString text;
  if( d->cdSize.mode1Bytes() >= d->doc->size() )
    text = i18n("Available: %1 of %2")
      .arg( d->showTime
	    ? i18n("%1 min").arg((K3b::Msf( cdSize*60*75 ) - d->doc->length()).toString(false))
	    : KIO::convertSize( QMAX( (cdSize * 1024LL * 1024LL) - (long long)d->doc->size(), 0LL ) ) )
      .arg( d->showTime
	    ? i18n("%1 min").arg(K3b::Msf( cdSize*60*75 ).toString(false))
	    : KIO::convertSizeFromKB( cdSize * 1024 ) );
  else
    text = i18n("Capacity exceeded by %1")
      .arg( d->showTime
	    ? i18n("%1 min").arg( (d->doc->length() - K3b::Msf( cdSize*60*75 ) ).toString(false))
	    : KIO::convertSize( (long long)d->doc->size() - (cdSize * 1024LL * 1024LL) ) );

  QFont fnt(font());
  fnt.setPointSize(8);
  fnt.setBold(false);
  p.setFont(fnt);

  int textLength = QFontMetrics(fnt).width(text);
  if( textLength+4 > crect.width() - (int)(one*cdSize) ) {
    // we don't have enough space on the right, so we paint to the left of the line
    crect.setLeft( (int)(one*cdSize) - textLength -4 );
  }
  else
    crect.setLeft( (int)(one*cdSize) + 4 );
  p.drawText( crect, Qt::AlignLeft | Qt::AlignVCenter, text );

  p.end();

  bitBlt( this, 0, 0, &buffer );
}



// ----------------------------------------------------------------------------------------------------


class K3bFillStatusDisplay::ToolTip : public QToolTip
{
public:
  ToolTip( K3bDoc* doc, QWidget* parent )
    : QToolTip( parent, 0 ),
      m_doc(doc) {
  }

  void maybeTip( const QPoint& ) {
    tip( parentWidget()->rect(),
	 KIO::convertSize( m_doc->size() ) +
	 " (" + KGlobal::locale()->formatNumber( m_doc->size(), 0 ) + "), " +
	 m_doc->length().toString(false) + " " + i18n("min") +
	 " (" + i18n("Right click for media sizes") + ")");
  }

private:
  K3bDoc* m_doc;
};

class K3bFillStatusDisplay::Private
{
public:
  KActionCollection* actionCollection;
  KRadioAction* actionShowMinutes;
  KRadioAction* actionShowMegs;
  KRadioAction* actionAuto;
  KRadioAction* action74Min;
  KRadioAction* action80Min;
  KRadioAction* action100Min;
  KRadioAction* actionDvd4_7GB;
  KRadioAction* actionDvdDoubleLayer;
  K3bRadioAction* actionCustomSize;
  K3bRadioAction* actionDetermineSize;
  KAction* actionSaveUserDefaults;
  KAction* actionLoadUserDefaults;

  KPopupMenu* popup;
  KPopupMenu* dvdPopup;

  QToolButton* buttonMenu;

  K3bFillStatusDisplayWidget* displayWidget;

  bool showDvdSizes;
  bool showTime;

  K3bDoc* doc;

  QTimer updateTimer;
};


K3bFillStatusDisplay::K3bFillStatusDisplay( K3bDoc* doc, QWidget *parent, const char *name )
  : QFrame(parent,name)
{
  d = new Private;
  d->doc = doc;

  m_toolTip = new ToolTip( doc, this );

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

  connect( d->doc, SIGNAL(changed()), this, SLOT(slotDocChanged()) );
  connect( &d->updateTimer, SIGNAL(timeout()), d->displayWidget, SLOT(update()) );
  connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3bDevice::Device*)),
	   this, SLOT(slotMediumChanged(K3bDevice::Device*)) );
}

K3bFillStatusDisplay::~K3bFillStatusDisplay()
{
  delete d;
  delete m_toolTip;
}


void K3bFillStatusDisplay::setupPopupMenu()
{
  d->actionCollection = new KActionCollection( this );

  // we use a nother popup for the dvd sizes
  d->popup = new KPopupMenu( this, "popup" );
  d->dvdPopup = new KPopupMenu( this, "dvdpopup" );

  d->actionShowMinutes = new KRadioAction( i18n("Minutes"), 0, this, SLOT(showTime()),
					   d->actionCollection, "fillstatus_show_minutes" );
  d->actionShowMegs = new KRadioAction( i18n("Megabytes"), 0, this, SLOT(showSize()),
					d->actionCollection, "fillstatus_show_megabytes" );

  d->actionShowMegs->setExclusiveGroup( "show_size_in" );
  d->actionShowMinutes->setExclusiveGroup( "show_size_in" );

  d->actionAuto = new KRadioAction( i18n("Auto"), 0, this, SLOT(slotAutoSize()),
				    d->actionCollection, "fillstatus_auto" );
  d->action74Min = new KRadioAction( i18n("%1 MB").arg(650), 0, this, SLOT(slot74Minutes()),
				     d->actionCollection, "fillstatus_74minutes" );
  d->action80Min = new KRadioAction( i18n("%1 MB").arg(700), 0, this, SLOT(slot80Minutes()),
				     d->actionCollection, "fillstatus_80minutes" );
  d->action100Min = new KRadioAction( i18n("%1 MB").arg(880), 0, this, SLOT(slot100Minutes()),
				      d->actionCollection, "fillstatus_100minutes" );
  d->actionDvd4_7GB = new KRadioAction( KIO::convertSizeFromKB((int)(4.4*1024.0*1024.0)), 0, this, SLOT(slotDvd4_7GB()),
					d->actionCollection, "fillstatus_dvd_4_7gb" );
  d->actionDvdDoubleLayer = new KRadioAction( KIO::convertSizeFromKB((int)(8.0*1024.0*1024.0)), 0, this, SLOT(slotDvdDoubleLayer()),
					 d->actionCollection, "fillstatus_dvd_double_layer" );
  d->actionCustomSize = new K3bRadioAction( i18n("Custom..."), 0, this, SLOT(slotCustomSize()),
					    d->actionCollection, "fillstatus_custom_size" );
  d->actionCustomSize->setAlwaysEmitActivated(true);
  d->actionDetermineSize = new K3bRadioAction( i18n("From Medium..."), "cdrom_unmount", 0,
					       this, SLOT(slotDetermineSize()),
					       d->actionCollection, "fillstatus_size_from_disk" );
  d->actionDetermineSize->setAlwaysEmitActivated(true);

  d->actionAuto->setExclusiveGroup( "cd_size" );
  d->action74Min->setExclusiveGroup( "cd_size" );
  d->action80Min->setExclusiveGroup( "cd_size" );
  d->action100Min->setExclusiveGroup( "cd_size" );
  d->actionDvd4_7GB->setExclusiveGroup( "cd_size" );
  d->actionDvdDoubleLayer->setExclusiveGroup( "cd_size" );
  d->actionCustomSize->setExclusiveGroup( "cd_size" );
  d->actionDetermineSize->setExclusiveGroup( "cd_size" );

  d->actionLoadUserDefaults = new KAction( i18n("User Defaults"), "", 0,
					   this, SLOT(slotLoadUserDefaults()),
					   d->actionCollection, "load_user_defaults" );
  d->actionSaveUserDefaults = new KAction( i18n("Save User Defaults"), "", 0,
					   this, SLOT(slotSaveUserDefaults()),
					   d->actionCollection, "save_user_defaults" );

  KAction* dvdSizeInfoAction = new KAction( i18n("Why 4.4 instead of 4.7?"), "", 0,
					    this, SLOT(slotWhy44()),
					    d->actionCollection, "why_44_gb" );

  d->popup->insertTitle( i18n("Show Size In") );
  d->actionShowMinutes->plug( d->popup );
  d->actionShowMegs->plug( d->popup );
  d->popup->insertTitle( i18n("CD Size") );
  d->actionAuto->plug( d->popup );
  d->action74Min->plug( d->popup );
  d->action80Min->plug( d->popup );
  d->action100Min->plug( d->popup );
  d->actionCustomSize->plug( d->popup );
  d->actionDetermineSize->plug( d->popup );
  d->popup->insertSeparator();
  d->actionLoadUserDefaults->plug( d->popup );
  d->actionSaveUserDefaults->plug( d->popup );

  d->dvdPopup->insertTitle( i18n("DVD Size") );
  dvdSizeInfoAction->plug( d->dvdPopup );
  d->actionAuto->plug( d->dvdPopup );
  d->actionDvd4_7GB->plug( d->dvdPopup );
  d->actionDvdDoubleLayer->plug( d->dvdPopup );
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


void K3bFillStatusDisplay::slotAutoSize()
{
  slotMediumChanged( 0 );
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
  d->displayWidget->setCdSize( 2295104 );
}


void K3bFillStatusDisplay::slotDvdDoubleLayer()
{
  d->displayWidget->setCdSize( 4173824 );
}


void K3bFillStatusDisplay::slotWhy44()
{
  QWhatsThis::display( i18n("<p><b>Why does K3b offer 4.4 GB and 8.0 GB instead of 4.7 and 8.5 like "
			    "it says on the media?</b>"
			    "<p>A single layer DVD media has a capacity of approximately "
			    "4.4 GB which equals 4.4*1024<sup>3</sup> bytes. Media producers just "
			    "calculate with 1000 instead of 1024 for advertising reasons.<br>"
			    "This results in 4.4*1024<sup>3</sup>/1000<sup>3</sup> = 4.7 GB.") );
}


void K3bFillStatusDisplay::slotCustomSize()
{
  // allow the units to be translated
  QString gbS = i18n("gb");
  QString mbS = i18n("mb");
  QString minS = i18n("min");

  QRegExp rx( "(\\d+\\" + KGlobal::locale()->decimalSymbol() + "?\\d*)(" + gbS + "|" + mbS + "|" + minS + ")?" );
  bool ok;
  QString size = KInputDialog::getText( i18n("Custom Size"),
					i18n("<p>Please specify the size of the media. Use suffixes <b>gb</b>,<b>mb</b>, "
					     "and <b>min</b> for <em>gigabytes</em>, <em>megabytes</em>, and <em>minutes</em>"
					     " respectively."),
					d->showDvdSizes ? QString("4%14%2").arg(KGlobal::locale()->decimalSymbol()).arg(gbS) : 
					(d->showTime ? QString("74")+minS : QString("650")+mbS),
					&ok, this, (const char*)0, 
					new QRegExpValidator( rx, this ) );
  if( ok ) {
    // determine size
    if( rx.exactMatch( size ) ) {
      QString valStr = rx.cap(1);
      if( valStr.endsWith( KGlobal::locale()->decimalSymbol() ) )
	valStr += "0";
      double val = KGlobal::locale()->readNumber( valStr, &ok );
      if( ok ) {
	QString s = rx.cap(2);
	if( s == gbS || (s.isEmpty() && d->showDvdSizes) )
	  val *= 1024*512;
	else if( s == mbS || (s.isEmpty() && !d->showTime) )
	  val *= 512;
	else
	  val *= 60*75;
	d->displayWidget->setCdSize( (int)val );
	update();
      }
    }
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
  bool canceled = false;
  K3bDevice::Device* dev = K3bMediaSelectionDialog::selectMedium( d->showDvdSizes ? K3bDevice::MEDIA_WRITABLE_DVD : K3bDevice::MEDIA_WRITABLE_CD,
								  K3bDevice::STATE_EMPTY|K3bDevice::STATE_INCOMPLETE,
								  parentWidget(),
								  QString::null, QString::null, &canceled );

  if( dev ) {
    K3b::Msf size = k3bappcore->mediaCache()->diskInfo( dev ).capacity();
    if( size > 0 ) {
      d->displayWidget->setCdSize( size );
      d->actionCustomSize->setChecked(true);
      update();
    }
    else
      KMessageBox::error( parentWidget(), i18n("Medium is not empty.") );
  }
  else if( !canceled )
    KMessageBox::error( parentWidget(), i18n("No usable medium found.") );
}


void K3bFillStatusDisplay::slotLoadUserDefaults()
{
  // load project specific values
  KConfig* c = k3bcore->config();
  c->setGroup( "default " + d->doc->typeString() + " settings" );

  // defaults to megabytes
  d->showTime = c->readBoolEntry( "show minutes", false );
  d->displayWidget->setShowTime(d->showTime);
  d->actionShowMegs->setChecked( !d->showTime );
  d->actionShowMinutes->setChecked( d->showTime );


  long size = c->readNumEntry( "default media size", 0 );

  switch( size ) {
  case 0:
    // automatic mode
    d->actionAuto->setChecked( true );
    break;
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

  if( size == 0 ) {
    slotMediumChanged( 0 );
  }
  else {
    d->displayWidget->setCdSize( size*60*75 );
  }
}


void K3bFillStatusDisplay::slotMediumChanged( K3bDevice::Device* )
{
  if( d->actionAuto->isChecked() ) {
    //
    // now search for a usable medium
    // if we find exactly one usable or multiple with the same size
    // we use that size
    //

    // TODO: once we have only one data project we need to change this to handle both

    // FIXME: unless we do not know the real capacity of CD-RW media we can only use
    //        empty here

    K3bDevice::Device* dev = 0;
    QPtrList<K3bDevice::Device> devs;
    if( d->showDvdSizes )
      devs = k3bcore->deviceManager()->dvdWriter();
    else
      devs = k3bcore->deviceManager()->cdWriter();

    for( QPtrListIterator<K3bDevice::Device> it( devs ); *it; ++it ) {
      const K3bMedium& medium = k3bappcore->mediaCache()->medium( *it );

      if( ( medium.diskInfo().empty() || 
	    medium.diskInfo().appendable() || 
	    (medium.diskInfo().rewritable() && medium.diskInfo().isDvdMedia()) ) &&
	  ( medium.diskInfo().isDvdMedia() == d->showDvdSizes ) &&
	  d->doc->length() <= medium.diskInfo().capacity() ) {

	// first usable medium
	if( !dev ) {
	  dev = medium.device();
	}

	// roughly compare the sizes of the two usable media. If they match, carry on.
	else if( k3bappcore->mediaCache()->diskInfo( dev ).capacity().lba()/75/60 != medium.diskInfo().capacity().lba()/75/60 ) {
	  // different usable media -> fallback
	  dev = 0;
	  break;
	}
	// else continue;
      }
    }

    if( dev ) {
      d->displayWidget->setCdSize( k3bappcore->mediaCache()->diskInfo( dev ).capacity().lba() );
    }
    else {
      // default fallback
      d->displayWidget->setCdSize( d->showDvdSizes ? 510*60*75 : 80*60*75 );
    }
  }
}


void K3bFillStatusDisplay::slotSaveUserDefaults()
{
  // save project specific values
  KConfig* c = k3bcore->config();
  c->setGroup( "default " + d->doc->typeString() + " settings" );

  c->writeEntry( "show minutes", d->showTime );
  c->writeEntry( "default media size", d->actionAuto->isChecked() ? 0 : d->displayWidget->cdSize().totalFrames() );
}


void K3bFillStatusDisplay::slotDocChanged()
{
  // cache updates
  if( !d->updateTimer.isActive() ) {
    d->displayWidget->update();
    d->updateTimer.start( 2000, false );
  }

  //
  // also update the medium list in case the docs size exceeds the capacity
  //
  slotMediumChanged( 0 );
}

#include "k3bfillstatusdisplay.moc"
