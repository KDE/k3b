/***************************************************************************
                          k3bstickybutton.cpp  -  description
                             -------------------
    begin                : Sun Apr 1 2001
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

#include <kiconloader.h>

#include "k3bstickybutton.h"

K3bStickyButton::K3bStickyButton(QWidget *parent, const char *name )
	: QToolButton(parent,name)
{
	setFixedSize( 20, 20 );
	setAutoRaise( true );
	setToggleButton( true );
	setOnIconSet( SmallIconSet("sticky") );
	setOffIconSet( SmallIconSet("nonsticky") );
	connect( this, SIGNAL(toggled(bool)), this, SLOT(setOn(bool)) );
}

K3bStickyButton::~K3bStickyButton(){
}

void K3bStickyButton::setOn( bool on )
{
	setDown( on );
	QToolButton::setOn( on );
}


#include "k3bstickybutton.moc"
