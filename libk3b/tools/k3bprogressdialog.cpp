/* 
 *
 * $Id$
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

#include "k3bprogressdialog.h"

#include <k3bbusywidget.h>

#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <q3frame.h>
#include <q3widgetstack.h>
//Added by qt3to4:
#include <Q3GridLayout>

#include <klocale.h>
#include <kprogress.h>


K3bProgressDialog::K3bProgressDialog( const QString& text,
				      QWidget* parent, 
				      const QString& caption,
				      const char* name ) 
  : KDialogBase( parent, name, true, caption, Cancel|Ok, Ok, true )
{
  QFrame* main = makeMainWidget();
  Q3GridLayout* mainLayout = new Q3GridLayout( main );
  mainLayout->setMargin( marginHint() );
  mainLayout->setSpacing( spacingHint() );

  m_label = new QLabel( text, main );
  m_stack = new Q3WidgetStack( main );
  m_progressBar = new KProgress( m_stack );
  m_busyWidget = new K3bBusyWidget( m_stack );
  m_stack->addWidget( m_progressBar );
  m_stack->addWidget( m_busyWidget );

  mainLayout->addWidget( m_label, 0, 0 );
  mainLayout->addWidget( m_stack, 1, 0 );

  showButtonOK( false );
}


K3bProgressDialog::~K3bProgressDialog()
{}


int K3bProgressDialog::exec( bool progress )
{
  if( progress )
    m_stack->raiseWidget( m_progressBar );
  else
    m_stack->raiseWidget( m_busyWidget );

  m_busyWidget->showBusy( !progress );

  actionButton( Cancel )->setEnabled(true);

  return KDialogBase::exec();
}


void K3bProgressDialog::setText( const QString& text )
{
  m_label->setText( text );
}


void K3bProgressDialog::slotFinished( bool success )
{
  m_busyWidget->showBusy( false );

  showButtonOK( true );
  showButtonCancel( false );

  if( success )
    m_label->setText( i18n("Disk successfully erased. Please reload the disk.") );
  else
    m_label->setText( i18n("K3b was unable to erase the disk.") );
}


void K3bProgressDialog::slotCancel()
{
  emit cancelClicked();
  // we simply forbid to click cancel twice
  actionButton( Cancel )->setEnabled(false);
}


void K3bProgressDialog::setProgress( int p )
{
  m_progressBar->setProgress( p );
}

#include "k3bprogressdialog.moc"
