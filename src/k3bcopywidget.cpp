/***************************************************************************
                          k3bcopywidget.cpp  -  description
                             -------------------
    begin                : Tue Mar 27 2001
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

#include "k3bcopywidget.h"

#include <qlabel.h>
#include <qlayout.h>


K3bCopyWidget::K3bCopyWidget(QWidget *parent, const char *name )
	: QWidget(parent,name)
{
	QLabel* mainLabel = new QLabel( "Not implemented yet!", this );
	QVBoxLayout* mainLay = new QVBoxLayout( this );
	mainLay->addWidget( mainLabel );
	mainLabel->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignCenter ) );
}

K3bCopyWidget::~K3bCopyWidget(){
}

#include "k3bcopywidget.moc"

