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

#include "k3bwritingmodewidget.h"

#include <k3bglobals.h>

#include <klocale.h>
#include <kconfig.h>

#include <qtooltip.h>
#include <qwhatsthis.h>


class K3bWritingModeWidget::Private
{
public:
  int modes;  
};


K3bWritingModeWidget::K3bWritingModeWidget( int modes, QWidget* parent, const char* name )
  : KComboBox( parent, name )
{
  init();
  setSupportedModes( modes );
}


K3bWritingModeWidget::K3bWritingModeWidget( QWidget* parent, const char* name )
  : KComboBox( parent, name )
{
  init();
  setSupportedModes( 0xFF );   // default: support all modes
}


K3bWritingModeWidget::~K3bWritingModeWidget()
{
  delete d;
}


void K3bWritingModeWidget::init()
{
  d = new Private();

  connect( this, SIGNAL(activated(int)), this, SLOT(slotActivated(int)) );

  QToolTip::add( this, i18n("Select the writing mode to use") );
  QWhatsThis::add( this, i18n("<p><b>Writing mode</b>"
			      "<p><b>Auto</b><br>"
			      "Let K3b select the best suited mode.</p>"
			      "<p><b>DAO</b><br>"
			      "<em>Disk At Once</em> or more properly <em>Session At Once</em>. "
			      "The laser is never turned off while writing the cd. "
			      "This is the preferred mode to write audio cds since it allows "
			      "pregaps other than 2 seconds. Not all writers support DAO.</p>"
			      "<p><b>TAO</b><br>"
			      "<em>Track At Once</em> should be supported by every writer. "
			      "The laser will be turned off after every track.</p>"
			      "<p><b>RAW</b><br>"
			      "RAW writing mode</p>") );
}


int K3bWritingModeWidget::writingMode() const
{
  if( currentText() == i18n("DAO") )
    return K3b::DAO;
  else if( currentText() == i18n("TAO") )
    return K3b::TAO;
  else if( currentText() == i18n("RAW") )
    return K3b::RAW;
  else
    return K3b::WRITING_MODE_AUTO;
}


void K3bWritingModeWidget::setWritingMode( int m )
{
  if( m & d->modes ) {
    switch( m ) {
    case K3b::DAO:
      setCurrentItem( i18n("DAO"), false );
      break;
    case K3b::TAO:
      setCurrentItem( i18n("TAO"), false );
      break;
    case K3b::RAW:
      setCurrentItem( i18n("RAW"), false );
      break;
    default:
      setCurrentItem( 0 ); // WRITING_MODE_AUTO
      break;
    }
  }
  else {
    setCurrentItem( 0 ); // WRITING_MODE_AUTO
  }
}


void K3bWritingModeWidget::setSupportedModes( int m )
{
  // save current mode
  int currentMode = writingMode();

  d->modes = m|K3b::WRITING_MODE_AUTO;  // we always support the Auto mode

  clear();

  insertItem( i18n("Auto") );
  if( m & K3b::DAO )
    insertItem( i18n("DAO") );
  if( m & K3b::TAO )
    insertItem( i18n("TAO") );
  if( m & K3b::RAW )
    insertItem( i18n("RAW") );

  // restore saved mode
  setWritingMode( currentMode );
}


void K3bWritingModeWidget::slotActivated( int )
{
  emit writingModeChanged( writingMode() );
}


void K3bWritingModeWidget::saveConfig( KConfig* c )
{
  switch( writingMode() ) {
  case K3b::DAO:
    c->writeEntry( "writing_mode", "dao" );
    break;
  case K3b::TAO:
    c->writeEntry( "writing_mode", "tao" );
    break;
  case K3b::RAW:
    c->writeEntry( "writing_mode", "raw" );
    break;
  default:
    c->writeEntry( "writing_mode", "auto" );
    break;
  }
}

void K3bWritingModeWidget::loadConfig( KConfig* c )
{
  QString mode = c->readEntry( "writing_mode" );
  if ( mode == "dao" )
    setWritingMode( K3b::DAO );
  else if( mode == "tao" )
    setWritingMode( K3b::TAO );
  else if( mode == "raw" )
    setWritingMode( K3b::RAW );
  else
    setWritingMode( K3b::WRITING_MODE_AUTO );
}

#include "k3bwritingmodewidget.moc"
