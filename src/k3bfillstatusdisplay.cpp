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

#include <kaction.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <klineeditdlg.h>
#include <kconfig.h>


K3bFillStatusDisplay::K3bFillStatusDisplay(K3bDoc* doc, QWidget *parent, const char *name )
  : QFrame(parent,name)
{
  m_doc = doc;
  k3bMain()->config()->setGroup( "General Options" );
  m_cdSize = k3bMain()->config()->readNumEntry( "Default cd size", 74 );

  setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred ) );	
  setFrameStyle( Panel | Sunken );
  setupPopupMenu();

  // defaults to megabytes
  m_showTime = false;
  m_actionShowMegs->setChecked( true );

  switch( m_cdSize ) {
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
}


void K3bFillStatusDisplay::mousePressEvent( QMouseEvent* e )
{
  if( e->button() == Qt::RightButton )
    m_popup->popup( e->globalPos() );
}


void K3bFillStatusDisplay::drawContents( QPainter* p )
{
  erase( contentsRect() );

  long docSize;
  long cdSize;
  long maxValue;
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
  double one = (double)contentsRect().width() / (double)maxValue;
  QRect rect( contentsRect() );
  rect.setWidth( (int)(one*(double)docSize) );
	
  p->fillRect( rect, Qt::green );

  if( m_showTime )
    p->drawText( contentsRect(), Qt::AlignLeft | Qt::AlignVCenter, 
		 " " + K3b::framesToString( m_doc->length(), false ) + " min" );
  else
    p->drawText( contentsRect(), Qt::AlignLeft | Qt::AlignVCenter, 
		 QString().sprintf( " %.2f MB", ((float)m_doc->size())/1024.0/1024.0 ) );
	
  // draw yellow if m_cdSize - tolerance < docSize
  if( docSize > cdSize - tolerance ) {
    rect.setLeft( rect.left() + (int)(one * (cdSize - tolerance)) );
    p->fillRect( rect, Qt::yellow );
  }
	
  // draw red if docSize > cdSize + tolerance
  if( docSize > cdSize + tolerance ) {
    rect.setLeft( rect.left() + (int)(one * tolerance*2) );
    p->fillRect( rect, Qt::red );
  }
	

  p->drawLine( contentsRect().left() + (int)(one*cdSize), contentsRect().bottom(), 
	       contentsRect().left() + (int)(one*cdSize), contentsRect().top() );
	
  // draw the text marks
  rect = contentsRect();
  rect.setLeft( (int)(one*cdSize) );
  p->drawText( rect, Qt::AlignLeft | Qt::AlignVCenter, " " + QString::number(cdSize) );
}


void K3bFillStatusDisplay::showSize()
{
  m_actionShowMegs->setChecked( true );

  m_action74Min->setText( i18n("%1 MB").arg(650) );
  m_action80Min->setText( i18n("%1 MB").arg(700) );
  m_action100Min->setText( i18n("%1 MB").arg(880) );

  m_showTime = false;
  update();
}
	
void K3bFillStatusDisplay::showTime()
{
  m_actionShowMinutes->setChecked( true );

  m_action74Min->setText( i18n("%1 minutes").arg(74) );
  m_action80Min->setText( i18n("%1 minutes").arg(80) );
  m_action100Min->setText( i18n("%1 minutes").arg(100) );

  m_showTime = true;
  update();
}


QSize K3bFillStatusDisplay::sizeHint() const
{
  return minimumSizeHint();
}


QSize K3bFillStatusDisplay::minimumSizeHint() const
{
  int margin = 2;
  QFontMetrics fm( font() );
  return QSize( -1, 2 * frameWidth() + fm.height() + 2 * margin );
}


void K3bFillStatusDisplay::slot74Minutes()
{
  m_cdSize = 74;
  update();
}


void K3bFillStatusDisplay::slot80Minutes()
{
  m_cdSize = 80;
  update();
}


void K3bFillStatusDisplay::slot100Minutes()
{
  m_cdSize = 100;
  update();
}


void K3bFillStatusDisplay::slotCustomSize()
{
  bool ok;
  QString size = KLineEditDlg::getText( i18n("Custom cd size"), i18n("Please specify the size of your CD in minutes:"), 
					   "74", &ok, this, new QIntValidator( this ) );
  if( ok ) {
    m_cdSize = size.toInt();
    update();
  }
}


#include "k3bfillstatusdisplay.moc"
