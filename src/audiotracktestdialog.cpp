/***************************************************************************
                          audiotracktestdialog.cpp  -  description
                             -------------------
    begin                : Thu Mar 29 2001
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

#include <qlayout.h>
#include <qlabel.h>

#include <kprogress.h>
#include <kpushbutton.h>

#include "audiotracktestdialog.h"

AudioTrackTestDialog::AudioTrackTestDialog( const QString& fileName, QWidget *parent, const char *name )
 : KDialog(parent, name, true, Qt::WDestructiveClose)
{
	m_progress = new KProgress(0, 100, 0, Qt::Horizontal, this );
	m_cancelButton = new KPushButton( "Cancel", this );
	QLabel* label = new QLabel( "Adding file " + fileName, this );
	QVBoxLayout* box = new QVBoxLayout( this );
	
	m_cancelButton->setFixedWidth( 100 );
	m_cancelButton->setDefault( true );
	m_progress->setMinimumWidth( 300 );
	label->setAlignment( Qt::AlignCenter );
	
	box->setMargin( 8 );
	box->setSpacing( 8 );
	
	box->add( label );
	box->add( m_progress );
	box->add( m_cancelButton );	
	
	connect( m_cancelButton, SIGNAL(pressed()), this, SIGNAL(canceled()) );
	connect( m_cancelButton, SIGNAL(pressed()), this, SLOT(close()) );
}

AudioTrackTestDialog::~AudioTrackTestDialog(){
}

void AudioTrackTestDialog::setPercent( int value )
{
	if( value < 0 )
		value = 0;
	if( value > 100 )
		value = 100;
		
	m_progress->setValue( value );
	
	if( value == 100 )
		close();
}

void AudioTrackTestDialog::show(){
	// i hope that this will return imediately
	QWidget::show();
}
