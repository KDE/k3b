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


#include "k3bdatamodewidget.h"
#include <tools/k3bglobals.h>

#include <klocale.h>

#include <qwhatsthis.h>
#include <qtooltip.h>

static const int s_autoIndex = 0;
static const int s_mode1Index = 1;
static const int s_mode2Index = 2;


K3bDataModeWidget::K3bDataModeWidget( QWidget* parent, const char* name )
  : QComboBox( false, parent, name )
{
  insertItem( i18n("Auto"), s_autoIndex );
  insertItem( i18n("Mode1"), s_mode1Index );
  insertItem( i18n("Mode2"), s_mode2Index );

  QToolTip::add( this,i18n("Select the mode for the data-track") );
  QWhatsThis::add( this, i18n("<p>Data tracks may be written in two different modes:</p>"
			      "<p><b>Mode 1</b><br>"
			      "This is the <em>original</em> writing mode as introduced in the "
			      "<em>Yellow Book</em> standard. It is the preferred mode when writing "
			      "pure data cds.</p>"
			      "<p><b>Mode 2</b><br>"
			      "To be exact <em>XA Mode 2 Form 1</em>, but since the "
			      "other modes are rarely used it is common to refer to it as <em>Mode 2</em>.</p>"
			      "<p><b>Auto</b><br>"
			      "lets K3b select the best suited data mode.</p>"
			      "<p><b>Be aware:</b> Do not mix different modes on one cd. "
			      "Some older drives may have problems reading mode 1 multisession cds.") );
}


K3bDataModeWidget::~K3bDataModeWidget()
{
}


int K3bDataModeWidget::dataMode() const
{
  if( currentItem() == s_autoIndex )
    return K3b::AUTO;
  else if( currentItem() == s_mode1Index )
    return K3b::MODE1;
  else
    return K3b::MODE2;
}


void K3bDataModeWidget::setDataMode( int mode )
{
  if( mode == K3b::MODE1 )
    setCurrentItem( s_mode1Index );
  else if( mode == K3b::MODE2 )
    setCurrentItem( s_mode2Index );
  else
    setCurrentItem( s_autoIndex );
}


#include "k3bdatamodewidget.moc"
