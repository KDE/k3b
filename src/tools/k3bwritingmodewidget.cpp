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
  setSupportedModes( K3b::DAO | K3b::TAO | K3b::RAW );   // default: support all CD-R(W) modes
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

  initWhatsThisHelp();
}


void K3bWritingModeWidget::initWhatsThisHelp()
{
  static QString s_daoHelp = i18n("<em>Disk At Once</em> or more properly <em>Session At Once</em>. "
				  "The laser is never turned off while writing the CD or DVD. "
				  "This is the preferred mode to write audio CDs since it allows "
				  "pregaps other than 2 seconds. Not all writers support DAO.<br>"
				  "DVDs written in DAO provide the best DVD-Video compatibility.");
  static QString s_taoHelp = i18n("<em>Track At Once</em> should be supported by every CD writer. "
				  "The laser will be turned off after every track.<br>"
				  "Most CD writers need this mode for writing multisession CDs.");
  static QString s_rawHelp = i18n("RAW writing mode. The error correction data is created by the "
				  "software instead of the writer device.<br>"
				  "Try this if your CD writer fails to write in DAO and TAO.");
  static QString s_seqHelp = i18n("Incremental writing allows multisession. It only applies to DVD-R(W). "
				  "Bla bla bla. FIXME");
  static QString s_ovwHelp = i18n("Restricted Overwrite allows to use a DVD-RW just like a DVD-RAM "
				  "or a DVD+RW. Bla bla bla. FIXME");

  QWhatsThis::remove( this );
  QString wh =
    "<p><b>" + i18n("Writing mode") + "</b></p>" +
    "<p><b>" + i18n("Auto") + "</b><br>" +
    i18n("Let K3b select the best suited mode. This is the recommended selection.") + "</p>";

  if( d->modes & K3b::DAO )
    wh += "<p><b>" + i18n("DAO") + "</b><br>" + s_daoHelp + "</p>";
  if( d->modes & K3b::TAO )
    wh += "<p><b>" + i18n("TAO") + "</b><br>" + s_taoHelp + "</p>";
  if( d->modes & K3b::RAW )
    wh += "<p><b>" + i18n("RAW") + "</b><br>" + s_rawHelp + "</p>";
  if( d->modes & K3b::WRITING_MODE_INCR_SEQ )
    wh += "<p><b>" + i18n("Incremental") + "</b><br>" + s_seqHelp + "</p>";
  if( d->modes & K3b::WRITING_MODE_RES_OVWR )
    wh += "<p><b>" + i18n("Restricted Overwrite") + "</b><br>" + s_ovwHelp + "</p>";

  QWhatsThis::add( this, wh );
}


int K3bWritingModeWidget::writingMode() const
{
  if( currentText() == i18n("DAO") )
    return K3b::DAO;
  else if( currentText() == i18n("TAO") )
    return K3b::TAO;
  else if( currentText() == i18n("RAW") )
    return K3b::RAW;
  else if( currentText() == i18n("Incremental") )
    return K3b::WRITING_MODE_INCR_SEQ;
  else if( currentText() == i18n("Overwrite") )
    return K3b::WRITING_MODE_RES_OVWR;
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
    case K3b::WRITING_MODE_INCR_SEQ:
      setCurrentItem( i18n("Incremental"), false );
      break;
    case K3b::WRITING_MODE_RES_OVWR:
      setCurrentItem( i18n("Overwrite") );
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
  if( m & K3b::WRITING_MODE_RES_OVWR )
    insertItem( i18n("Overwrite") );
  if( m & K3b::WRITING_MODE_INCR_SEQ )
    insertItem( i18n("Incremental") );

  // restore saved mode
  setWritingMode( currentMode );

  initWhatsThisHelp();
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
  case K3b::WRITING_MODE_INCR_SEQ:
    c->writeEntry( "writing_mode", "incremental" );
    break;
  case K3b::WRITING_MODE_RES_OVWR:
    c->writeEntry( "writing_mode", "overwrite" );
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
  else if( mode == "incremental" )
    setWritingMode( K3b::WRITING_MODE_INCR_SEQ );
  else if( mode == "overwrite" )
    setWritingMode( K3b::WRITING_MODE_RES_OVWR );
  else
    setWritingMode( K3b::WRITING_MODE_AUTO );
}

#include "k3bwritingmodewidget.moc"
