/***************************************************************************
                          k3baudiotrackdialog.cpp  -  description
                             -------------------
    begin                : Sat Mar 31 2001
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

#include <qlineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>

#include <kiconloader.h>
#include <klocale.h>
#include <knuminput.h>

#include "k3baudiotrackdialog.h"
#include "k3baudiotrack.h"
#include "../k3bstickybutton.h"


K3bAudioTrackDialog::K3bAudioTrackDialog(QWidget *parent, const char *name )
	: KDialog(parent,name), m_track(0)
{
	QGridLayout* mainLayout = new QGridLayout( this );
	mainLayout->setMargin( 5 );
	mainLayout->setSpacing( 5 );
	
	buttonSticky = new K3bStickyButton( this );
	buttonSticky->setOn(true);
	
	mainLayout->addMultiCellWidget( setupInfoBox(), 0, 0, 0, 1 );
	mainLayout->addMultiCellWidget( setupTagBox(), 1, 1, 0, 1 );
	mainLayout->addWidget( buttonSticky, 2, 1 );
	
	connect( inputTitle, SIGNAL(textChanged(const QString&)), this, SLOT(updateTitle(const QString&)) );
	connect( inputArtist, SIGNAL(textChanged(const QString&)), this, SLOT(updateArtist(const QString&)) );
	connect( inputPregap, SIGNAL(valueChanged(int)), this, SLOT(updatePregap(int)) );
	connect( buttonSticky, SIGNAL(toggled(bool)), this, SLOT(setSticky(bool)) );
	
	// set defaults
	m_sticky = true;
}

K3bAudioTrackDialog::~K3bAudioTrackDialog()
{
}

void K3bAudioTrackDialog::setTrack( K3bAudioTrack* _track )
{
	m_track = _track;
	updateView();
}

void K3bAudioTrackDialog::updateTitle( const QString& _title )
{
	if( m_track )
		m_track->setTitle( _title );
}

void K3bAudioTrackDialog::updateArtist( const QString& _artist )
{
	if( m_track )
		m_track->setArtist( _artist );
}

void K3bAudioTrackDialog::updatePregap( int value )
{
	if( m_track )
		m_track->setPregap( value );
}

QGroupBox* K3bAudioTrackDialog::setupInfoBox()
{
	QGroupBox* _infoBox = new QGroupBox( "File Info", this );
	QGridLayout* grid = new QGridLayout( _infoBox );
	grid->setMargin( 15 );
	grid->setSpacing( 6 );
	
	labelFileName = new QLabel( "", _infoBox );
	labelTrackLength = new QLabel( "", _infoBox );
	QLabel* _label1 = new QLabel( "Filename", _infoBox );
	QLabel* _label2 = new QLabel( "Length", _infoBox );

	grid->addWidget( _label1, 0, 0 );
	grid->addMultiCellWidget( labelFileName, 0, 0, 1, 4 );
	grid->addWidget( _label2, 1, 3 );
	grid->addWidget( labelTrackLength, 1, 4 );
		
	return _infoBox;
}

QGroupBox* K3bAudioTrackDialog::setupTagBox()
{
	QGroupBox* _tagBox = new QGroupBox( "Track Info", this );
	QGridLayout* grid = new QGridLayout( _tagBox );
	grid->setMargin( 15 );
	grid->setSpacing( 6 );
	
	inputTitle = new QLineEdit( _tagBox );
	inputArtist = new QLineEdit(_tagBox );
	inputPregap = new KIntNumInput( _tagBox );
	QLabel* _label1 = new QLabel( inputArtist, "&Artist", _tagBox );
	QLabel* _label2 = new QLabel( inputTitle, "&Title", _tagBox );
	QLabel* _label3 = new QLabel( inputPregap, "&Pregap", _tagBox );
	
	grid->addWidget( _label1, 1,0 );
	grid->addMultiCellWidget( inputArtist, 1,1,1,2 );
	grid->addWidget( _label2, 2,0 );
	grid->addMultiCellWidget( inputTitle, 2,2,1,2 );
	grid->addWidget( _label3, 0, 0 );
	grid->addWidget( inputPregap, 0, 1 );
	
	grid->setColStretch( 1, 1 );
	grid->setColStretch( 2, 2 );
	
	_tagBox->setTabOrder( inputArtist, inputTitle );
	_tagBox->setTabOrder( inputTitle, inputPregap );
	
	return _tagBox;
}

void K3bAudioTrackDialog::updateView()
{
	if( m_track ) {
		inputTitle->setText( m_track->title() );
		inputArtist->setText( m_track->artist() );
		inputPregap->setValue( m_track->pregap() );
		labelFileName->setText( m_track->fileName() );
		labelTrackLength->setText( m_track->length().toString() );
	}
	else  {
		inputTitle->setText("");
		inputArtist->setText("");
		inputPregap->setValue( 2 );
		labelFileName->setText("");
		labelTrackLength->setText("");
		if( !sticky() )
			hide();
	}	
}
