/***************************************************************************
                          k3bfillstatusdisplay.cpp  -  description
                             -------------------
    begin                : Tue Apr 10 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#include "k3bfillstatusdisplay.h"
#include "audio/k3baudiodoc.h"
#include "tools/k3bglobals.h"
#include "k3b.h"

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


K3bFillStatusDisplayWidget::K3bFillStatusDisplayWidget( K3bDoc* doc, QWidget* parent )
  : QWidget( parent ),
    m_doc(doc)
{
  k3bMain()->config()->setGroup( "General Options" );
  m_cdSize = k3bMain()->config()->readNumEntry( "Default cd size", 74 );

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


void K3bFillStatusDisplayWidget::setCdSize( long size )
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
    docSize = m_doc->length() / 75 / 60;
    cdSize = m_cdSize;
    maxValue = (cdSize > docSize ? cdSize : docSize) + 10;
    tolerance = 1;
  }
  else {
    docSize = m_doc->size()/1024/1024;
    cdSize = m_cdSize*60*75*2048/1024/1024;
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
		 " " + K3b::framesToString( m_doc->length(), false ) + " min" );
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
  crect.setLeft( (int)(one*cdSize) );
  p.drawText( crect, Qt::AlignLeft | Qt::AlignVCenter, " " + QString::number((long)cdSize) );
}



// ----------------------------------------------------------------------------------------------------

K3bFillStatusDisplay::K3bFillStatusDisplay(K3bDoc* doc, QWidget *parent, const char *name )
  : QFrame(parent,name)
{
  setFrameStyle( Panel | Sunken );

  m_displayWidget = new K3bFillStatusDisplayWidget( doc, this );
  m_buttonMenu = new QToolButton( this );
  m_buttonMenu->setIconSet( SmallIconSet("cdrom_unmount") );
  m_buttonMenu->setAutoRaise(true);
  QToolTip::add( m_buttonMenu, i18n("Fill display properties") );
  connect( m_buttonMenu, SIGNAL(clicked()), this, SLOT(slotMenuButtonClicked()) );

  QGridLayout* layout = new QGridLayout( this );
  layout->setSpacing(5);
  layout->setMargin(frameWidth());
  layout->addWidget( m_displayWidget, 0, 0 );
  layout->addWidget( m_buttonMenu, 0, 1 );
  layout->setColStretch( 0, 1 );

  setupPopupMenu();

  // defaults to megabytes
  m_displayWidget->setShowTime(false);
  m_actionShowMegs->setChecked( true );

  switch( m_displayWidget->cdSize() ) {
  case 74:
    m_action74Min->setChecked( true );
    break;
  case 80:
    m_action80Min->setChecked( true );
    break;
  case 100:
    m_action100Min->setChecked( true );
    break;
  default:
    m_actionCustomSize->setChecked( true );
    break;
  }
}

K3bFillStatusDisplay::~K3bFillStatusDisplay()
{
}


void K3bFillStatusDisplay::paintEvent(QPaintEvent* e)
{
  // just to pass updates to the display
  m_displayWidget->update();
  QFrame::paintEvent(e);
}

void K3bFillStatusDisplay::setupPopupMenu()
{
  m_popup = new KPopupMenu( this, "popup" );
  m_actionShowMinutes = new KToggleAction( i18n("Minutes"), "kmidi", 0, this, SLOT(showTime()), this );
  m_actionShowMegs = new KToggleAction( i18n("Megabytes"), "kwikdisk", 0, this, SLOT(showSize()), this );

  m_actionShowMegs->setExclusiveGroup( "show_size_in" );
  m_actionShowMinutes->setExclusiveGroup( "show_size_in" );

  m_action74Min = new KToggleAction( i18n("%1 MB").arg(650), 0, this, SLOT(slot74Minutes()), this );
  m_action80Min = new KToggleAction( i18n("%1 MB").arg(700), 0, this, SLOT(slot80Minutes()), this );
  m_action100Min = new KToggleAction( i18n("%1 MB").arg(880), 0, this, SLOT(slot100Minutes()), this );
  m_actionCustomSize = new KToggleAction( i18n("custom..."), 0, this, SLOT(slotCustomSize()), this );

  m_action74Min->setExclusiveGroup( "cd_size" );
  m_action80Min->setExclusiveGroup( "cd_size" );
  m_action100Min->setExclusiveGroup( "cd_size" );
  m_actionCustomSize->setExclusiveGroup( "cd_size" );
 
  m_popup->insertTitle( i18n("Show size in...") );
  m_actionShowMinutes->plug( m_popup );
  m_actionShowMegs->plug( m_popup );
  m_popup->insertTitle( i18n("CD size") );
  m_action74Min->plug( m_popup );
  m_action80Min->plug( m_popup );
  m_action100Min->plug( m_popup );
  m_actionCustomSize->plug( m_popup );

  connect( m_displayWidget, SIGNAL(contextMenu(const QPoint&)), this, SLOT(slotPopupMenu(const QPoint&)) );
}


void K3bFillStatusDisplay::showSize()
{
  m_actionShowMegs->setChecked( true );

  m_action74Min->setText( i18n("650 MB"));
  m_action80Min->setText( i18n("700 MB"));
  m_action100Min->setText( i18n("880 MB") );

  m_displayWidget->setShowTime(false);
}
	
void K3bFillStatusDisplay::showTime()
{
  m_actionShowMinutes->setChecked( true );

  // if you have more than three strings, it might be easier to
  // use i18n("unused", "%n minutes", 74);
  m_action74Min->setText( i18n("74 minutes") );
  m_action80Min->setText( i18n("80 minutes") );
  m_action100Min->setText( i18n("100 minutes") );

  m_displayWidget->setShowTime(true);
}


void K3bFillStatusDisplay::slot74Minutes()
{
  m_displayWidget->setCdSize( 74 );
}


void K3bFillStatusDisplay::slot80Minutes()
{
  m_displayWidget->setCdSize( 80 );
}


void K3bFillStatusDisplay::slot100Minutes()
{
  m_displayWidget->setCdSize( 100 );
}


void K3bFillStatusDisplay::slotCustomSize()
{
  bool ok;
  QString size = KLineEditDlg::getText( i18n("Custom CD size"), i18n("Please specify the size of your CD in minutes:"), 
					   "74", &ok, this, new QIntValidator( this ) );
  if( ok ) {
    m_displayWidget->setCdSize( size.toInt() );
    update();
  }
}


void K3bFillStatusDisplay::slotMenuButtonClicked()
{
  QSize size = m_popup->sizeHint();
  slotPopupMenu( m_buttonMenu->mapToGlobal(QPoint(m_buttonMenu->width(), 0)) + QPoint(-1*size.width(), -1*size.height()) );
}


void K3bFillStatusDisplay::slotPopupMenu( const QPoint& p )
{
  m_popup->popup(p);
}

#include "k3bfillstatusdisplay.moc"
