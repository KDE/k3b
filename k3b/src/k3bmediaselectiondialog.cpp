/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bmediaselectiondialog.h"
#include "k3bmediaselectioncombobox.h"
#include "k3bmediacache.h"
#include "k3bapplication.h"

#include <klocale.h>

#include <qlayout.h>
#include <qlabel.h>


K3bMediaSelectionDialog::K3bMediaSelectionDialog( QWidget* parent, 
						  const QString& title, 
						  const QString& text, 
						  bool modal )
  : KDialogBase( KDialogBase::Plain, 
		 title.isEmpty() ? i18n("Medium Selection") : title, 
		 Ok|Cancel, 
		 Ok,
		 parent,
		 0,
		 modal )
{
  QGridLayout* lay = new QGridLayout( plainPage() );

  QLabel* label = new QLabel( text.isEmpty() ? i18n("Please select a medium:") : text, plainPage() );
  m_combo = new K3bMediaSelectionComboBox( plainPage() );

  //  lay->setMargin( marginHint() );
  lay->setSpacing( spacingHint() );
  lay->addWidget( label, 0, 0 );
  lay->addWidget( m_combo, 1, 0 );
  lay->setRowStretch( 2, 1 );

  connect( m_combo, SIGNAL(selectionChanged(K3bDevice::Device*)),
	   this, SLOT(slotSelectionChanged(K3bDevice::Device*)) );

  slotSelectionChanged( m_combo->selectedDevice() );
}


K3bMediaSelectionDialog::~K3bMediaSelectionDialog()
{
}


void K3bMediaSelectionDialog::setWantedMediumType( int type )
{
  m_combo->setWantedMediumType( type );
}


void K3bMediaSelectionDialog::setWantedMediumState( int state )
{
  m_combo->setWantedMediumState( state );
}


void K3bMediaSelectionDialog::setWantedMediumContent( int content )
{
  m_combo->setWantedMediumContent( content );
}


K3bDevice::Device* K3bMediaSelectionDialog::selectedDevice() const
{
  return m_combo->selectedDevice();
}


void K3bMediaSelectionDialog::slotSelectionChanged( K3bDevice::Device* dev )
{
  enableButtonOK( dev != 0 );
}


K3bDevice::Device* K3bMediaSelectionDialog::selectMedium( int type, int state, int content,
							  QWidget* parent, 
							  const QString& title, const QString& text,
							  bool* canceled )
{
  K3bMediaSelectionDialog dlg( parent, title, text );
  dlg.setWantedMediumType( type );
  dlg.setWantedMediumState( state );
  dlg.setWantedMediumContent( content );

  // even if no usable medium is inserted the combobox shows the "insert one" message
  // so it's not sufficient to check for just one entry to check if there only is a 
  // single useable medium
  if( ( dlg.selectedDevice() && dlg.m_combo->count() == 1 ) 
      || dlg.exec() == Accepted ) {
    if( canceled )
      *canceled = false;
    return dlg.selectedDevice();
  }
  else {
    if( canceled )
      *canceled = true;
    return 0;
  }
}


K3bDevice::Device* K3bMediaSelectionDialog::selectMedium( int type, int state, 
							  QWidget* parent, 
							  const QString& title, const QString& text,
							  bool* canceled )
{
  return selectMedium( type, state, K3bMedium::CONTENT_ALL, parent, title, text, canceled );
}

#include "k3bmediaselectiondialog.moc"
