/***************************************************************************
                          k3bemptydiscwaiter.cpp  -  description
                             -------------------
    begin                : Mon Oct 22 2001
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

#include "k3bemptydiscwaiter.h"
#include "k3bdevice.h"

#include <qtimer.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <kiconloader.h>


K3bEmptyDiscWaiter::K3bEmptyDiscWaiter( K3bDevice* device, QWidget* parent, const char* name )
  : KDialogBase( KDialogBase::Plain, i18n("insert empty disk"), KDialogBase::Cancel | KDialogBase::User1, 
		 KDialogBase::Cancel, parent, name, true, true, i18n("Force") )
{
  m_timer = new QTimer( this );
  m_device = device;

  QLabel* label = new QLabel( plainPage() );
  label->setText( i18n("Please insert an empty cdr medium into drive<p><b>%1 %2 (%3)</b>.").arg(m_device->vendor()).arg(m_device->description()).arg(m_device->devicename()) );
  label->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

  QLabel* pixLabel = new QLabel( plainPage() );
  pixLabel->setAlignment( Qt::AlignCenter | Qt::AlignVCenter );
  pixLabel->setPixmap( KGlobal::instance()->iconLoader()->loadIcon( "cdwriter_unmount", KIcon::NoGroup, KIcon::SizeMedium ) );

  QHBoxLayout* box = new QHBoxLayout( plainPage() );
  box->setSpacing( 20 );
  box->setMargin( marginHint() );
  box->addWidget( pixLabel );
  box->addWidget( label );
  box->setStretchFactor( label, 1 );

  QToolTip::add( actionButton(KDialogBase::User1), i18n("Force K3b to continue if it seems not to detect your empty cdr") );
}


K3bEmptyDiscWaiter::~K3bEmptyDiscWaiter()
{
}

void K3bEmptyDiscWaiter::waitForEmptyDisc( bool appendable )
{
  m_apppendable = appendable;

  connect( m_timer, SIGNAL(timeout()), this, SLOT(slotTestForEmptyCd()) );
  m_timer->start(1000);

  show();
}


void K3bEmptyDiscWaiter::slotTestForEmptyCd()
{
  int x = m_device->isEmpty();
  if( x == 0 || ( x == 1 && m_apppendable ) ) {
    m_timer->stop();
    m_timer->disconnect();
    delayedDestruct();
    
    emit discReady();
  }
}


void K3bEmptyDiscWaiter::slotCancel()
{
  m_timer->stop();
  m_timer->disconnect();
  delayedDestruct();

  emit canceled();
}


void K3bEmptyDiscWaiter::slotUser1()
{
  delayedDestruct();

  emit discReady();
}


#include "k3bemptydiscwaiter.moc"
